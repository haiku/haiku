#ifndef _FSSH_NODE_MONITOR_H
#define _FSSH_NODE_MONITOR_H
/* Node monitor calls for kernel add-ons
**
** Distributed under the terms of the MIT License.
*/


#include "fssh_defs.h"


/* Flags for the watch_node() call.
 *
 * Note that B_WATCH_MOUNT is NOT included in B_WATCH_ALL.
 * You may prefer to use BVolumeRoster for volume watching.
 */

enum {
    FSSH_B_STOP_WATCHING		= 0x0000,
	FSSH_B_WATCH_NAME			= 0x0001,
	FSSH_B_WATCH_STAT			= 0x0002,
	FSSH_B_WATCH_ATTR			= 0x0004,
	FSSH_B_WATCH_DIRECTORY		= 0x0008,
	FSSH_B_WATCH_ALL			= 0x000f,

	FSSH_B_WATCH_MOUNT			= 0x0010,
	FSSH_B_WATCH_INTERIM_STAT	= 0x0020
};


/* The "opcode" field of the B_NODE_MONITOR notification message you get.
 *
 * The presence and meaning of the other fields in that message specifying what
 * exactly caused the notification depend on this value.
 */

#define	FSSH_B_ENTRY_CREATED	1
#define	FSSH_B_ENTRY_REMOVED	2
#define	FSSH_B_ENTRY_MOVED		3
#define	FSSH_B_STAT_CHANGED		4
#define	FSSH_B_ATTR_CHANGED		5
#define	FSSH_B_DEVICE_MOUNTED	6
#define	FSSH_B_DEVICE_UNMOUNTED	7


// More specific info in the "cause" field of B_ATTR_CHANGED notification
// messages. (Haiku only)
#define	FSSH_B_ATTR_CREATED		1
#define	FSSH_B_ATTR_REMOVED		2
//		FSSH_B_ATTR_CHANGED is reused


// More specific info in the "fields" field of B_STAT_CHANGED notification
// messages, specifying what parts of the stat data have actually been
// changed. (Haiku only)
enum {
	FSSH_B_STAT_MODE				= 0x0001,
	FSSH_B_STAT_UID					= 0x0002,
	FSSH_B_STAT_GID					= 0x0004,
	FSSH_B_STAT_SIZE				= 0x0008,
	FSSH_B_STAT_ACCESS_TIME			= 0x0010,
	FSSH_B_STAT_MODIFICATION_TIME	= 0x0020,
	FSSH_B_STAT_CREATION_TIME		= 0x0040,
	FSSH_B_STAT_CHANGE_TIME			= 0x0080,
	FSSH_B_STAT_INTERIM_UPDATE		= 0x1000
};


#endif	/* _FSSH_NODE_MONITOR_H */
