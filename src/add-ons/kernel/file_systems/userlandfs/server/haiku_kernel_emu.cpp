// beos_kernel_emu.cpp

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <fs_interface.h>
#include <KernelExport.h>
#include <NodeMonitor.h>

#include "Debug.h"
#include "kernel_emu.h"


// #pragma mark - Notifications


// notify_entry_created
status_t
notify_entry_created(mount_id device, vnode_id directory, const char *name,
	vnode_id node)
{
	if (!name)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_listener(B_ENTRY_CREATED, 0, device, 0,
		directory, node, NULL, name);
}

// notify_entry_removed
status_t
notify_entry_removed(mount_id device, vnode_id directory, const char *name,
	vnode_id node)
{
	if (!name)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_listener(B_ENTRY_REMOVED, 0, device, 0,
		directory, node, NULL, name);
}

// notify_entry_moved
status_t
notify_entry_moved(mount_id device, vnode_id fromDirectory,
	const char *fromName, vnode_id toDirectory, const char *toName,
	vnode_id node)
{
	if (!fromName || !toName)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_listener(B_ENTRY_MOVED, 0, device,
		fromDirectory, toDirectory, node, fromName, toName);
}

// notify_stat_changed
status_t
notify_stat_changed(mount_id device, vnode_id node, uint32 statFields)
{
	return UserlandFS::KernelEmu::notify_listener(B_STAT_CHANGED, statFields,
		device, 0, 0, node, NULL, NULL);
}

// notify_attribute_changed
status_t
notify_attribute_changed(mount_id device, vnode_id node, const char *attribute,
	int32 cause)
{
	if (!attribute)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_listener(B_ATTR_CHANGED, cause,
		device, 0, 0, node, NULL, attribute);
}

// notify_select_event
status_t
notify_select_event(selectsync *sync, uint32 ref, uint8 event)
{
	return UserlandFS::KernelEmu::notify_select_event(sync, ref, event, false);
}

// notify_query_entry_created
status_t
notify_query_entry_created(port_id port, int32 token, mount_id device,
	vnode_id directory, const char *name, vnode_id node)
{
	if (!name)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_query(port, token, B_ENTRY_CREATED,
		device, directory, name, node);
}

// notify_query_entry_removed
status_t
notify_query_entry_removed(port_id port, int32 token, mount_id device,
	vnode_id directory, const char *name, vnode_id node)
{
	if (!name)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_query(port, token, B_ENTRY_REMOVED,
		device, directory, name, node);
}


// #pragma mark - VNodes


// new_vnode
status_t
new_vnode(mount_id mountID, vnode_id vnodeID, fs_vnode privateNode)
{
	return UserlandFS::KernelEmu::publish_vnode(mountID, vnodeID, privateNode);
}

// publish_vnode
status_t
publish_vnode(mount_id mountID, vnode_id vnodeID, fs_vnode privateNode)
{
	return UserlandFS::KernelEmu::publish_vnode(mountID, vnodeID, privateNode);
}

// get_vnode
status_t
get_vnode(mount_id mountID, vnode_id vnodeID, fs_vnode *privateNode)
{
	return UserlandFS::KernelEmu::get_vnode(mountID, vnodeID, privateNode);
}

// put_vnode
status_t
put_vnode(mount_id mountID, vnode_id vnodeID)
{
	return UserlandFS::KernelEmu::put_vnode(mountID, vnodeID);
}

// remove_vnode
status_t
remove_vnode(mount_id mountID, vnode_id vnodeID)
{
	return UserlandFS::KernelEmu::remove_vnode(mountID, vnodeID);
}

// unremove_vnode
status_t
unremove_vnode(mount_id mountID, vnode_id vnodeID)
{
	return UserlandFS::KernelEmu::unremove_vnode(mountID, vnodeID);
}

// get_vnode_removed
status_t
get_vnode_removed(mount_id mountID, vnode_id vnodeID, bool* removed)
{
	return UserlandFS::KernelEmu::get_vnode_removed(mountID, vnodeID, removed);
}


// #pragma mark - Misc


// kernel_debugger
void
kernel_debugger(const char *message)
{
	UserlandFS::KernelEmu::kernel_debugger(message);
}

// panic
void
panic(const char *format, ...)
{
	char buffer[1024];
	strcpy(buffer, "PANIC: ");
	int32 prefixLen = strlen(buffer);
	int bufferSize = sizeof(buffer) - prefixLen;
	va_list args;
	va_start(args, format);
	vsnprintf(buffer + prefixLen, bufferSize - 1, format, args);
	va_end(args);
	buffer[sizeof(buffer) - 1] = '\0';
	debugger(buffer);
}

// add_debugger_command
int
add_debugger_command(char *name, debugger_command_hook hook, char *help)
{
	return B_OK;
}

// remove_debugger_command
int
remove_debugger_command(char *name, debugger_command_hook hook)
{
	return B_OK;
}

// kprintf
void
kprintf(const char *format, ...)
{
}

// spawn_kernel_thread
thread_id
spawn_kernel_thread(thread_func function, const char *threadName,
	int32 priority, void *arg)
{
	return UserlandFS::KernelEmu::spawn_kernel_thread(function,	threadName,
		priority, arg);
}
