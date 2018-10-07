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
#include "elevatortest.h"

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
	    printf("*** thread name %s id %d user %d looped %d times\n", currentThread->getName(), 
            currentThread->getThreadID(), currentThread->getUserID(), num);
        currentThread->Yield();
    }
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

    Thread *t1 = new Thread("forked thread1");
    Thread *t2 = new Thread("forked thread2");

    t1->Fork(SimpleThread, (void*)t1->getThreadID());
    t2->Fork(SimpleThread, (void*)t2->getThreadID());

    SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest2
// 	Fork over 128 threads to see whether the error will be detected.
//  add in lab1 ex4
//----------------------------------------------------------------------
void ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest2");

    for(int i = 0; i <= MAX_THREAD_NUM; i++){
        Thread *t = new Thread("forked thread");
        printf("*** thread name %s id %d user %d \n", t->getName(), 
            t->getThreadID(), t->getUserID());
    }
}

//----------------------------------------------------------------------
// ThreadTest3
// 	Fork over 128 threads to see whether the error will be detected.
//  add in lab1 ex4
//----------------------------------------------------------------------
void foo(int which)
{
    which++; // pretend to do something
    //printf("I am thread %d\n",currentThread->getThreadID());
}

void ThreadTest3()
{
    DEBUG('t', "Entering ThreadTest3");
    
    for(int i = 0; i < 5; i++){
        Thread *t = new Thread("forked thread");
        t->Fork(foo, (void*)t->getThreadID());
    }
    ShowAllThreadStatus();
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
    default:
	printf("No test specified.\n");
	break;
    }
}

