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
#include "synch.h"

#define N 10
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
    
    for (num = 0; num < 100; num++) {
	    printf("*** thread name %s id %d user %d looped %d times\n", currentThread->getName(), 
            currentThread->getThreadID(), currentThread->getUserID(), num);
        interrupt->SetLevel(IntOn);
        interrupt->SetLevel(IntOff);
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

    for(int i = 0; i < 5; i++){
        Thread *t = new Thread("forked thread");
        printf("*** thread name %s id %d user %d \n", t->getName(), 
            t->getThreadID(), t->getUserID());
    }
}

//----------------------------------------------------------------------
// ThreadTest3
// 	Fork over 5 threads and show the threads status
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
// ThreadTest4
// 	Fork over 5 threads and preempt
//  add in lab2 ex3
//----------------------------------------------------------------------
void foo1(int which)
{  
    printf("I am thread %d with priority %d\n",currentThread->getThreadID(), which);
}

void ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest4");

    for(int i = 0; i < 5; i++ ){
        Thread *t = new Thread("forked thread", 14 - i);
        printf("*** thread name %s id %d priority %d \n", t->getName(), 
            t->getThreadID(), t->getPriority());
        t->Fork(foo1, (void*)t->getPriority());
    }
}

//----------------------------------------------------------------------
// ThreadTestSync
// 	use semaphore and condition to solve producer-consumer problem
//  add in lab3 ex4
//----------------------------------------------------------------------
Semaphore* mutex = new Semaphore("mutex", 1);
Semaphore* empty = new Semaphore("empty", N);
Semaphore* full = new Semaphore("full", 0);
Condition* c = new Condition("condition");
Lock* arrayLock = new Lock("arraylock");
int sharedArray[N];
int arrayIndex = 0;

void Producer1(int num){
    while(num--){
        int product = 1; //pretend to produce
        empty->P();
        mutex->P();
        sharedArray[arrayIndex] = product;
        printf("producer put %d in no.%d\n", product, arrayIndex++);
        mutex->V();
        full->V();
    }

}

void Consumer1(int num){
    while(num--){
        int product;
        full->P();
        mutex->P();
        product = sharedArray[--arrayIndex];
        printf("consumer take %d in no.%d\n", product, arrayIndex);
        mutex->V();
        empty->V();
    }
}

void semaphoreTest(){
    Thread* tProducer = new Thread("Producer");
    Thread* tConsumer = new Thread("Consumer");
    tProducer->Fork(Producer1, (void*)12);
    tConsumer->Fork(Consumer1, (void*)15);
}

void Producer2(int num){
    while(num--){
        int product = 1; 
        arrayLock->Acquire();
        while(arrayIndex == N){
            c->Wait(arrayLock);
        }
        sharedArray[arrayIndex] = product;
        printf("producer put %d in no.%d\n", product, arrayIndex++);
        if(arrayIndex == 1)
            c->Signal(arrayLock);
        arrayLock->Release();    
    }
}

void Consumer2(int num){
    while(num--){
        int product;
        arrayLock->Acquire();
        while(arrayIndex == 0){
            c->Wait(arrayLock);
        }
        product = sharedArray[--arrayIndex];
        printf("consumer take %d in no.%d\n", product, arrayIndex);
        if(arrayIndex == N-1)
            c->Signal(arrayLock);
        arrayLock->Release();
    }
}

void conditionTest(){
    Thread* tProducer = new Thread("Producer");
    Thread* tConsumer = new Thread("Consumer");
    tProducer->Fork(Producer2, (void*)12);
    tConsumer->Fork(Consumer2, (void*)15);
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
    semaphoreTest();
    break;
    case 6:
    conditionTest();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}

