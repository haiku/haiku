/* $Id: nodes.c 1.6 Wed, 01 Mar 2000 21:15:40 -0700 lars $ */

/*---------------------------------------------------------------------------
 * Management of the dependency tree and nodes.
 *
 * Copyright (c) 1995-2000 by Lars Düning. All Rights Reserved.
 * This file is free software. For terms of use see the file LICENSE.
 *---------------------------------------------------------------------------
 * Each file (source or include) is associated one node which is kept in
 * a binary search tree with the name as index. Glued to each node is
 * a list of that nodes which files the master nodes includes.
 * Each node contains an extra node pointer the module uses to implement
 * tree-independant lists, namely the TODO list of files still to read
 * and a virtual stack for inorder tree traversals.
 *
 * Adding and retrieving the files to analyse is done using the functions
 *   nodes_addsource()
 *   nodes_depend()
 *   nodes_todo()
 *   nodes_select_mark()
 *
 * The output of the dependency tree is done inorder using
 *   nodes_initwalk()
 *   nodes_inorder()
 *
 * The list of dependencies for one single node is managed using
 *   nodes_deplist()
 *   nodes_freelist()
 *
 * Tree traversal and creation must not be mixed!
 *---------------------------------------------------------------------------
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "nodes.h"
#include "util.h"

/*-------------------------------------------------------------------------*/

/* Types for block allocation of Nodes and NodeRefs.
 * Note that Nodes are never freed.
 */

#define NBLOCKSIZE 16

typedef struct nodeblock
 {
  struct nodeblock * pNext;               /* Next nodeblock */
  int                iFree;               /* Number of first free node */
  Node               aNodes[NBLOCKSIZE];  /* Block of nodes */
 }
NodeBlock;

#define RBLOCKSIZE 16

typedef struct refblock
 {
  struct refblock * pNext;              /* Next refblock */
  int               iFree;              /* Number of first free noderef */
  NodeRef           aRefs[NBLOCKSIZE];  /* Block of noderefs */
 }
RefBlock;

/*-------------------------------------------------------------------------*/

static NodeBlock *pFreeNBlocks = NULL;  /* Nodeblocks */
static RefBlock  *pFreeRBlocks = NULL;  /* Refblocks */
static NodeRef   *pFreeRefs    = NULL;  /* List of free NodeRefs */
static Node      *pTree        = NULL;  /* Dependency tree */
static Node      *pList        = NULL;  /* List of (tree) nodes */
  /* The List is used as TODO list for the files to analyse as well as
   * stack simulation for the tree traversal.
   */

/*-------------------------------------------------------------------------*/
static Node *
nodes_newnode (void)

/* Allocate a new Node.
 *
 * Result:
 *   Pointer to the new Node, or NULL on error.
 *
 * The memory of the Node is cleared, except for .flags which is set
 * to NODE_NEW.
 */

{
  Node * pNode;

  if (!pFreeNBlocks || !pFreeNBlocks->iFree)
  {
    NodeBlock * pNewBlock;
    pNewBlock = (NodeBlock *)malloc(sizeof(*pNewBlock));
    if (!pNewBlock)
      return NULL;
    memset(pNewBlock, 0, sizeof(*pNewBlock));
    pNewBlock->iFree = NBLOCKSIZE;
    pNewBlock->pNext = pFreeNBlocks;
    pFreeNBlocks = pNewBlock;
  }
  pNode = &(pFreeNBlocks->aNodes[--pFreeNBlocks->iFree]);
  pNode->flags |= NODE_NEW;
  pNode->iInclude = -1;
  return pNode;
}

/*-------------------------------------------------------------------------*/
static NodeRef *
nodes_newref (void)

/* Allocate a new NodeRef.
 *
 * Result:
 *   Pointer to the new NodeRef, or NULL on error.
 *
 * The memory of the NodeRef is cleared.
 */

{
  NodeRef * pRef;

  if (pFreeRefs)
  {
    pRef = pFreeRefs;
    pFreeRefs = pRef->pNext;
  }
  else
  {
    if (!pFreeRBlocks || !pFreeRBlocks->iFree)
    {
      RefBlock * pNewBlock;
      pNewBlock = (RefBlock *)malloc(sizeof(*pNewBlock));
      if (!pNewBlock)
        return NULL;
      pNewBlock->iFree = NBLOCKSIZE;
      pNewBlock->pNext = pFreeRBlocks;
      pFreeRBlocks = pNewBlock;
    }
    pRef = &(pFreeRBlocks->aRefs[--pFreeRBlocks->iFree]);
  }
  memset(pRef, 0, sizeof(*pRef));
  return pRef;
}

/*-------------------------------------------------------------------------*/
Node *
nodes_findadd (const char *pName, int bAddToList)

