// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "noff.h"

void pageFaultHandler();
//----------------------------------------------------------------------
// TLB replacement 
// if TLB is full, find one entry which is less likely to be used in the 
// future.
// arg method means different algorithm
// add in lab4 ex3
//----------------------------------------------------------------------
int findReplace(int method){
    int index;
    if(method == 0) // stupid one
        return 0; 
    else if (method == 1){ // LRU
        int t = stats->totalTicks + 1;
        for(int i = 0; i < TLBSize; i++){
            if(machine->tlb[i].lastUse < t){
                t = machine->tlb[i].lastUse;
                index = i;
            }
        }
        return index;
    }
    else{ // LFU
        int f = 0x7fffffff;
        for(int i = 0; i < TLBSize; i++){
            if(machine->tlb[i].firstInTime < f){
                f = machine->tlb[i].firstInTime;
                index = i;
            }
        }
        return index;
    }
}

void tlbMissHandler(){
    int badVAddr = machine->ReadRegister(BadVAddrReg);
    unsigned int vpn = (unsigned) badVAddr / PageSize;
    if(machine->pageTable[vpn].valid == FALSE)
        pageFaultHandler(); 
    TranslationEntry *entry = &(machine->pageTable[vpn]); // find entry in the page table
    // TLB replace
    int i;
    for(i = 0; i < TLBSize; i++){
        if(!machine->tlb[i].valid){
            break;
        }
    }
    if(i == TLBSize){
        i = findReplace(1);
    }
    // restore old entry
    unsigned int old_vpn = machine->tlb[i].virtualPage;
    machine->pageTable[old_vpn].dirty = machine->tlb[i].dirty; 
    // load new entry into tlb
    machine->tlb[i].virtualPage = vpn;	// for now, virtual page # = phys page #
    machine->tlb[i].physicalPage = entry->physicalPage;
    machine->tlb[i].valid = true;
    machine->tlb[i].use = false;
    machine->tlb[i].dirty = false;
    machine->tlb[i].readOnly = entry->readOnly;
    machine->tlb[i].firstInTime = stats->totalTicks;
    //machine->tlb[i].lastUse = 0;
}

//----------------------------------------------------------------------
// page fault handler
// if there is enough space in memory, load page into physical memory
// if not, replace a page
// add in lab4 ex6
//----------------------------------------------------------------------

void pageFaultHandler(){
    printf("page fault\n");
    unsigned int badVAddr = machine->ReadRegister(BadVAddrReg);
    printf("bad vaddr:%u\n",badVAddr);
    int vpn = (unsigned) badVAddr / PageSize;
    //int offset = (unsigned) badVAddr % PageSize;
    #ifndef USE_INV
    int ppn = machine->mBitMap->Find(); // find if there exists empty pages
    if(ppn == -1){
        // should replace a page
        int i, vpn_old;
        int t = stats->totalTicks + 1;
        for(i = 0; i < machine->pageTableSize; i++){
            if(machine->pageTable[i].valid && machine->pageTable[i].lastUse < t){
                t = machine->pageTable[i].lastUse;
                vpn_old = i;
            }
        }    
        printf("replace page %d\n", vpn_old);
        ppn = machine->pageTable[vpn_old].physicalPage;
        // disable the old one, if modified, write back
        machine->pageTable[vpn_old].valid = FALSE;
        if(machine->pageTable[vpn_old].dirty == TRUE){
            printf("write back\n");
            memcpy(&(currentThread->space->fakeDisk[vpn_old * PageSize]), &(machine->mainMemory[ppn * PageSize]), PageSize);
        }
        //ASSERT(FALSE);
    }
    machine->pageTable[vpn].physicalPage = ppn;
    machine->pageTable[vpn].valid = TRUE;
    machine->pageTable[vpn].dirty = FALSE;
    #endif
    #ifdef USE_INV
    int ppn = -1, i;
    for(i = 0; i < NumPhysPages; i++){
        if(machine->invertedTable[i].valid == FALSE){
            ppn = i;
            break;
        }
    }  
    if(ppn == -1){
        int t = stats->totalTicks + 1;
        for(i = 0; i < NumPhysPages; i++){
            if(machine->invertedTable[i].lastUse < t 
                && machine->invertedTable[i].threadID == currentThread->getThreadID()){
                t = machine->invertedTable[i].lastUse;
                ppn = i;
            }
        }
        if(machine->invertedTable[ppn].dirty == TRUE){
            int vpn_old = machine->invertedTable[ppn].virtualPage;
            memcpy(&(currentThread->space->fakeDisk[vpn_old * PageSize]), &(machine->mainMemory[ppn * PageSize]), PageSize); 
        }
    }
    machine->invertedTable[ppn].virtualPage = vpn;
    machine->invertedTable[ppn].valid = TRUE;
    machine->invertedTable[ppn].threadID = currentThread->getThreadID();

    #endif
    //load into memory
    int paddr = ppn * PageSize;
    memcpy(&(machine->mainMemory[paddr]), &(currentThread->space->fakeDisk[vpn * PageSize]), PageSize);

}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    //printf("which exception: %d\n", which);
    if ((which == SyscallException) && (type == SC_Halt)) {
	    DEBUG('a', "Shutdown, initiated by user program.\n");
        currentThread->space->cleanBitMap();
   	    interrupt->Halt();
    } 
    /* add in lab4 ex2 */
    /* add in lab4 ex6 */
    else if(which == PageFaultException){
        /*
        if(machine->tlb != NULL){ //TLB miss
            tlbMissHandler();
        }
        else{
            //printf("should not reach there\n");
            pageFaultHandler();
        }
        */
        pageFaultHandler();
    }
    else if (which == TLBMissException){
        tlbMissHandler();
    }
    /* end add */
    else {
	    printf("Unexpected user mode exception %d %d\n", which, type);
	    ASSERT(FALSE);
    }
}
