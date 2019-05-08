// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"
#include "elevatortest.h"
#include "synchlist.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// TS
// Print the info of all created threads
//----------------------------------------------------------------------

void
TS()
{
    for (int i = 0; i < 128; i++) {
        if (tp_array[i] != NULL) {
            tp_array[i] -> Print();
        }
    }

    return;
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, (void*)1);
    SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest2
// Create 129 Threads,
// call TS() to show threads' information
//----------------------------------------------------------------------
void
DumbThread() {
    currentThread -> Yield();
}

void
ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest2");
    printf("Entering ThreadTest2\n");
    for(int i = 0; i < 129; i++) {
        if (i == 127) {
            TS();
        }
        Thread *t = new Thread("forked thread");
        t->Fork(DumbThread, (void*)1);
    }
}

//----------------------------------------------------------------------
// ThreadTest3
// Create 5 threads
// priorities of which are 8, 5, 1, 9, 0
//----------------------------------------------------------------------
void
DumbThread2(int which) 
{
    for (int num = 0; num < 3; num++) {
        printf("*** thread %d looped %d times, Prio: %d\n", which, num, currentThread->getPrio());
        currentThread->Yield();
    }
}

void
ThreadTest3()
{
    DEBUG('t', "Entering ThreadTest3");
    printf("Entering ThreadTest3\n");
    Thread *t1 = new Thread("Prio8");
    Thread *t2 = new Thread("Prio5");
    Thread *t3 = new Thread("Prio1");
    Thread *t4 = new Thread("Prio9");
    Thread *t5 = new Thread("Prio0");
    t1->setPrio(8);
    t2->setPrio(5);
    t3->setPrio(1);
    t4->setPrio(9);
    t5->setPrio(0);
    t1->Fork(DumbThread2, (void*)8);
    t2->Fork(DumbThread2, (void*)5);
    t3->Fork(DumbThread2, (void*)1);
    t4->Fork(DumbThread2, (void*)9);
    t5->Fork(DumbThread2, (void*)0);
}
//----------------------------------------------------------------------
// ThreadTest4
// Create 5 threads
// Used for RR test
//----------------------------------------------------------------------
void
OnOffInt(int which)
{
    for (int i = 0; i < 7; i++) {
        interrupt->SetLevel(IntOn);
        interrupt->SetLevel(IntOff);
        printf("*** thread %d looped %d times, Prio: %d, Counter: %d\n", which, i, currentThread->getPrio(), currentThread->getCounter());
    }
}

void
ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest4");
    printf("Entering ThreadTest4\n");

    Thread *t1 = new Thread("Prio8");
    Thread *t2 = new Thread("Prio5");
    Thread *t3 = new Thread("Prio1");
    t1->setPrio(8);
    t2->setPrio(5);
    t3->setPrio(1);
    t1->Fork(OnOffInt, (void*)8);
    t2->Fork(OnOffInt, (void*)5);
    t3->Fork(OnOffInt, (void*)1);
}
//----------------------------------------------------------------------
//  pcSem
//  producer and consumer model
//  implemented by semaphore
//----------------------------------------------------------------------
int buf = 0; // buffer is empty at first
int bufsize = 5; // buffer size
Semaphore* mutex;
Semaphore* full;
Semaphore* empty;
void ProSem(int num);
void ConSem(int num);
void pcSem()
{
    mutex = new Semaphore("mutex", 1);
    empty = new Semaphore("empty", bufsize);
    full  = new Semaphore("full", 0);
    Thread* Pro;
    Thread* Con;
    Pro = new Thread("producer");
    Con = new Thread("consumer");
    Con->Fork(ConSem, (void*)10);
    Pro->Fork(ProSem, (void*)10);
}

void
ProSem(int num)
{
    for (int i = 0; i < num; i++) {
        empty->P();
        mutex->P();
        printf("Producer produce item in %d\n", buf);
        buf++;
        mutex->V();
        full->V();
    }
}

void
ConSem(int num)
{
    for (int i = 0; i < num; i++) {
        full->P();
        mutex->P();
        buf--;
        printf("Consumer consume item in %d\n", buf);
        mutex->V();
        empty->V();
    }
}
//----------------------------------------------------------------------
//  pcCon
//  producer and consumer model
//  implemented by conditional variables
//----------------------------------------------------------------------
Condition* notempty;
Condition* notfull;
Lock* alock;
void ProCon(int num);
void ConCon(int num);

