// addrspace.h
//	Data structures to keep track of executing user programs
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "noff.h"

#define UserStackSize 1024 // increase this as necessary!

typedef int PageFlags;
/**
 * @brief Flags used to set TranslationEntry
 *
 */
enum PageFlag
{
  VALID = 0b0001,
  READ_ONLY = 0b0010,
  USE = 0b0100,
  DIRTY = 0b1000,
};

class AddrSpace
{
public:
  AddrSpace();  // Create an address space.
  ~AddrSpace(); // De-allocate an address space

  bool Load(char *fileName); // Load a program into addr space from
                             // a file
                             // return false if not found

  void Execute(char *fileName); // Run a program
                                // assumes the program has already
                                // been loaded

  void SaveState();    // Save/restore address space-specific
  void RestoreState(); // info on a context switch

  // Translate virtual address _vaddr_
  // to physical address _paddr_. _mode_
  // is 0 for Read, 1 for Write.
  ExceptionType Translate(unsigned int vaddr, unsigned int *paddr, int mode);

private:
  TranslationEntry *pageTable; // Assume linear page table translation
                               // for now!
  unsigned int numPages;       // Number of pages in the virtual
                               // address space

  void InitRegisters(); // Initialize user-level CPU registers,
                        // before jumping to user code

  /**
   * @brief Allocate free frames and load the segment into the page table
   *
   * @param segmentName For debug
   * @param segment
   * @param executable The user program to be loaded. NULL if we don't need to load it from file.
   * @param flags Used to set TranslationEntry
   * @param remaining Remaining space of the last page
   */
  void AllocateAndLoad(char const *segmentName, Segment &segment, OpenFile *executable, PageFlags flags, int &remaining);
};

#endif // ADDRSPACE_H
