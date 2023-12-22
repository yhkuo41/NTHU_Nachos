// scheduler.cc
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would
//	end up calling FindNextToRun(), and that would put us in an
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{
    readyList1 = new SortedList<Thread *>(CompareSfj);
    readyList2 = new SortedList<Thread *>(ComparePriority);
    readyList3 = new List<Thread *>;
    toBeDestroyed = NULL;
}

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{
    delete readyList1;
    delete readyList2;
    delete readyList3;
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void Scheduler::ReadyToRun(Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());

    thread->setStatus(READY, kernel->stats->totalTicks);
    pushToQ(thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *Scheduler::FindNextToRun()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    Thread *next = NULL;
    int q;
    if (!readyList1->IsEmpty())
    {
        q = 1;
        next = readyList1->RemoveFront();
    }
    else if (!readyList2->IsEmpty())
    {
        q = 2;
        next = readyList2->RemoveFront();
    }
    else if (!readyList3->IsEmpty())
    {
        q = 3;
        next = readyList3->RemoveFront();
    }
    if (next != NULL)
    {
        popFromQMsg(next, q);
    }
    return next;
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void Scheduler::Run(Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;

    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing)
    { // mark that we need to delete current thread
        ASSERT(toBeDestroyed == NULL);
        toBeDestroyed = oldThread;
    }

    if (oldThread->space != NULL)
    {                               // if this thread is a user program,
        oldThread->SaveUserState(); // save the user's CPU registers
        oldThread->space->SaveState();
    }

    oldThread->CheckOverflow(); // check if the old thread
                                // had an undetected stack overflow

    kernel->currentThread = nextThread;                        // switch to the next thread
    nextThread->setStatus(RUNNING, kernel->stats->totalTicks); // nextThread is now running

    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    DEBUG(dbgBeta, "[E] Tick [" << kernel->stats->totalTicks << "]: Thread [" << nextThread->getID() << " " << nextThread->getName()
                                << "] is now selected for execution, thread [" << oldThread->getID() << " " << oldThread->getName()
                                << "] is replaced, and it has executed [" << oldThread->ts->getPreRunningTicks() << "] ticks");
    DEBUG(dbgTs, "[E] Tick [" << kernel->stats->totalTicks << "]: Thread [" << nextThread->getID()
                              << "] is now selected for execution, thread [" << oldThread->getID()
                              << "] is replaced, and it has executed [" << oldThread->ts->getPreRunningTicks() << "] ticks");
    // This is a machine-dependent assembly language routine defined
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread

    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed(); // check if thread we were running
                          // before this one has finished
                          // and needs to be cleaned up

    if (oldThread->space != NULL)
    {                                  // if there is an address space
        oldThread->RestoreUserState(); // to restore, do it.
        oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL)
    {
        delete toBeDestroyed;
        toBeDestroyed = NULL;
    }
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList1->Apply(ThreadPrint);
    readyList2->Apply(ThreadPrint);
    readyList3->Apply(ThreadPrint);
}

int Scheduler::qLv(const Thread *thread) const
{
    int p = thread->getPriority();
    if (PRI_L1_MIN <= p && p <= PRI_L1_MAX)
    {
        return 1;
    }
    else if (PRI_L2_MIN <= p)
    {
        return 2;
    }
    else
    {
        return 3;
    }
}

void Scheduler::pushToQ(Thread *thread)
{
    int q = qLv(thread);
    ASSERT(1 <= q && q <= 3);
    DEBUG(dbgBeta, "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << " " << thread->getName() << "] is inserted into queue L[" << q << "]");
    DEBUG(dbgTs, "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[" << q << "]");
    if (q == 3)
    {
        readyList3->Append(thread);
    }
    else if (q == 2)
    {
        readyList2->Insert(thread);
    }
    else
    {
        readyList1->Insert(thread);
    }
}

void Scheduler::aging(const int totalTicks)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    aging(totalTicks, this->readyList1, 1);
    aging(totalTicks, this->readyList2, 2);
    aging(totalTicks, this->readyList3, 3);
    // check the implementation of aging
    // Print();
    readyList1->SanityCheck();
    readyList2->SanityCheck();
    readyList3->SanityCheck();
}

