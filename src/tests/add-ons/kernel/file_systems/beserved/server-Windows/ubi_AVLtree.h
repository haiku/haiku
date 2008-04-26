#ifndef UBI_AVLTREE_H
#define UBI_AVLTREE_H
/* ========================================================================== **
 *                              ubi_AVLtree.h
 *
 *  Copyright (C) 1991-1998 by Christopher R. Hertel
 *
 *  Email: crh@ubiqx.mn.org
 * -------------------------------------------------------------------------- **
 *
 *  This module provides an implementation of AVL height balanced binary
 *  trees.  (Adelson-Velskii, Landis 1962)
 *
 *  This header file contains the basic AVL structure and pointer typedefs
 *  as well as the prototypes needed to access the functions in the AVL
 *  module ubi_AVLtree.  The .c file implements the low-level height balancing
 *  routines that manage the AVL tree, plus all of the basic primops for
 *  adding, searching for, and deleting nodes.
 *
 * -------------------------------------------------------------------------- **
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * -------------------------------------------------------------------------- **
 * $Log: ubi_AVLtree.h,v $
 * Revision 4.5  2000/01/31 05:21:17  crh
 * Made checks for NULL more explicit in a few places.
 *
 * Revision 4.4  1998/06/04 21:29:27  crh
 * Upper-cased defined constants (eg UBI_BINTREE_H) in some header files.
 * This is more "standard", and is what people expect.  Weird, eh?
 *
 * Revision 4.3  1998/06/03 17:41:46  crh
 * Further fiddling with sys_include.h.  It's now in ubi_BinTree.h which is
 * included by all of the binary tree files.
 *
 * Reminder: Some of the ubi_tr* macros in ubi_BinTree.h are redefined in
 *           ubi_AVLtree.h and ubi_SplayTree.h.  This allows easy swapping
 *           of tree types by simply changing a header.  Unfortunately, the
 *           macro redefinitions in ubi_AVLtree.h and ubi_SplayTree.h will
 *           conflict if used together.  You must either choose a single tree
 *           type, or use the underlying function calls directly.  Compare
 *           the two header files for more information.
 *
 * Revision 4.2  1998/06/02 01:26:53  crh
 * Changed ubi_null.h to sys_include.h to make it more generic.
 *
 * Revision 4.1  1998/05/20 04:35:50  crh
 * The C file now includes ubi_null.h.  See ubi_null.h for more info.
 *
 * Revision 4.0  1998/03/10 03:34:45  crh
 * Major changes.
 * By adding the AVL balance field to the base ubi_btNode structure, I no
 * longer need AVL-specific ReplaceNode(), SwapNodes(), and InitNode()
 * functions.  The Remove() function is also simplified.  It's all much
 * cleaner.
 * This is rev. 4.0.  The 3.x series was dropped.
 *
 * Revision 2.5  1997/12/23 04:00:15  crh
 * In this version, all constants &amp; macros defined in the header file have
 * the ubi_tr prefix.  Also cleaned up anything that gcc complained about
 * when run with '-pedantic -fsyntax-only -Wall'.
 *
 * Revision 2.4  1997/07/26 04:36:23  crh
 * Andrew Leppard, aka "Grazgur", discovered that I still had my brains tied
 * on backwards with respect to node deletion.  I did some more digging and
 * discovered that I was not changing the balance values correctly in the
 * single rotation functions.  Double rotation was working correctly because
 * the formula for changing the balance values is the same for insertion or
 * deletion.  Not so for single rotation.
 *
 * I have tested the fix by loading the tree with over 44 thousand names,
 * deleting 2,629 of them (all those in which the second character is 'u')
 * and then walking the tree recursively to verify that the balance factor of
 * each node is correct.  Passed.
 *
 * Thanks Andrew!
 *
 * Also:
 * + Changed ubi_TRUE and ubi_FALSE to ubi_trTRUE and ubi_trFALSE.
 * + Rewrote the ubi_tr&lt;func&gt; macros because they weren't doing what I'd
 *   hoped they would do (see the bottom of the header file).  They work now.
 *
 * Revision 2.3  1997/06/03 05:22:07  crh
 * Changed TRUE and FALSE to ubi_TRUE and ubi_FALSE to avoid causing
 * problems.
 *
 * Revision 2.2  1995/10/03 22:15:47  CRH
 * Ubisized!
 *
 * Revision 2.1  95/03/09  23:46:44  CRH
 * Added the ModuleID static string and function.  These modules are now
 * self-identifying.
 * 
 * Revision 2.0  95/03/05  14:11:22  CRH
 * This revision of ubi_AVLtree coincides with revision 2.0 of ubi_BinTree,
 * and so includes all of the changes to that module.  In addition, a bug in
 * the node deletion process has been fixed.
 *
 * After rewriting the Locate() function in ubi_BinTree, I decided that it was
 * time to overhaul this module.  In the process, I discovered a bug related
 * to node deletion.  To fix the bug, I wrote function Debalance().  A quick
 * glance will show that it is very similar to the Rebalance() function.  In
 * previous versions of this module, I tried to include the functionality of
 * Debalance() within Rebalance(), with poor results.
 *
 * Revision 1.0  93/10/15  22:58:48  CRH
 * With this revision, I have added a set of #define's that provide a single,
 * standard API to all existing tree modules.  Until now, each of the three
 * existing modules had a different function and typedef prefix, as follows:
 *
 *       Module        Prefix
 *     ubi_BinTree     ubi_bt
 *     ubi_AVLtree     ubi_avl
 *     ubi_SplayTree   ubi_spt
 *
 * To further complicate matters, only those portions of the base module
 * (ubi_BinTree) that were superceeded in the new module had the new names.
 * For example, if you were using ubi_SplayTree, the locate function was
 * called "ubi_sptLocate", but the next and previous functions remained
 * "ubi_btNext" and "ubi_btPrev".
 *
 * This was not too terrible if you were familiar with the modules and knew
 * exactly which tree model you wanted to use.  If you wanted to be able to
 * change modules (for speed comparisons, etc), things could get messy very
 * quickly.
 *
 * So, I have added a set of defined names that get redefined in any of the
 * descendant modules.  To use this standardized interface in your code,
 * simply replace all occurances of "ubi_bt", "ubi_avl", and "ubi_spt" with
 * "ubi_tr".  The "ubi_tr" names will resolve to the correct function or
 * datatype names for the module that you are using.  Just remember to
 * include the header for that module in your program file.  Because these
 * names are handled by the preprocessor, there is no added run-time
 * overhead.
 *
 * Note that the original names do still exist, and can be used if you wish
 * to write code directly to a specific module.  This should probably only be
 * done if you are planning to implement a new descendant type, such as
 * red/black trees.  CRH
 *
 *  V0.0 - May, 1990   -  Written by Christopher R. Hertel (CRH).
 *
 *  ========================================================================= **
 */

