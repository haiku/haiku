/* Node monitor calls for kernel add-ons
**
** Distributed under the terms of the OpenBeOS License.
*/

#ifndef _DRIVERS_NODE_MONITOR_H
#define _DRIVERS_NODE_MONITOR_H


#include <OS.h>
#include <storage/NodeMonitor.h>


extern status_t stop_notifying(port_id port, uint32 token);
extern status_t start_watching(dev_t device, ino_t node, uint32 flags,
					port_id port, uint32 token);
extern status_t stop_watching(dev_t device, ino_t node, uint32 flags,
					port_id port, uint32 token);

// ToDo: add simple message parsing convenience functionality

#endif	/* _DRIVERS_NODE_MONITOR_H */
