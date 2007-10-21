#ifndef ZOIDBERG_GARGOYLE_NODE_MESSAGE_H
#define ZOIDBERG_GARGOYLE_NODE_MESSAGE_H
/* NodeMessage - "streaming" interface and support functions for BNodes
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/

/*
   These functions gives a nice BMessage interface to node attributes,
   by letting you transfer attributes to and from BMessages.  It makes
   it so you can use all the convenient Find...() and Add...() functions
   provided by BMessage for attributes too.  You use it as follows:

   BMessage m;
   BNode n(path);
   if (reading) { n>>m; printf("woohoo=%s\n",m.FindString("woohoo")) }
   else { m.AddString("woohoo","it's howdy doody time"); n<<m; }
   
   If there is more than one data item with a given name, the first
   item is the one writen to the node.
*/

#include <Node.h>
#include <Message.h>

#if defined(HAIKU_TARGET_PLATFORM_DANO) || defined(HAIKU_TARGET_PLATFORM_HAIKU) || defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST)
#define _IMPEXP_MAIL
#endif

_IMPEXP_MAIL BNode& operator<<(BNode& n, const BMessage& m);
_IMPEXP_MAIL BNode& operator>>(BNode& n, BMessage& m);
inline const BMessage& operator>>(const BMessage& m, BNode& n){n<<m;return m;}
inline       BMessage& operator<<(      BMessage& m, BNode& n){n>>m;return m;}

#endif	/* ZOIDBERG_GARGOYLE_NODE_MESSAGE_H */
