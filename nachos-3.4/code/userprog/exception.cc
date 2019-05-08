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
MyCreate()
{
	int fileaddr = machine->ReadRegister(4);
	int cnt = 0;
	int x;
	machine->ReadMem(fileaddr++, 1, &x);
	cnt++;
	while(x) {
		machine->ReadMem(fileaddr++, 1, &x);
		cnt++;
	} // get filename length
	char name[cnt];
	fileaddr -= cnt;
	for (int i = 0; i < cnt; i++) {
		machine->ReadMem(fileaddr + i, 1, &x);
		name[i] = (char)x;
	}
	fileSystem->Create(name, 256);
	printf("Thread %s Created %s\n", currentThread->getName(), name);
	int newPC = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PCReg, newPC); // forward the PC
	machine->WriteRegister(2, 0); // set return value
	return;
}

void
MyOpen()
{
	int fileaddr = machine->ReadRegister(4);
	int cnt = 0;
	int x;
	machine->ReadMem(fileaddr++, 1, &x);
	cnt++;
	while(x) {
		machine->ReadMem(fileaddr++, 1, &x);
		cnt++;
	} // get filename length
	char name[cnt];
	fileaddr -= cnt;
	for (int i = 0; i < cnt; i++) {
		machine->ReadMem(fileaddr + i, 1, &x);
		name[i] = (char)x;
	}

	OpenFile *file = fileSystem->Open(name);
	if (file == NULL) {
		printf("cannot open!\n");
	}

	int id = currentThread->filenum; // get id
	currentThread->filenum++; // increase filenum
	currentThread->filetable[id] = file; // set openfile pointer
	printf("Thread %s Open %s\n", currentThread->getName(), name);
	machine->WriteRegister(2, id); // set return value
	int newPC = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PCReg, newPC); // forward the PC
	return;
}

void
MyClose()
{
	int id = machine->ReadRegister(4);
	OpenFile *file = currentThread->filetable[id];
	if (file != NULL) {
		delete file;
		currentThread->filetable[id] = NULL;
		currentThread->filenum--;
	}
	printf("Thread %s Close a File, id is %d\n", currentThread->getName(), id);
	int newPC = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PCReg, newPC); // forward the PC
	machine->WriteRegister(2, 0); // set return value
	return;
}

void
MyRead()
{
	int buffer_addr = machine->ReadRegister(4);
	int size = machine->ReadRegister(5);
	int id = machine->ReadRegister(6);
	OpenFile *file = currentThread->filetable[id];
	if (file == NULL){
		printf("cannot open!\n");
	}
	char tmpbuf[size];
	int retval = file->Read(tmpbuf, size);
	printf("Thread %s Read a File, id is %d\n", currentThread->getName(), id);
	printf("Contents:%s\n", tmpbuf);
	int i = 0;
	while(i < size) {
		machine->WriteMem(buffer_addr + i, 1, int(tmpbuf[i]));
		i++;
	}
	int newPC = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PCReg, newPC); // forward the PC
	machine->WriteRegister(2, retval); // set return value
	return;
}

void
MyWrite()
{
	int buffer_addr = machine->ReadRegister(4);
	int size = machine->ReadRegister(5);
	int id = machine->ReadRegister(6);
	OpenFile *file = currentThread->filetable[id];
	if (file == NULL){
		printf("cannot open!\n");
	}
	char tmpbuf[size];
	int val;
	int i = 0;
	while(i < size) {
		machine->ReadMem(buffer_addr, 1, &val);
		tmpbuf[i++] = (char)val;
		buffer_addr++;
	}
	file->Write(tmpbuf, size);
	printf("Thread %s Write a File, id is %d\n", currentThread->getName(), id);
	int newPC = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PCReg, newPC); // forward the PC
	machine->WriteRegister(2, 0); // set return value
	return;
}

void
StartPro(char *name)
{
	OpenFile *ex = fileSystem->Open(name);
	AddrSpace *space = new AddrSpace(ex);
	currentThread->space = space;
	space->InitRegisters();
	space->RestoreState();
	printf("Thread %s is executing %s\n", currentThread->getName(), name);
	machine->Run();
}

void
MyExec()
{
	printf("Exec Call!\n");
	int fileaddr = machine->ReadRegister(4);
	int cnt = 0;
	int x;
	machine->ReadMem(fileaddr++, 1, &x);
	cnt++;
	while(x) {
		machine->ReadMem(fileaddr++, 1, &x);
		cnt++;
	} // get filename length
	char name[cnt];
	fileaddr -= cnt;
	for (int i = 0; i < cnt; i++) {
		machine->ReadMem(fileaddr + i, 1, &x);
		name[i] = (char)x;
	}

	Thread *execthread = new Thread("execThread");
	currentThread->children->Append(execthread);
	execthread->Fork(StartPro, name);
	machine->WriteRegister(2, int(execthread)); // set return value
	int newPC = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PCReg, newPC); // forward the PC
}

void
StartFork(int addr)
{
	printf("Thread %s start running!\n", currentThread->getName());
	machine->WriteRegister(PCReg, addr);
	machine->WriteRegister(NextPCReg, addr + 4);
	machine->Run();
}

void
MyFork()
{
	int addr = machine->ReadRegister(4);
	Thread *forkthread = new Thread("forkThread");
	AddrSpace *newspace = new AddrSpace(currentThread->space); // copy the space
	forkthread->space = newspace;
	forkthread->SaveUserState(); //initialize
	forkthread->Fork(StartFork, (void*)addr);
	machine->WriteRegister(2, 0); // set return value
	int newPC = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PCReg, newPC); // forward the PC
}

void
MyJoin()
{
	int spaceid = machine->ReadRegister(4);
	Thread *jointhread = (Thread*) spaceid;
	printf("Thread %s Join and Wait!\n", currentThread->getName());
	printf("%d\n", spaceid);
	while(currentThread->children->Find((void*)jointhread)) {
		currentThread->Yield();
	}
	printf("Thread %s End Waiting!\n", currentThread->getName());
	int newPC = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PCReg, newPC); // forward the PC
	machine->WriteRegister(2, 0); // set return value
}

void
MyExit()
{
	int newPC = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PCReg, newPC); // forward the PC
	printf("Exit!\n");
	currentThread->Finish();
}

void
MyYield()
{
	int newPC = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PCReg, newPC); // forward the PC
	printf("Thread %s Yield!\n", currentThread->getName());
	currentThread->Yield();
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } 
    else if ((which == SyscallException) && (type == SC_Create))
    {
    	MyCreate();
    }
    else if ((which == SyscallException) && (type == SC_Open))
    {
    	MyOpen();
    }
    else if ((which == SyscallException) && (type == SC_Close))
    {
    	MyClose();
    }
    else if ((which == SyscallException) && (type == SC_Read))
    {
    	MyRead();
    }
    else if ((which == SyscallException) && (type == SC_Write))
    {
    	MyWrite();
    }
    else if ((which == SyscallException) && (type == SC_Fork))
    {
    	MyFork();
    }
    else if ((which == SyscallException) && (type == SC_Join))
    {
    	MyJoin();
    }
    else if ((which == SyscallException) && (type == SC_Exit))
    {
    	MyExit();
    }
    else if ((which == SyscallException) && (type == SC_Yield))
    {
    	MyYield();
    }
    else if ((which == SyscallException) && (type == SC_Exec))
    {
    	MyExec();
    }
    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
