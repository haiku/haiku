#ifndef _NODE_MONITOR_H
#define _NODE_MONITOR_H
/* Node monitor calls for kernel add-ons
**
** Distributed under the terms of the OpenBeOS License.
*/


#include <StorageDefs.h>


/* Flags for the watch_node() call.
 *
 * Note that B_WATCH_MOUNT is NOT included in B_WATCH_ALL.
 * You may prefer to use BVolumeRoster for volume watching.
 */

enum {
    B_STOP_WATCHING		= 0x0000,
	B_WATCH_NAME		= 0x0001,
	B_WATCH_STAT		= 0x0002,
	B_WATCH_ATTR		= 0x0004,
	B_WATCH_DIRECTORY	= 0x0008,
	B_WATCH_ALL			= 0x000f,

	B_WATCH_MOUNT		= 0x0010
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


extern status_t watch_node(const node_ref *node, uint32 flags, BMessenger target);
extern status_t watch_node(const node_ref *node, uint32 flags, 
					const BHandler *handler,
					const BLooper *looper = NULL);

extern status_t stop_watching(BMessenger target);
extern status_t stop_watching(const BHandler *handler, const BLooper *looper = NULL);

#endif	/* __cplusplus && !_KERNEL_MODE */

#endif	/* _NODE_MONITOR_H*/
