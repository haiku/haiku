// beos_kernel_emu.cpp

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <fs_cache.h>
#include <fs_interface.h>
#include <KernelExport.h>
#include <NodeMonitor.h>

#include "Debug.h"
#include "HaikuKernelNode.h"
#include "HaikuKernelVolume.h"
#include "kernel_emu.h"


// #pragma mark - Notifications


// notify_entry_created
status_t
notify_entry_created(dev_t device, ino_t directory, const char *name,
	ino_t node)
{
	if (!name)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_listener(B_ENTRY_CREATED, 0, device, 0,
		directory, node, NULL, name);
}

// notify_entry_removed
status_t
notify_entry_removed(dev_t device, ino_t directory, const char *name,
	ino_t node)
{
	if (!name)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_listener(B_ENTRY_REMOVED, 0, device, 0,
		directory, node, NULL, name);
}

// notify_entry_moved
status_t
notify_entry_moved(dev_t device, ino_t fromDirectory,
	const char *fromName, ino_t toDirectory, const char *toName,
	ino_t node)
{
	if (!fromName || !toName)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_listener(B_ENTRY_MOVED, 0, device,
		fromDirectory, toDirectory, node, fromName, toName);
}

// notify_stat_changed
status_t
notify_stat_changed(dev_t device, ino_t node, uint32 statFields)
{
	return UserlandFS::KernelEmu::notify_listener(B_STAT_CHANGED, statFields,
		device, 0, 0, node, NULL, NULL);
}

// notify_attribute_changed
status_t
notify_attribute_changed(dev_t device, ino_t node, const char *attribute,
	int32 cause)
{
	if (!attribute)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_listener(B_ATTR_CHANGED, cause,
		device, 0, 0, node, NULL, attribute);
}

// notify_select_event
status_t
notify_select_event(selectsync *sync, uint8 event)
{
	return UserlandFS::KernelEmu::notify_select_event(sync, event, false);
}

// notify_query_entry_created
status_t
notify_query_entry_created(port_id port, int32 token, dev_t device,
	ino_t directory, const char *name, ino_t node)
{
	if (!name)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_query(port, token, B_ENTRY_CREATED,
		device, directory, name, node);
}

// notify_query_entry_removed
status_t
notify_query_entry_removed(port_id port, int32 token, dev_t device,
	ino_t directory, const char *name, ino_t node)
{
	if (!name)
		return B_BAD_VALUE;

	return UserlandFS::KernelEmu::notify_query(port, token, B_ENTRY_REMOVED,
		device, directory, name, node);
}


// #pragma mark - VNodes


// new_vnode
status_t
new_vnode(fs_volume *_volume, ino_t vnodeID, void *privateNode,
	fs_vnode_ops *ops)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	// translate to a wrapper node
	HaikuKernelNode* node;
	status_t error = volume->NewVNode(vnodeID, privateNode, ops, &node);
	if (error != B_OK)
		return error;

	// announce the new node
	error = UserlandFS::KernelEmu::new_vnode(volume->GetID(), vnodeID, node);
	if (error != B_OK)
		volume->UndoNewVNode(node);

	return error;
}

// publish_vnode
status_t
publish_vnode(fs_volume *_volume, ino_t vnodeID, void *privateNode,
	fs_vnode_ops *ops, int type, uint32 flags)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	// translate to a wrapper node
	HaikuKernelNode* node;
	status_t error = volume->PublishVNode(vnodeID, privateNode, ops, type,
		flags, &node);
	if (error != B_OK)
		return error;

	// publish the new node
	error = UserlandFS::KernelEmu::publish_vnode(volume->GetID(), vnodeID, node,
		type, flags);
	if (error != B_OK)
		volume->UndoPublishVNode(node);

	return error;
}

// get_vnode
status_t
get_vnode(fs_volume *_volume, ino_t vnodeID, fs_vnode *privateNode)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	// get the node
	void* foundNode;
	status_t error = UserlandFS::KernelEmu::get_vnode(volume->GetID(), vnodeID,
		&foundNode);
	if (error != B_OK)
		return error;

	*privateNode = *(HaikuKernelNode*)foundNode;

	return B_OK;
}

// put_vnode
status_t
put_vnode(fs_volume *_volume, ino_t vnodeID)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	return UserlandFS::KernelEmu::put_vnode(volume->GetID(), vnodeID);
}

// acquire_vnode
status_t
acquire_vnode(fs_volume *_volume, ino_t vnodeID)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	return UserlandFS::KernelEmu::acquire_vnode(volume->GetID(), vnodeID);
}

// remove_vnode
status_t
remove_vnode(fs_volume *_volume, ino_t vnodeID)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	return UserlandFS::KernelEmu::remove_vnode(volume->GetID(), vnodeID);
}

// unremove_vnode
status_t
unremove_vnode(fs_volume *_volume, ino_t vnodeID)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	return UserlandFS::KernelEmu::unremove_vnode(volume->GetID(), vnodeID);
}

// get_vnode_removed
status_t
get_vnode_removed(fs_volume *_volume, ino_t vnodeID, bool* removed)
{
	HaikuKernelVolume* volume = HaikuKernelVolume::GetVolume(_volume);

	return UserlandFS::KernelEmu::get_vnode_removed(volume->GetID(), vnodeID,
		removed);
}

// volume_for_vnode
fs_volume*
volume_for_vnode(fs_vnode *vnode)
{
	return HaikuKernelNode::GetNode(vnode)->GetVolume()->GetFSVolume();
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
	return UserlandFS::KernelEmu::add_debugger_command(name, hook, help);
}

// remove_debugger_command
int
remove_debugger_command(char *name, debugger_command_hook hook)
{
	return UserlandFS::KernelEmu::remove_debugger_command(name, hook);
}

// parse_expression
uint64
parse_expression(const char *string)
{
	return UserlandFS::KernelEmu::parse_expression(string);
}

// dprintf
void
dprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	UserlandFS::KernelEmu::vdprintf(format, args);
	va_end(args);
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
