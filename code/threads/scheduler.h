// scheduler.h
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

// thread priority
#define PRI_L3_MIN 0
#define PRI_L2_MIN 50
#define PRI_L1_MIN 100
#define PRI_L1_MAX 149

int CompareSjf(Thread *a, Thread *b);
int ComparePriority(Thread *a, Thread *b);

// The following class defines the scheduler/dispatcher abstraction --
// the data structures and operations needed to keep track of which
// thread is running, and which threads are ready but not running.
class Scheduler
{
public:
  // Initialize list of ready threads
  Scheduler();
  // De-allocate ready list
  ~Scheduler();
  // Thread can be dispatched.
  void ReadyToRun(Thread *thread);
  // Dequeue first thread on the ready list, if any, and return thread.
  Thread *FindNextToRun();
  // Cause nextThread to start running
  void Run(Thread *nextThread, bool finishing);
  // Check if thread that had been running needs to be deleted
  void CheckToBeDestroyed();
  // Print contents of ready list
  void Print();
  // increse the thread priority by 10 (max 149) when the thread stays in ready state more than 1500 ticks
  void aging(const int totalTicks);
  // if the currentThread is preempted
  bool isPreempted(const Thread *currentThread, const int totalTicks) const;
  // if the current L3 thread has run more than 100 ticks and L3 queue is not empty, return true
  bool shouldDoRoundRobin(const Thread *currentThread, const int totalTicks) const;
  // SelfTest for scheduler is implemented in class Thread

private:
  // SJF
  SortedList<Thread *> *readyList1;
  // non-preemptive
  SortedList<Thread *> *readyList2;
  // round-robin
  List<Thread *> *readyList3;

  // finishing thread to be destroyed by the next thread that runs
  Thread *toBeDestroyed;
  /**
   * @brief the corresponding ready queue level of this thread
   *
   * @param thread
   * @return int 1, 2 or 3
   */
  int qLv(const Thread *thread) const;
  void pushToQ(Thread *thread);
  void aging(const int totalTicks, List<Thread *> *readyList, const int readyListLevel);
  void popFromQMsg(Thread *thread, const int q);
};

#endif // SCHEDULER_H
