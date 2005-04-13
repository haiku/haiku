/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <KernelExport.h>
#include <OS.h>
#include <vfs.h>


#define MESSAGE_HEADER	'FOB1'

/*
static void
init_message(uint8 **_buffer, uint32 what, uint32 token)
{
	uint32 *header = *_buffer;
	*header++ = MESSAGE_HEADER;

	*_buffer = (uint8 *)header;
}
*/

status_t
send_notification(port_id port, long token, ulong what, long op, mount_id device,
	mount_id toDevice, vnode_id parentNode, vnode_id toParentNode,
	vnode_id node, const char *name)
{
	// this is currently the BeOS compatible send_notification() function
	// and will probably stay that way, not yet implemented, though

	dprintf("send_notification(port = %ld, token = %ld, op = %ld, device = %ld, "
		"node = %Ld, parentNode = %Ld, toParentNode = %Ld, name = \"%s\"\n",
		port, token, op, device, node, parentNode, toParentNode, name);
	return 0;
}