#include "ubi_BinTree.h"   /* Base erg binary tree support.       */

/* -------------------------------------------------------------------------- **
 *  Function prototypes.
 * -------------------------------------------------------------------------- **
 */

ubi_trBool ubi_avlInsert( ubi_btRootPtr  RootPtr,
                          ubi_btNodePtr  NewNode,
                          ubi_btItemPtr  ItemPtr,
                          ubi_btNodePtr *OldNode );
  /* ------------------------------------------------------------------------ **
   * This function uses a non-recursive algorithm to add a new element to
   * the tree.
   *
   *  Input:   RootPtr  -  a pointer to the ubi_btRoot structure that indicates
   *                       the root of the tree to which NewNode is to be added.
   *           NewNode  -  a pointer to an ubi_btNode structure that is NOT
   *                       part of any tree.
   *           ItemPtr  -  A pointer to the sort key that is stored within
   *                       *NewNode.  ItemPtr MUST point to information stored
   *                       in *NewNode or an EXACT DUPLICATE.  The key data
   *                       indicated by ItemPtr is used to place the new node
   *                       into the tree.
   *           OldNode  -  a pointer to an ubi_btNodePtr.  When searching
   *                       the tree, a duplicate node may be found.  If
   *                       duplicates are allowed, then the new node will
   *                       be simply placed into the tree.  If duplicates
   *                       are not allowed, however, then one of two things
   *                       may happen.
   *                       1) if overwritting *is not* allowed, this
   *                          function will return FALSE (indicating that
   *                          the new node could not be inserted), and
   *                          *OldNode will point to the duplicate that is
   *                          still in the tree.
   *                       2) if overwritting *is* allowed, then this
   *                          function will swap **OldNode for *NewNode.
   *                          In this case, *OldNode will point to the node
   *                          that was removed (thus allowing you to free
   *                          the node).
   *                          **  If you are using overwrite mode, ALWAYS  **
   *                          ** check the return value of this parameter! **
   *                 Note: You may pass NULL in this parameter, the
   *                       function knows how to cope.  If you do this,
   *                       however, there will be no way to return a
   *                       pointer to an old (ie. replaced) node (which is
   *                       a problem if you are using overwrite mode).
   *
   *  Output:  a boolean value indicating success or failure.  The function
   *           will return FALSE if the node could not be added to the tree.
   *           Such failure will only occur if duplicates are not allowed,
   *           nodes cannot be overwritten, AND a duplicate key was found
   *           within the tree.
   * ------------------------------------------------------------------------ **
   */