/* Find the node associated with file *pName.
 * If necessary, add the node.
 *
 *   pName:      Pointer to the (minimalized) filename of the node.
 *   bAddToList: If true, a newly allocated node is also added to
 *               the TODO list.
 *
 * Result:
 *   Pointer to the node associated with this filename, or NULL on error.
 *
 * If the node has been added by this call, NODE_NEW is set in its .flags,
 * but it is empty otherwise.
 */

{
    Node * pThis, * pPrev;
    int    i;

    assert(pName);

    if (!pTree)
    {
        pTree = nodes_newnode();
        if (bAddToList)
        {
            pTree->pNext = pList;
            pList = pTree;
        }
        return pTree;
    }

    pPrev = NULL;
    pThis = pTree;
    while(pThis)
    {
        i = strcmp(pThis->pName, pName);
        if (!i)
            return pThis;
        pPrev = pThis;
        if (i > 0)
            pThis = pThis->pLeft;
        else
            pThis = pThis->pRight;
    }
    pThis = nodes_newnode();
    if (pThis)
    {
        if (i > 0)
            pPrev->pLeft = pThis;
        else
            pPrev->pRight = pThis;

        if (bAddToList)
        {
            pThis->pNext = pList;
            pList = pThis;
        }
    }
    return pThis;
}

/*-------------------------------------------------------------------------*/
Node *
nodes_find (const char *pName)

/* Find the node associated with file *pName.
 *
 *   pName:      Pointer to the (minimalized) filename of the node.
 *
 * Result:
 *   Pointer to the node associated with this filename, or NULL on error.
 */

{
    Node * pThis, * pPrev;
    int    i;

    assert(pName);

    if (!pTree)
      return NULL;

    pPrev = NULL;
    pThis = pTree;
    while(pThis)
    {
        i = strcmp(pThis->pName, pName);
        if (!i)
            return pThis;
        pPrev = pThis;
        if (i > 0)
            pThis = pThis->pLeft;
        else
            pThis = pThis->pRight;
    }
    return NULL;
}

/*-------------------------------------------------------------------------*/
int
nodes_addsource (char *pName, int bAvoid)

/* Add a source file to the dependency tree.
 *
 *   pName : Name of the file to add (it will be duplicated).
 *   bAvoid: TRUE if this is a file to be avoided.
 *
 * Result:
 *   0 on success, non-0 on error (out of memory).
 *
 * The file is also added to the internal TODO list of files if it is
 * a genuine source file.
 */

{
    Node *pNode;
    char *pBasename;

    assert(pName);

    pName = util_getpath(pName, &pBasename);
    if (!pName)
        return 1;
    pNode = nodes_findadd(pName, !bAvoid);
    if (!pNode)
        return 1;
    if (pNode->flags & NODE_NEW)
    {
        pNode->flags = NODE_SOURCE;
        pNode->pName = pName;
        pNode->pBase = pBasename;
        pNode->pPath = util_getpath(pName, NULL);
        if (!pNode->pPath)
            return 1;
    }
    else
        free(pName);

    if (bAvoid)
        pNode->flags |= NODE_AVOID;

    return 0;
}

/*-------------------------------------------------------------------------*/
int
nodes_depend (Node * pChild, Node * pParent)

/* Add node pChild to the list of dependees of pParent, vice versa add
 * pParent to the list of users of pName.
 *
 *  pChild : Node of the file included by pParent.
 *  pParent: Node which depends on pChild.
 *
 * Result:
 *   0 on success, non-0 on error (out of memory).
 *
 * The file is added to the dependee- and user-lists.
 */

{
  NodeRef * pRef;

  assert(pParent);
  assert(pChild);

  /* Add pChild to pParent->pDeps if not already there */
  for ( pRef = pParent->pDeps
      ; pRef && pRef->pNode != pChild
      ; pRef = pRef->pNext
      ) /* SKIP */;
  if (!pRef)
  {
    pRef = nodes_newref();
    if (!pRef)
      return 1;
    pRef->pNode = pChild;
    pRef->pNext = pParent->pDeps;
    pParent->pDeps = pRef;
  }

  /* Add pParent to pChild->pUsers if not already there */
  for ( pRef = pChild->pUsers
      ; pRef && pRef->pNode != pParent
      ; pRef = pRef->pNext
      ) /* SKIP */;
  if (!pRef)
  {
    pRef = nodes_newref();
    if (!pRef)
      return 1;
    pRef->pNode = pParent;
    pRef->pNext = pChild->pUsers;
    pChild->pUsers = pRef;
  }
  return 0;
}

/*-------------------------------------------------------------------------*/
Node *
nodes_todo (void)

/* Return the tree node of the next file to analyse.
 *
 * Result:
 *   Pointer to the node of the next file to analyse, or NULL if there is
 *   none.
 */

{
  Node * pNode;

  /* NODE_DONE should not happen, but checking it is free */
  while (pList && (pList->flags & (NODE_AVOID|NODE_DONE)))
    pList = pList->pNext;
  if (pList)
  {
    pNode = pList;
    pList = pList->pNext;
    pNode->flags |= NODE_DONE;
  }
  else
    pNode = NULL;
  return pNode;
}

