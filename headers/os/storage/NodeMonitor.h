/*
 * Copyright 2003-2010, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NODE_MONITOR_H
#define _NODE_MONITOR_H


#include <StorageDefs.h>


/* Flags for the watch_node() call.
 *
 * Note that B_WATCH_MOUNT is NOT included in B_WATCH_ALL.
 * You may prefer to use BVolumeRoster for volume watching.
 */

enum {
	B_STOP_WATCHING			= 0x0000,
	B_WATCH_NAME			= 0x0001,
	B_WATCH_STAT			= 0x0002,
	B_WATCH_ATTR			= 0x0004,
	B_WATCH_DIRECTORY		= 0x0008,
	B_WATCH_ALL				= 0x000f,

	B_WATCH_MOUNT			= 0x0010,
	B_WATCH_INTERIM_STAT	= 0x0020
};


/* The "opcode" field of the B_NODE_MONITOR notification message you get.
 *
 * The presence and meaning of the other fields in that message specifying what
 * exactly caused the notification depend on this value.
 */

#define	B_ENTRY_CREATED		1
#define	B_ENTRY_REMOVED		2
#define	B_ENTRY_MOVED		3
#define	B_STAT_CHANGED		4
#define	B_ATTR_CHANGED		5
#define	B_DEVICE_MOUNTED	6
#define	B_DEVICE_UNMOUNTED	7


// More specific info in the "cause" field of B_ATTR_CHANGED notification
// messages. (Haiku only)
#define	B_ATTR_CREATED		1
#define	B_ATTR_REMOVED		2
//		B_ATTR_CHANGED is reused


// More specific info in the "fields" field of B_STAT_CHANGED notification
// messages, specifying what parts of the stat data have actually been
// changed. (Haiku only)
enum {
	B_STAT_MODE					= 0x0001,
	B_STAT_UID					= 0x0002,
	B_STAT_GID					= 0x0004,
	B_STAT_SIZE					= 0x0008,
	B_STAT_ACCESS_TIME			= 0x0010,
	B_STAT_MODIFICATION_TIME	= 0x0020,
	B_STAT_CREATION_TIME		= 0x0040,
	B_STAT_CHANGE_TIME			= 0x0080,

	B_STAT_INTERIM_UPDATE		= 0x1000
};


/* C++ callable Prototypes
 *
 * Since you are not able to parse BMessages from plain C, there is no
 * API exported.
 */

#if defined(__cplusplus) && !defined(_KERNEL_MODE)

// these are only needed for the function exports
#include <Node.h>
#include <Messenger.h>

class BLooper;
class BHandler;


extern status_t watch_volume(dev_t volume, uint32 flags, BMessenger target);
extern status_t watch_volume(dev_t volume, uint32 flags,
					const BHandler *handler, const BLooper *looper = NULL);

extern status_t watch_node(const node_ref *node, uint32 flags,
					BMessenger target);
extern status_t watch_node(const node_ref *node, uint32 flags, 
					const BHandler *handler, const BLooper *looper = NULL);

extern status_t stop_watching(BMessenger target);
extern status_t stop_watching(const BHandler *handler, const BLooper *looper = NULL);

#endif	/* __cplusplus && !_KERNEL_MODE */

#endif	/* _NODE_MONITOR_H*/
