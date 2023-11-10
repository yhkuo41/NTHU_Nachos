/* noff.h
 *     Data structures defining the Nachos Object Code Format
 *
 *     Basically, we only know about three types of segments:
 *	code (read-only), initialized data, and unitialized data
 */

// magic number denoting Nachos object code file
#define NOFFMAGIC 0xbadfad

typedef struct segment
{
   int virtualAddr; // location of segment in virt addr space
   int inFileAddr;  // location of segment in this file
   int size;        // size of segment
} Segment;

typedef struct noffHeader
{
   int noffMagic;    // should be NOFFMAGIC
   Segment code;     // executable code segment
   Segment initData; // initialized data segment, e.g. explicitly initialized global and static variables
#ifdef RDATA
   Segment readonlyData; // data with const keyword
#endif
   Segment uninitData; // uninitialized data segment (should be zero'ed before use), e.g. uninitialized or zero-initialized global and static variables in BSS
} NoffHeader;
