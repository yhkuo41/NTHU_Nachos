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

  // SelfTest for scheduler is implemented in class Thread

private:
  // queue of threads that are ready to run, but not running
  List<Thread *> *readyList;
  // finishing thread to be destroyed by the next thread that runs
  Thread *toBeDestroyed;
};

#endif // SCHEDULER_H