void 
pcCon()
{
    alock = new Lock("alock");
    notempty = new Condition("notempty", alock);
    notfull = new Condition("notfull", alock);
    Thread* Pro;
    Thread* Con;
    Pro = new Thread("producer");
    Con = new Thread("consumer");
    Con->Fork(ConCon, (void*) 10);
    Pro->Fork(ProCon, (void*) 10);
}

void
ProCon(int num)
{
    for (int i = 0; i < num; i++) {
        alock->Acquire();
        while(buf >= bufsize) notfull->Wait(alock);
        printf("Producer produce item in %d\n", buf);
        buf++;
        notempty->Signal(alock);
        alock->Release();
    }
}

void
ConCon(int num)
{
    for (int i = 0; i < num; i++) {
        alock->Acquire();
        while(buf == 0) notempty->Wait(alock);
        buf--;
        printf("Consumer consume item in %d\n", buf);
        notfull->Signal(alock);
        alock->Release();
    }
}

//----------------------------------------------------------------------
// BarTest
// Test for Barrier
//----------------------------------------------------------------------
Barrier* bar;

void 
barFunc(int num)
{
    for (int i = 0; i < num; i++) {
        printf("Thread %s is in PHASE %d.\n", currentThread->getName(), i);
        currentThread->Yield();
    }
    bar->useBarrier();
    printf("Thread %s has gone through Barrier %s.\n", currentThread->getName(), bar->getName());
    for (int i = num; i < 2 * num; i++) {
        printf("Thread %s is in PHASE %d.\n", currentThread->getName(), i);
        currentThread->Yield();
    }
    bar->useBarrier();
    printf("Thread %s has gone through Barrier %s.\n", currentThread->getName(), bar->getName());
}

void
BarTest()
{
    bar = new Barrier("MyBarrier", 3);
    Thread* t1;
    Thread* t2;
    Thread* t3;
    t1 = new Thread("BTest1");
    t2 = new Thread("BTest2");
    t3 = new Thread("BTest3");
    t1->Fork(barFunc, 2);
    t2->Fork(barFunc, 2);
    t3->Fork(barFunc, 2);
}

//----------------------------------------------------------------------
// RwTest
// Test for RwLock
//----------------------------------------------------------------------
RwLock* rwlock;

void
Reader(int num)
{
    rwlock->ReadAcquire();
    for (int i = 0; i < num; i++) {
        printf("%s is reading.\n", currentThread->getName());
        currentThread->Yield();
    }
    rwlock->ReadRelease();
}

void
Writer(int num)
{
    rwlock->WriteAcquire();
    for (int i = 0; i < num; i++) {
        printf("%s is writing.\n", currentThread->getName());
        currentThread->Yield();
    }
    rwlock->WriteRelease();
}

void
RwTest()
{
    rwlock = new RwLock("myRwLock");
    Thread* r1;
    Thread* r2;
    Thread* r3;
    Thread* w1;
    Thread* w2;
    r1 = new Thread("Reader1");
    r2 = new Thread("Reader2");
    r3 = new Thread("Reader3");
    w1 = new Thread("Writer1");
    w2 = new Thread("Writer2");
    w1->Fork(Writer, 2);
    r1->Fork(Reader, 2);
    r2->Fork(Reader, 2);
    w2->Fork(Writer, 2);
    r3->Fork(Reader, 2);
}

void S(int w)
{
    for (int i = 0; i < 100; i++) {
        printf("i'm %s \n", currentThread->getName());
        interrupt->OneTick();
    }
}
void ss()
{
    Thread* t = new Thread('kitty');
    t->Fork(S, 1);
    Thread *t1 = new Thread('helloworld');
    t1->Fork(S, 1);
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    case 2:
    ThreadTest2();
    break;
    case 3:
    ThreadTest3();
    break;
    case 4:
    ThreadTest4();
    break;
    case 5:
    pcSem();
    break;
    case 6:
    pcCon();
    break;
    case 7:
    BarTest();
    break;
    case 8:
    RwTest();
    break;
    case 9:
    ss();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}

