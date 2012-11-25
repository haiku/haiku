/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "fssh_errors.h"
#include "fssh_fs_interface.h"


fssh_status_t
fssh_notify_entry_created(fssh_mount_id device, fssh_vnode_id directory,
	const char *name, fssh_vnode_id node)
{
	return FSSH_B_OK;
}


fssh_status_t
fssh_notify_entry_removed(fssh_mount_id device, fssh_vnode_id directory,
	const char *name, fssh_vnode_id node)
{
	return FSSH_B_OK;
}


fssh_status_t
fssh_notify_entry_moved(fssh_mount_id device, fssh_vnode_id fromDirectory,
	const char *fromName, fssh_vnode_id toDirectory, const char *toName,
	fssh_vnode_id node)
{
	return FSSH_B_OK;
}


fssh_status_t
fssh_notify_stat_changed(fssh_mount_id device, fssh_vnode_id node,
	uint32_t statFields)
{
	return FSSH_B_OK;
}


fssh_status_t
fssh_notify_attribute_changed(fssh_mount_id device, fssh_vnode_id node,
	const char *attribute, int32_t cause)
{
	return FSSH_B_OK;
}


fssh_status_t
fssh_notify_query_entry_created(fssh_port_id port, int32_t token,
	fssh_mount_id device, fssh_vnode_id directory, const char *name,
	fssh_vnode_id node)
{
	return FSSH_B_OK;
}


fssh_status_t
fssh_notify_query_entry_removed(fssh_port_id port, int32_t token,
	fssh_mount_id device, fssh_vnode_id directory, const char *name,
	fssh_vnode_id node)
{
	return FSSH_B_OK;
}


fssh_status_t
fssh_notify_query_attr_changed(fssh_port_id port, int32_t token,
	fssh_mount_id device, fssh_vnode_id directory, const char *name,
	fssh_vnode_id node)
{
	return FSSH_B_OK;
}