ubi_btNodePtr ubi_avlRemove( ubi_btRootPtr RootPtr,
                             ubi_btNodePtr DeadNode );
  /* ------------------------------------------------------------------------ **
   * This function removes the indicated node from the tree, after which the
   * tree is rebalanced.
   *
   *  Input:  RootPtr  -  A pointer to the header of the tree that contains
   *                      the node to be removed.
   *          DeadNode -  A pointer to the node that will be removed.
   *
   *  Output: This function returns a pointer to the node that was removed
   *          from the tree (ie. the same as DeadNode).
   *
   *  Note:   The node MUST be in the tree indicated by RootPtr.  If not,
   *          strange and evil things will happen to your trees.
   * ------------------------------------------------------------------------ **
   */

int ubi_avlModuleID( int size, char *list[] );
  /* ------------------------------------------------------------------------ **
   * Returns a set of strings that identify the module.
   *
   *  Input:  size  - The number of elements in the array &lt;list&gt;.
   *          list  - An array of pointers of type (char *).  This array
   *                  should, initially, be empty.  This function will fill
   *                  in the array with pointers to strings.
   *  Output: The number of elements of &lt;list&gt; that were used.  If this value
   *          is less than &lt;size&gt;, the values of the remaining elements are
   *          not guaranteed.
   *
   *  Notes:  Please keep in mind that the pointers returned indicate strings
   *          stored in static memory.  Don't free() them, don't write over
   *          them, etc.  Just read them.
   * ------------------------------------------------------------------------ **
   */

/* -------------------------------------------------------------------------- **
 * Masquarade...
 *
 * This set of defines allows you to write programs that will use any of the
 * implemented binary tree modules (currently BinTree, AVLtree, and SplayTree).
 * Instead of using ubi_avl... or ubi_bt, use ubi_tr... and select the tree
 * type by including the appropriate module header.
 */

#undef ubi_trInsert
#define ubi_trInsert( Rp, Nn, Ip, On ) \
        ubi_avlInsert( (ubi_btRootPtr)(Rp), (ubi_btNodePtr)(Nn), \
                       (ubi_btItemPtr)(Ip), (ubi_btNodePtr *)(On) )

#undef ubi_trRemove
#define ubi_trRemove( Rp, Dn ) \
        ubi_avlRemove( (ubi_btRootPtr)(Rp), (ubi_btNodePtr)(Dn) )

#undef ubi_trModuleID
#define ubi_trModuleID( s, l ) ubi_avlModuleID( s, l )


/* =========================== End  ubi_AVLtree.h =========================== */
#endif /* UBI_AVLTREE_H */
