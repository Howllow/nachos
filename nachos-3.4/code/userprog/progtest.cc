// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------
void
NewProg(char* filename)
{
    
    OpenFile *executable = fileSystem->Open(filename);
    if (executable == NULL) {
    printf("Unable to open file %s\n", filename);
    return;
    }
    AddrSpace *space;
    printf("Initializing space for Program:%s!!\n", currentThread->getName());
    space = new AddrSpace(executable);
    /*
    scheduler->Print();
    currentThread->space = space;
    */
    space->InitRegisters();

    //space->RestoreState();
    printf("Program: %s start running!!\n", currentThread->getName());
    machine->Run();

}
void
StartProcess(char *filename)
{
    /*
    OpenFile *executable = fileSystem->Open(filename);
    OpenFile *executable1 = fileSystem->Open(filename);
    if (executable == NULL) {
    printf("Unable to open file %s!\n", filename);
    return;
    }
    
    AddrSpace *space1;
    AddrSpace *space2;
    printf("Initializing space for Thread:test1!!\n");
    space1 = new AddrSpace(executable);
    printf("Initializing space for Thread:test2!!\n");
    space2 = new AddrSpace(executable1);
    
    space2->InitRegisters();
    space2->RestoreState();
    test1->Fork(NewProg, 1);
    currentThread->Yield();
    space1->InitRegisters();
    space1->RestoreState();
    printf("Thread: %s start running!!\n", currentThread->getName());
    machine->Run();
    ASSERT(FALSE);
    */
    Thread* test1;
    Thread* test2;
    test1 = new Thread("test1");
    test2 = new Thread("test2");
    test1->Fork(NewProg, "../test/halt");
    test2->Fork(NewProg, filename);
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}