void Scheduler::aging(const int totalTicks, List<Thread *> *readyList, const int readyListLevel)
{
    ListIterator<Thread *> *it = new ListIterator<Thread *>(readyList);
    Thread *to_delete, *t;
    int waitingTime, oldPriority, newPriority, q;
    while (true)
    {
        if (to_delete) // delay the deletion to avoid null pointer error
        {
            readyList->Remove(to_delete);
            popFromQMsg(to_delete, readyListLevel);
            pushToQ(to_delete);
            to_delete = NULL;
        }
        if (it->IsDone())
        {
            break;
        }
        t = it->Item();
        waitingTime = totalTicks - t->ts->readyStartTime;
        q = readyListLevel;
        if (waitingTime > 1500)
        {
            oldPriority = t->getPriority();
            newPriority = min(oldPriority + 10, PRI_L1_MAX);
            if (oldPriority != newPriority)
            {
                t->setPriority(newPriority);
                t->ts->readyStartTime = totalTicks; // reset the waiting time
                DEBUG(dbgBeta, "[C] Tick [" << totalTicks << "]: Thread [" << t->getID() << " " << t->getName() << "] changes its priority from [" << oldPriority << "] to [" << newPriority << "]");
                DEBUG(dbgTs, "[C] Tick [" << totalTicks << "]: Thread [" << t->getID() << "] changes its priority from [" << oldPriority << "] to [" << newPriority << "]");
                if (qLv(t) != readyListLevel || readyListLevel == 2)
                { // Remove and append the thread again to make sure the ordering is correct. If a thread has elevated priority in L2,
                    // we must reorder the thread, since L2 is sorted by priority.
                    to_delete = t;
                }
            }
        }
        it->Next();
    }
    delete it;
}

void Scheduler::popFromQMsg(Thread *thread, const int q)
{
    DEBUG(dbgBeta, "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << " " << thread->getName() << "] is removed from queue L[" << q << "]");
    DEBUG(dbgTs, "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is removed from queue L[" << q << "]");
}

bool Scheduler::isPreempted(const Thread *currentThread, const int totalTicks) const
{
    if (currentThread->getStatus() != RUNNING)
    {
        return FALSE;
    }
    int q = qLv(currentThread);
    if (q == 3 && (!readyList1->IsEmpty() || !readyList2->IsEmpty()))
    {
        // L1 or L2 preempts L3
        DEBUG(dbgBeta, "Tick " << totalTicks << " current L3 thread " << currentThread->getID() << " " << currentThread->getName()
                               << " is preempted by other L1/L2 thread");
        return TRUE;
    }
    else if (q == 2 && !readyList1->IsEmpty())
    { // L1 preempts L2
        DEBUG(dbgBeta, "Tick " << totalTicks << " current L2 thread " << currentThread->getID() << " " << currentThread->getName()
                               << " is preempted by other L1 thread");
        return TRUE;
    }
    else if (!readyList1->IsEmpty() && readyList1->Front()->getRemainingCpuBurstTime() < currentThread->getRemainingCpuBurstTime())
    { // L1 preempts L1
        DEBUG(dbgBeta, "Tick " << totalTicks << " current L1 thread " << currentThread->getID() << " " << currentThread->getName()
                               << " (remaining " << currentThread->getRemainingCpuBurstTime() << ")"
                               << " is preempted by other L1 thread "
                               << readyList1->Front()->getID() << " " << readyList1->Front()->getName()
                               << " (remaining " << readyList1->Front()->getRemainingCpuBurstTime() << ")");
        return TRUE;
    }
    return FALSE;
}

bool Scheduler::shouldDoRoundRobin(const Thread *currentThread, const int totalTicks) const
{
    if (currentThread->getPriority() < PRI_L2_MIN && !readyList3->IsEmpty())
    {
        // the ticks passed in this running state
        int curRunningTicks = totalTicks - currentThread->ts->getRunningStartTime();
        ASSERT(curRunningTicks >= 0);
        if (curRunningTicks >= 100)
        {
            DEBUG(dbgBeta, "current L3 thread " << currentThread->getID() << " " << currentThread->getName()
                                                << " has run " << curRunningTicks << " ticks, yields on return");
            return TRUE;
        }
    }
    return FALSE;
}

int CompareSfj(Thread *a, Thread *b)
{
    double ra = a->getRemainingCpuBurstTime();
    double rb = b->getRemainingCpuBurstTime();
    if (ra < rb)
    {
        return -1;
    }
    else if (ra == rb)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

int ComparePriority(Thread *a, Thread *b)
{
    return b->getPriority() - a->getPriority();
}
