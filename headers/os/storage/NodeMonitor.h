/***************************************************************************
//
//	File:			NodeMonitor.h
//
//	Description:	The Node Monitor
//
//	Copyright 1992-98, Be Incorporated, All Rights Reserved.
//
***************************************************************************/


#ifndef _NODE_MONITOR_H
#define _NODE_MONITOR_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <StorageDefs.h>
#include <Node.h>
#include <Messenger.h>

class	BLooper;
class	BHandler;

_IMPEXP_BE status_t watch_node(const node_ref *node, 
					   uint32 flags, 
					   BMessenger target);

_IMPEXP_BE status_t watch_node(const node_ref *node, 
					   uint32 flags, 
					   const BHandler *handler,
					   const BLooper *looper = NULL);

_IMPEXP_BE status_t stop_watching(BMessenger target);

_IMPEXP_BE status_t stop_watching(const BHandler *handler, 
					  const BLooper *looper=NULL);

/* Flags for the watch_node() call.
 * Note that B_WATCH_MOUNT is NOT included in 
 * B_WATCH_ALL -- the latter is a convenience
 * for watching all watchable state changes for 
 * a specific node.  Watching for volumes
 * being mounted and unmounted (B_WATCH_MOUNT)
 * is in a category by itself. 
 *
 * BVolumeRoster provides a handy cover for
 * volume watching.
 */
enum {
    B_STOP_WATCHING = 0x0000,
	B_WATCH_NAME =	0x0001,
	B_WATCH_STAT =	0x0002,
	B_WATCH_ATTR =	0x0004,
	B_WATCH_DIRECTORY =	0x0008,
	B_WATCH_ALL = 0x000f,
	B_WATCH_MOUNT = 0x0010
};

/* When a node monitor notification
 * returns to the target, the "opcode" field 
 * will hold one of these values.  The other
 * fields in the message tell you which
 * node (or device) changed.  See the documentation
 * for the names of these other fields.
 */
#define		B_ENTRY_CREATED		1
#define		B_ENTRY_REMOVED		2
#define		B_ENTRY_MOVED		3
#define		B_STAT_CHANGED		4
#define		B_ATTR_CHANGED		5
#define		B_DEVICE_MOUNTED	6
#define		B_DEVICE_UNMOUNTED	7


#endif
