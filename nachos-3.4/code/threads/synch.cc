// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) 
{
    name = debugName;
    mutex = new Semaphore("mutex", 1);
    lockedThread = NULL;
}

Lock::~Lock() 
{
    delete mutex;
}

bool Lock::isHeldByCurrentThread()
{
    return (currentThread == lockedThread);
}

void Lock::Acquire() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    mutex->P();
    lockedThread = currentThread;
    (void) interrupt->SetLevel(oldLevel);
}

void Lock::Release() 
{
    /*
    if (isHeldByCurrentThread == FALSE) {
        printf("Release false; Thread %s hasn't held the lock~\n", currentThread->getName());
    }*/
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    //ASSERT(isHeldByCurrentThread() == TRUE);
    mutex->V();
    lockedThread = NULL;
    (void) interrupt->SetLevel(oldLevel);
}

Condition::Condition(char* debugName, Lock* lock = NULL) 
{
    name = debugName;
    condLock = lock;
    condList = new List();
}

Condition::~Condition()
{
    delete condList;
}

void
Condition::Wait(Lock* conditionLock)
{ 
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(conditionLock == condLock);
    ASSERT(condLock->isHeldByCurrentThread() == TRUE);
    condLock->Release();
    condList->Append(currentThread);
    currentThread->Sleep();
    condLock->Acquire();
    interrupt->SetLevel(oldLevel);
}

void 
Condition::Signal(Lock* conditionLock)
{ 
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(conditionLock == condLock); 
    ASSERT(condLock->isHeldByCurrentThread() == TRUE);
    Thread* wokenThread = condList->Remove();
    if (wokenThread != NULL)
        scheduler->ReadyToRun(wokenThread);
    interrupt->SetLevel(oldLevel);
}


void
Ready(Thread *thread)
{
    scheduler->ReadyToRun(thread);
}

void 
Condition::Broadcast(Lock* conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(conditionLock == condLock); 
    ASSERT(condLock->isHeldByCurrentThread() == TRUE);
    condList->Mapcar(Ready);
    condList = new List();
    interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Implemention of Barrier
//----------------------------------------------------------------------
Barrier::Barrier(char* debugName, int tnum)
{
    needed_num = tnum;
    name = debugName;
    broken = false;
    now_num = 0;
    lock = new Lock(name);
    cond = new Condition(name, lock);
}

Barrier::~Barrier()
{
    delete lock;
    delete cond;
}

void
Barrier::useBarrier()
{
    lock->Acquire();
    now_num++;
    if (now_num == needed_num) {
        broken = true;
        cond->Broadcast(lock);
    }
    while (!broken) {
        cond->Wait(lock);
    }
    now_num--;
    if (!now_num) {
        broken = false;
    }
    lock->Release();
}

//----------------------------------------------------------------------
// Implemention of RwLock
//----------------------------------------------------------------------
RwLock::RwLock(char* debugName)
{
    name = debugName;
    buf_mutex = new Lock("buf_mutex");
    read_mutex = new Lock("read_mutex");
    readers = 0;
}

RwLock::~RwLock()
{
    delete buf_mutex;
    delete read_mutex;
}

void
RwLock::ReadAcquire()
{
    read_mutex->Acquire();
    if (readers == 0) {
        buf_mutex->Acquire();
        //printf("%s gets the Buffer Lock.\n", currentThread->getName());
    }
    //printf("%s gets the Read Lock.\n", currentThread->getName());
    readers++;
    read_mutex->Release();
}

void
RwLock::ReadRelease()
{
    read_mutex->Acquire();
   // printf("%s releases the Read Lock.\n", currentThread->getName());
    readers--;
    if (readers == 0) {
        buf_mutex->Release();
        //printf("%s releases the Buffer Lock.\n", currentThread->getName());
    }
    read_mutex->Release();
}

void
RwLock::WriteAcquire()
{
    buf_mutex->Acquire();
    writers++;
    //printf("%s gets the Buffer Lock.\n", currentThread->getName());
}

void
RwLock::WriteRelease()
{
    writers--;
    //printf("%s releases the Buffer Lock.\n", currentThread->getName());
    buf_mutex->Release();
}