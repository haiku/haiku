/*
  This header file contains the definitions for use with the generic
  SkipList package.

  THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED 
  OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
  NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

  FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

  Dominic Giampaolo
  dbg@be.com
 */

#ifndef SKIPLIST_H
#define SKIPLIST_H


/* RAND_MAX should be defined if you are using an ANSI compiler system,
 * but alas it isn't always.  You should define it to be the correct
 * value for whatever your library rand() function returns.
 *
 * Under unix (mach, bsd, etc), that's 2^31 - 1.  On my Amiga at home 
 * it's 2^15 - 1.  It would be wise to verify what your compiler uses
 * for RAND_MAX (the maximum value returned from rand()) because otherwise
 * the code will _not_ work.
 */
#ifndef RAND_MAX
#define RAND_MAX (0x7fffffff)
#endif


#define ALLOW_DUPLICATES  1   /* allow or disallow duplicates in a list */
#define NO_DUPLICATES     0
#define DUPLICATE_ITEM   -1   /* ret val from InsertSL if dups not allowed */


/* typedef's */
typedef struct SLNodeStruct *SLNode;

struct SLNodeStruct
{
  void   *key;
  SLNode  forward[1]; /* variable sized array of forward pointers */
};

typedef struct _SkipList
{
  struct SLNodeStruct *header;     /* pointer to header */

  int  (*compare)();
  void (*freeitem)();

  int flags;
  int level;               /* max index+1 of the forward array */

  int count;                       /* number of elements in the list */
} *SkipList;



/* protos */
SkipList   NewSL(int (*compare)(), void (*freeitem)(), int flags);
void       FreeSL(SkipList l);
int    InsertSL(SkipList l, void *key);
int    DeleteSL(SkipList l, void *key);
void      *SearchSL(SkipList l, void *key);
void       DoForSL(SkipList  l, int (*function)(), void *arg);
void       DoForRangeSL(SkipList l, void *key, int (*compare)(),
            int (*func)(), void *arg);

int        NumInSL(SkipList l);


/* These defines are to be used as return values from the function
 * you pass to DoForSL().  They can be or'ed together to do multiple
 * things (like delete a node and then quit going through the list).
 */
#define  SL_CONTINUE  0x00
#define  SL_DELETE    0x01
#define  SL_QUIT      0x02

#endif  /* SKIPLIST_H */
