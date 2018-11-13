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
    //unsigned int old_vpn = machine->tlb[i].virtualPage;
    //machine->pageTable[old_vpn].lastUse = machine->tlb[i].lastUse; 
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
    else if(which == PageFaultException){
        if(machine->tlb != NULL){ //TLB miss
            tlbMissHandler();
        }
        else{
            printf("should not reach there\n");
        }
    }
    /* end add */
    else {
	    printf("Unexpected user mode exception %d %d\n", which, type);
	    ASSERT(FALSE);
    }
}
