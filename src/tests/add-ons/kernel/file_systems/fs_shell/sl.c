/* This file contains a heavily hacked and generalized version of the
   Example skiplist code distributed on mimsy.cs.umd.edu.

  Here is a short excerpt from the original comment :

     Example of Skip List source code for C :

       Skip Lists are a probabilistic alternative to balanced trees,
     as described in the June 1990 issue of CACM and were invented by
     William Pugh in 1987.

 These are my additions :

     This file contains my (Dominic Giampaolo's) heavily hacked version
   of skip lists.  These work on any arbitrary data by using callback
   functions which you supply (at list creation time) to do the data
   comparisons.  You could instantly use this package to implement a
   symbol table for a compiler which would be blazingly fast and
   require zippo effort on your part.

     I've changed the function names (not to protect the innocent, but
   to make them easier to read :) and changed the data structures a bit.
   I've ansi'fied the code, added prototypes, and changed all those ugly
   do/while's to for loops.  I also removed the dependance on those silly
   NIL items at the end of the list (it just checks for regular NULL
   pointers instead).  Additionally, the code is more easily reentrant now,
   and doesn't depend on any global variables.  The code is quite a bit
   different looking than it originally was, but the underlying algorithims
   (of course) remain unchanged.

  
   THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED 
   OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
   NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.
 
   FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

   Dominic Giampaolo
   dbg@be.com
*/
#include <stdio.h>
#include <stdlib.h>
#include "skiplist.h"

/* define's */
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MaxNumberOfLevels 16
#define MaxLevel (MaxNumberOfLevels-1)
#define NewNodeOfLevel(x) (SLNode)malloc(sizeof(struct SLNodeStruct)+(x)*sizeof(SLNode *))


/* private proto */
static int RandomLevelSL(SkipList l);


/* functions */
SkipList  NewSL(int (*compare)(), void (*freeitem)(), int flags)
{
  SkipList l;
  int i;

  if (compare == NULL)    /* need at least a compare function... */
    return NULL;

  l = (SkipList)malloc(sizeof(struct _SkipList));
  if (l == NULL)
    return NULL;

  l->level = 1;
  l->header = NewNodeOfLevel(MaxNumberOfLevels);
  if (l->header == NULL)
    { free(l); return NULL; }

  for(i=0; i < MaxNumberOfLevels; i++)
    l->header->forward[i] = NULL;
  l->header->key = NULL;         /* just to be sure */

  /* XXXdbg -- don't want this! srand(time(NULL) | 0x01); */  /* seed with an odd number */

  l->compare     = compare;
  l->freeitem    = freeitem;
  l->flags   = flags;

  l->count = 0;

  return(l);
}


void FreeSL(SkipList l)
{
  register SLNode p,q;
  void (*freeitem)() = l->freeitem;

  if (l == NULL)
    return;

  if (l->header == NULL)
   {
     free(l);
     return;
   }


  p = l->header;       /* free header node first, because it doesn't */
  q = p->forward[0];       /* have a real key to it          */
  free(p);
  p = q;

  while (p != NULL)
   {
     q = p->forward[0];
     if (freeitem)
       (*freeitem)(p->key);
     free(p);
     p = q;
   }

  free(l);
}




/*
 *   This RandomLevelSL function generates a very good representation of
 *   p=.25 (or p=.5, etc).  The number of nodes of each level works out
 *   to be very very close to what they should be.  I didn't check it
 *   statistically,  but on large data sets, I imagine it's +/- 5% of what
 *   it should be.  This P value is good for lists of up to 64K elements.
 *   
 *   For more info about the P value, see the papers by Mr. Pugh (available
 *   in postscript from mimsy.umd.edu).
 */
#define P_50   (RAND_MAX / 2)     /* p value of .50   */
#define P_25   (RAND_MAX / 4)     /* p value of .25   */
#define P_125  (RAND_MAX / 8)     /* p value of .125  */

static int RandomLevelSL(SkipList l)
{
  register int level = 0;

  while(rand() < P_25)
    {
      level++;
    }

  return (level > MaxLevel ? MaxLevel : level);
}


