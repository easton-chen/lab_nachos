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
    //printf("page fault\n");
    unsigned int badVAddr = machine->ReadRegister(BadVAddrReg);
    //printf("bad vaddr:%u\n",badVAddr);
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
        //printf("replace page %d\n", vpn_old);
        ppn = machine->pageTable[vpn_old].physicalPage;
        // disable the old one, if modified, write back
        machine->pageTable[vpn_old].valid = FALSE;
        if(machine->pageTable[vpn_old].dirty == TRUE){
            //printf("write back\n");
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

//----------------------------
// setupProcess
// setup everything before a user programme could be executed
// add in lab6 ex4
//---------------------------------
void setupProcess(char* name){
    
    printf("filename:%s\n",name);
    OpenFile *executable = fileSystem->Open(name);
    
    AddrSpace *space = new AddrSpace(executable);
    
    currentThread->space = space;
    
    delete executable;
  
    space->InitRegisters();
    space->RestoreState();
    //printf("ready to run?\n");
    machine->Run();
}

void setupFork(int func){
    machine->WriteRegister(PCReg,func);
    machine->WriteRegister(NextPCReg, func+4);
    currentThread->SaveUserState();
    machine->Run();
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
    //IntStatus oldLevel = interrupt->SetLevel(IntOff);
    int type = machine->ReadRegister(2);
    //printf("which exception: %d\n", which);
    if (which == SyscallException) {
        if(type == SC_Halt){
            DEBUG('a', "Shutdown, initiated by user program.\n");
            //currentThread->space->cleanBitMap();
            interrupt->Halt();
        }
        else if(type == SC_Create){
            printf("SC_CREATE\n");
            int addr = machine->ReadRegister(4);
            char name[20];
            int pos = 0;
            int data;
            while(1){
                while(machine->ReadMem(addr + pos, 1, &data) == FALSE){

                }
                    //printf("f**k\n");
                if(data == 0){
                    name[pos] = '\0';
                    break;
                }
                name[pos++] = char(data);
                //printf("%d\n",data);
            }
            //printf("1%s\n",name);
            fileSystem->Create(name, SectorSize);
            machine->changePC();
        }
        else if(type == SC_Open){
            printf("SC_Open\n");
            int addr = machine->ReadRegister(4);
            char name[20];
            int pos = 0;
            int data;
            while(1){
                while(machine->ReadMem(addr + pos, 1, &data) == FALSE)
                {

                }
                    //printf("f**k\n");
                if(data == 0){
                    name[pos] = '\0';
                    break;
                }
                name[pos++] = char(data);
            }
            //printf("openfile name:%s\n",name);
            OpenFile *openfile = fileSystem->Open(name);
            //printf("openfileid:%d\n",int(openfile));
            machine->WriteRegister(2, int(openfile));
            machine->changePC();
        }
        else if (type == SC_Close){
            printf("SC_Close\n");
            int fd = machine->ReadRegister(4);
            OpenFile* openfile = (OpenFile*)fd;
            delete openfile;
            machine->changePC();
        }
        else if(type == SC_Read){
            printf("SC_READ\n");
            int bufferAddr = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fd = machine->ReadRegister(6);
            //printf("buffer:%x,size:%d,fd:%d\n",bufferAddr,size,fd);
            OpenFile *openfile = (OpenFile*)fd;
            char buf[size];
            int res = openfile->Read(buf, size);
            for(int i = 0; i < res; i++){
                while(machine->WriteMem(bufferAddr + i, 1, int(buf[i])) == FALSE){

                }
                    //printf("no!\n");
            }
            machine->WriteRegister(2, res);
            machine->changePC();
        }
        else if(type == SC_Write){
            printf("SC_Write\n");
            int bufferAddr = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fd = machine->ReadRegister(6);
            OpenFile *openfile = (OpenFile*)fd;
            char buf[size];
            int data;
            for(int i = 0; i < size; i++){
                while(machine->ReadMem(bufferAddr + i, 1, &data) == FALSE){

                }
                    //printf("no\n");
                buf[i] = char(data);
            }
            openfile->Write(buf, size);
            machine->changePC();
        }
        else if(type == SC_Exec){
            printf("SC_Exec\n");
            int addr =  machine->ReadRegister(4);
            char *name = new char[20];
            int pos = 0;
            int data;
            while(1){
                while(machine->ReadMem(addr + pos, 1, &data) == FALSE){

                }
                if(data == 0){
                    name[pos] = '\0';
                    break;
                }
                name[pos++] = char(data);
            }
            //printf("in SCEXEC %s\n",name);
            Thread *nt = new Thread("new thread");
            nt->Fork(setupProcess, name);
            machine->WriteRegister(2, nt->getThreadID());
            machine->changePC();
        }
        else if(type == SC_Fork){
            printf("SC_Fork\n");
            int func = machine->ReadRegister(4);
            //OpenFile *executable = fileSytem->Open(currentThread->filename);
            AddrSpace *space = currentThread->space;
            Thread *nt = new Thread("new thread");
            nt->space = space;
            nt->Fork(setupFork, (void*)func);
            machine->changePC();
        }
        else if(type == SC_Yield){
            printf("SC_Yield\n");
            machine->changePC();
            currentThread->Yield();
        }
        else if(type == SC_Join){
            printf("SC_Join\n");
            int tid = machine->ReadRegister(4);
            //printf("tid:%d tidbitmap[tid]:%d\n",tid,tidbitmap[tid]);
            while(tidbitmap[tid]){
                //printf("wait\n");
                currentThread->Yield();
            }
            //printf("joined?\n");
            machine->changePC();
        }
        else if(type == SC_Exit){
            printf("SC_Exit\n");
            int status = machine->ReadRegister(4);
            printf("exit status:%d\n",status);
            currentThread->space->cleanBitMap();
            //printf("1\n");
            machine->changePC();
            //printf("1\n");
            currentThread->Finish();
        }

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
    //(void) interrupt->SetLevel(oldLevel);
}
