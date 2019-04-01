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
#include "machine.h"

//----------------------------------------------------------------------
// ExitHandler
// Function to deal with exit syscall
//----------------------------------------------------------------------
void
ExitHandler()
{
    if (!currentThread->MapCleared){
        currentThread->MapCleared = TRUE;
        printf("Program: %s  is Exiting!!\n", currentThread->getName());
        printf("TLBMiss: %d,  TLBHit: %d,  MissRate: %f\n", machine->tlbmiss, machine->tlbhit, 
                                    float(machine->tlbmiss) / ((machine->tlbmiss) + (machine->tlbhit)));
        for (int i = 0; i < machine->pageTableSize; i++) {
            if (machine->pageTable[i].valid) {
                machine->pageTable[i].valid = FALSE;
                machine->pageTable[i].use = FALSE;
                machine->pageTable[i].dirty = FALSE;
                machine->pageTable[i].readOnly = FALSE; 
                machine->MemoryMap->Clear(machine->pageTable[i].physicalPage);
                printf("Deallocate physical page %d for Thread: %s\n", machine->pageTable[i].physicalPage, currentThread->getName());
            }
        }
    }
    machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
    currentThread->Finish();
}

//----------------------------------------------------------------------
// TLBMissHandler
// Function to deal with tlb miss
// strat=0:FIFO; strat=1:LRU;
//----------------------------------------------------------------------
void
SortTLBList()
{
    TLBPos* tmp1 = machine->TLBList->Remove();
    TLBPos* tmp2 = machine->TLBList->Remove();
    TLBPos* tmp3 = machine->TLBList->Remove();
    TLBPos* tmp4 = machine->TLBList->Remove();
    machine->TLBList->SortedInsert(tmp1, tmp1->lru);
    machine->TLBList->SortedInsert(tmp2, tmp2->lru);
    machine->TLBList->SortedInsert(tmp3, tmp3->lru);
    machine->TLBList->SortedInsert(tmp4, tmp4->lru);
}
int
FindPos()
{
    for (int i = 0; i < TLBSize; i++) {
        if (machine->tlb[i].valid == FALSE)
            return i;
    }
    return -1;
}
void
TLBMissHandler(int strat)
{
    int vaddr = machine->registers[BadVAddrReg];
    unsigned int vpn = (unsigned) vaddr / PageSize;
    int index;
    if (machine->TLBList->NumInList() < TLBSize) {
        index = FindPos();
        ASSERT(index != -1);
        TLBPos* tmp = new TLBPos(index);
        if (strat) {
            machine->TLBList->Change(index);
        }
        machine->TLBList->Append(tmp);
    }
    else if (!strat) {
        TLBPos* tmp = machine->TLBList->Remove();
        index = tmp->index;
        //printf("%d\n", index);
        machine->TLBList->Append(tmp);
    }
    else {
        SortTLBList();
        //machine->TLBList->PrintL();
        TLBPos* tmp = machine->TLBList->Remove();
        index = tmp->index;
        tmp->lru = 0;
        machine->TLBList->Change(index);
        machine->TLBList->Append(tmp);
    }
    //printf("%d\n", index);
    machine->tlb[index].valid = TRUE;
    machine->tlb[index].virtualPage = vpn;
    machine->tlb[index].physicalPage = machine->pageTable[vpn].physicalPage;
    machine->tlb[index].use = FALSE;
    machine->tlb[index].dirty = FALSE;
    machine->tlb[index].readOnly = FALSE;
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

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } 
    else if ((which == SyscallException) && (type == SC_Exit)) {
        ExitHandler();
    }
    else if ((which == PageFaultException) && (machine->tlb != NULL)) {
    	TLBMissHandler(Strategy);
    }
    else if ((which == PageFaultException) && (machine->tlb == NULL)) {
        ASSERT(FALSE);
    }
    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