/*-------------------------------------------------------------------------*/
int
nodes_mark_select (Node *pNode)

/* Check if this node, or one of its dependency nodes has been marked
 * as selected. If yes, mark pNode as well and return TRUE, otherwise
 * return FALSE.
 *
 * This is a recursive function. We avoid loops by temporarily marking
 * the node.
 */

{
    int rc;
    NodeRef * pRef;

    assert(pNode);
    
    if (NODES_MARKED(pNode))
        return FALSE;

    if (pNode->flags & NODE_ISELECT)
        return TRUE;

    NODES_MARK(pNode);
    /* First, recurse into the dependency nodes */
    rc = FALSE;
    for (pRef = pNode->pDeps; pRef != NULL; pRef = pRef->pNext)
    {
        if (nodes_mark_select(pRef->pNode))
            rc = TRUE;
    }

    if (rc && !(pNode->flags & NODE_SELECT))
    {
        pNode->flags |= NODE_ISELECT;
    }

    NODES_UNMARK(pNode);

    return rc || (pNode->flags & (NODE_SELECT|NODE_ISELECT));
}  /* nodes_mark_select() */

/*-------------------------------------------------------------------------*/
void
nodes_initwalk (void)

/* Initialise a tree traversal.
 */

{
  assert(!pList);
  pList = pTree;
  if (pList)
  {
    pList->iStage = 0;
    pList->pNext = NULL;
  }
}

/*-------------------------------------------------------------------------*/
Node *
nodes_inorder (void)

/* Return the next Node of an inorder tree traversal.
 *
 * Result:
 *   Pointer to the next node, or NULL if traversal is complete.
 */

{
  Node *pNode = NULL;
  if (!pList)
    return NULL;
  while (pList)
  {
    switch (pList->iStage)
    {
    case 0:
      pList->iStage++;
      pNode = pList->pLeft;
      break;
    case 1:
      pList->iStage++;
      return pList;
      break;
    case 2:
      pList->iStage++;
      pNode = pList->pRight;
      break;
    case 3:
      pList = pList->pNext;
      pNode = NULL;
      break;
    default:
      assert(0);
      break;
    }
    if (pNode)
    {
      pNode->pNext = pList;
      pList = pNode;
      pNode->iStage = 0;
    }
  }
  return NULL;
}

/*-------------------------------------------------------------------------*/
NodeRef *
nodes_deplist (Node * pNode, int bUsers, int bSelect)

/* Gather the list of dependees or users of *pNode.
 *
 *   pNode: the Node to get the list for.
 *   bUsers: TRUE: gather the list of users instead of the dependees.
 *   bSelect: TRUE: gather only files marked as NODE_SELECT.
 *
 * Result:
 *   Pointer to the list of dependees/users, or NULL if an error occurs.
 *   First entry in the list is pNode itself.
 */

{
  NodeRef *pDList;
  NodeRef *pMark;
  NodeRef *pThis;
  NodeRef *pRover;
  Node    *pDep;

  assert(pNode);
  pDList = nodes_newref();
  if (!pDList)
    return NULL;

  /* The list of dependencies is gathered in a wide search. */
  pDList->pNode = pNode;
  NODES_MARK(pNode);
  for ( pThis = pDList, pMark = pDList
      ; pMark
      ; pMark = pMark->pNext
      )
  {
    for ( pRover = (bUsers ? pMark->pNode->pUsers : pMark->pNode->pDeps)
        ; pRover
        ; pRover = pRover->pNext
        )
    {
      pDep = pRover->pNode;
      if (!NODES_MARKED(pDep))
      {
        pThis->pNext = nodes_newref();
        if (!pThis->pNext)
          return NULL;
        pThis = pThis->pNext;
        pThis->pNode = pDep;
        NODES_MARK(pDep);
      }
    }
  }

  /* If bSelect is requested, remove all non-SELECT nodes from the list
   * (but always keep the first one).
   */
  if (bSelect)
  {
      pThis = pDList;
      while (pThis->pNext != NULL)
      {
        pMark = pThis->pNext;
        if (!(pMark->pNode->flags & NODE_SELECT))
        {
          NODES_UNMARK(pMark->pNode);
          pThis->pNext = pMark->pNext;
          pMark->pNext = pFreeRefs;
          pFreeRefs = pMark;
        }
        else
          pThis = pMark;
      }
  }

  return pDList;
}

/*-------------------------------------------------------------------------*/
void
nodes_freelist (NodeRef * pDList)

/* Free a dependency list produced earlier by nodes_deplist().
 *
 *   pDList: Base of the list to free.
 */

{
  NodeRef * pThis;

  while (pDList)
  {
    NODES_UNMARK(pDList->pNode);
    pThis = pDList;
    pDList = pDList->pNext;
    pThis->pNext = pFreeRefs;
    pFreeRefs = pThis;
  }
}

/***************************************************************************/
