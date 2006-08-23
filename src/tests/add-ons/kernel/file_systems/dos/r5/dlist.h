/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_DLIST_H_
#define _DOSFS_DLIST_H_

status_t dlist_init(nspace *vol);
status_t dlist_uninit(nspace *vol);
status_t dlist_add(nspace *vol, vnode_id vnid);
status_t dlist_remove(nspace *vol, vnode_id vnid);
vnode_id dlist_find(nspace *vol, uint32 cluster);

void dlist_dump(nspace *vol);

#endif