int InsertSL(SkipList l, void *key)
{
  register int i,k;
  SLNode update[MaxNumberOfLevels];
  register SLNode p,q = 0;
  int (*compare)() = l->compare;

  p = l->header;
  
  for(k = l->level-1; k >= 0; k--)
   {
     while((q = p->forward[k]) && (*compare)(q->key, key) < 0)
    p = q;

     update[k] = p;
   }

  if ((l->flags & ALLOW_DUPLICATES) == FALSE) /* if no duplicates allowed */
   if (q && (*compare)(q->key, key) == 0)     /* item is a duplicate */
     {
       return DUPLICATE_ITEM;
     }


  k = RandomLevelSL(l);
  if (k >= l->level)
    {
      k = l->level;
      l->level++;
      update[k] = l->header;
    }

  q = NewNodeOfLevel(k);  /* was k+1 */

  if (q == NULL)
    return FALSE;

  l->count++;             /* update the number of nodes in the list */
  
  q->key = key;
  for(i=0; i < k; i++)
    q->forward[i] = NULL;

  for(; k >= 0; k--)
    {
      p = update[k];
      q->forward[k] = p->forward[k];
      p->forward[k] = q;
    }

  return TRUE;
}



int DeleteSL(SkipList l, void *key)
{
  register int k,m;
  SLNode update[MaxNumberOfLevels];
  register SLNode p,q;
  int  (*compare)()  = l->compare;
  void (*freeitem)() = l->freeitem;

  p = l->header;

  for(k=l->level-1; k >= 0; k--)
   {
     while((q = p->forward[k]) && (*compare)(q->key, key) < 0)
    p = q;

     update[k] = p;
   }
  q = p->forward[0];

  if (q && (*compare)(q->key, key) == 0)
    {
      m = l->level;
      for(k=0; k < m; k++)
    {
      p = update[k];
      if (p == NULL || p->forward[k] != q)
        break;
      p->forward[k] = q->forward[k];
    }

      l->count--;
      
      if (freeitem)
    (*freeitem)(q->key);

      free(q);

      m = l->level - 1;
      while(l->header->forward[m] == NULL && m > 0)
    m--;

      l->level = m + 1;
      return TRUE;
    }
  else
    return FALSE;
}


void *SearchSL(SkipList l, void *key)
{
  register int k;
  register SLNode p,q = 0;
  int (*compare)() = l->compare;

  p = l->header;
  
  for(k=l->level-1; k >= 0; k--)
   {
     while((q = p->forward[k]) && (*compare)(q->key, key) < 0)
    p = q;
   }

  if (q == NULL || (*compare)(q->key, key) != 0)
    return NULL;

  return q->key;
}


void DoForSL(SkipList l, int (*function)(), void *arg)
{
  register SLNode p, q, fix;
  register int k, ret;
  SLNode save[MaxNumberOfLevels], who[MaxNumberOfLevels];
  void (*freeitem)() = l->freeitem;
  

  if (l == NULL || l->header == NULL || function == NULL)
    return;

  p = l->header;       /* skip header node because it isn't a real node */

  /* Save the initial header info
   */
  for(k=0; k < l->level; k++)
   {
     save[k] = p->forward[k];
     who[k]  = p;
   }

  p = p->forward[0];      /* skip to the first data node */

  while (p != NULL)
   {
     q = p->forward[0];
     ret = (*function)(p->key, arg);

     if (ret & SL_DELETE)
      {
    k = 0;
    while(save[k] == p)
     {
       fix = who[k];
       fix->forward[k] = p->forward[k];
       save[k] = p->forward[k];
       k++;
     }

    l->count--;         /* decrement the count of items */
    
    if (freeitem)
      (*freeitem)(p->key, arg);
    free(p);
      }
     else
      {
    k = 0;
    while(save[k] == p)
     {
       save[k] = p->forward[k];
       who[k]  = p;
       k++;
     }
      }
     
     if (ret & SL_QUIT)
       break;

     p = q;    /* advance to the next one */
   }
}


void DoForRangeSL(SkipList l, void *key, int (*compare)(), int (*func)(),
          void *arg)
{
  register int k;
  SLNode update[MaxNumberOfLevels];
  register SLNode p,q;
  void (*freeitem)() = l->freeitem;
  int  ret;

  p = l->header;

  for(k=l->level-1; k >= 0; k--)
   {
     while((q = p->forward[k]) && (*compare)(q->key, key) < 0)
    p = q;

     update[k] = p;
   }
  p = p->forward[0];

  if (p == NULL || (*compare)(p->key, key) != 0)   /* then nothing matched */
    return;

  do
   {
     q = p->forward[0];            /* save next pointer */
     ret = (*func)(p->key, arg);
     
     if (ret & SL_DELETE)
      {
    for(k=0; k < l->level && update[k] && update[k]->forward[k] == p; k++)
      update[k]->forward[k] = p->forward[k];

    l->count--;         /* decrement the count of items */
    
    if (freeitem)
      (*freeitem)(p->key, arg);
    free(p);
      }
     
     if (ret & SL_QUIT)
       break;

     p = q;    /* advance to the next one */
   }
  while(p != NULL && (*compare)(p->key, key) == 0);

}


int NumInSL(SkipList l)
{
  return l->count;
}

