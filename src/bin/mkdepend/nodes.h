/* $Id: nodes.h 1.5 Wed, 01 Mar 2000 21:15:40 -0700 lars $ */

/*---------------------------------------------------------------------------
** Copyright (c) 1995-2000 by Lars Düning. All Rights Reserved.
** This file is free software. For terms of use see the file LICENSE.
**---------------------------------------------------------------------------
*/

#ifndef __NODES_H__
#define __NODES_H__ 1

struct noderef;  /* forward */

/* Node structure to build the dependency tree
 */
typedef struct node
 {
   struct node    * pLeft, * pRight;  /* Tree pointers */
   struct node    * pNext;    /* List of Files to do; stack for traversals */

   char           * pName;
     /* Searchkey: Name of this file.
      * This is the minimal full pathname with which the file can be read.
      * For source files (and files to be avoided) it is identical with the name
      * given on the commandline.
      */
   char           * pBase;
     /* The filename part of the the file's pathname.
      * This is a pointer into pName.
      */
   char           * pPath;
     /* The directory part of the file's pathname.
      * Relative paths are interpreted depending on the type of the file:
      * source files are relative to the current working directory,
      * non-source files are relative to aIncl[.iInclude].
      */
   int              iInclude;
     /* Included non-sourcefiles: the index into aIncl[]/aSymbol[].
      * A negative index denotes an absolute filename, or one relative
      * to the cwd.
      */
   short            flags;    /* misc flags */
   struct noderef * pDeps;    /* What this file depends on */
   struct noderef * pUsers;   /* Who depends on this file */
   short            iStage;   /* Stage counter for tree traversals */
 }
Node;

/* Node.flags
 */
#define NODE_MARK   (1<<0)  /* Generic Node marker, used e.g. in tree walks */
#define NODE_NEW    (1<<1)  /* Node is unused */
#define NODE_SOURCE (1<<2)  /* Node is a skeleton source */
#define NODE_AVOID  (1<<3)  /* Node is not part of the skeleton sources */
#define NODE_NOTFND (1<<4)  /* Node describes a non-existant file */
#define NODE_DONE   (1<<5)  /* Node has been evaluated */
#define NODE_SYSTEM (1<<6)  /* File is an <>-include */
#define NODE_IGNORE (1<<7)  /* Data for this node shall not be printed */
#define NODE_SELECT (1<<8)  /* Marks selected include files */
#define NODE_ISELECT (1<<9) /* Marks nodes between source and selected
                             * include files */

/* Node marking
 */
#define NODES_MARK(node)   ((node)->flags |= NODE_MARK)
#define NODES_UNMARK(node) ((node)->flags &= (short)(~NODE_MARK))
#define NODES_MARKED(node) ((node)->flags & NODE_MARK)

/* Test if the _SYSTEM flag has a specific value
 */
#define NODES_SYSTEST(node,value) ( (node)->flags & NODE_SYSTEM ? (value) : !(value) )

/* Reference structure to treenodes
 */
typedef struct noderef
 {
   struct noderef * pNext;  /* next NodeRef */
   Node           * pNode;  /* referenced Node */
 }
NodeRef;

/* Prototypes */

extern Node *    nodes_findadd (const char *, int);
extern Node *    nodes_find (const char *);
extern int       nodes_addsource (char *, int);
extern int       nodes_depend (Node *, Node *);
extern int       nodes_mark_select (Node *);
extern Node    * nodes_todo (void);
extern void      nodes_initwalk (void);
extern Node    * nodes_inorder (void);
extern NodeRef * nodes_deplist (Node *, int, int);
extern void      nodes_freelist (NodeRef *);

#endif
