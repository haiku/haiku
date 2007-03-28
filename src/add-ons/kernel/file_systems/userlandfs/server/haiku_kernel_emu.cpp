// beos_kernel_emu.cpp

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <fs_cache.h>
#include <fs_interface.h>
#include <KernelExport.h>
#include <NodeMonitor.h>

#include "Debug.h"
#include "haiku_fs_cache.h"
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


// #pragma mark - Transaction


// cache_start_transaction
int32
cache_start_transaction(void *_cache)
{
	return UserlandFS::HaikuKernelEmu::cache_start_transaction(_cache);
}

// cache_sync_transaction
status_t
cache_sync_transaction(void *_cache, int32 id)
{
	return UserlandFS::HaikuKernelEmu::cache_sync_transaction(_cache, id);
}

// cache_end_transaction
status_t
cache_end_transaction(void *_cache, int32 id,
	transaction_notification_hook hook, void *data)
{
	return UserlandFS::HaikuKernelEmu::cache_end_transaction(_cache, id, hook,
		data);
}

// cache_abort_transaction
status_t
cache_abort_transaction(void *_cache, int32 id)
{
	return UserlandFS::HaikuKernelEmu::cache_abort_transaction(_cache, id);
}

// cache_detach_sub_transaction
int32
cache_detach_sub_transaction(void *_cache, int32 id,
	transaction_notification_hook hook, void *data)
{
	return UserlandFS::HaikuKernelEmu::cache_detach_sub_transaction(_cache, id,
		hook, data);
}

// cache_abort_sub_transaction
status_t
cache_abort_sub_transaction(void *_cache, int32 id)
{
	return UserlandFS::HaikuKernelEmu::cache_abort_sub_transaction(_cache, id);
}

// cache_start_sub_transaction
status_t
cache_start_sub_transaction(void *_cache, int32 id)
{
	return UserlandFS::HaikuKernelEmu::cache_start_sub_transaction(_cache, id);
}

// cache_next_block_in_transaction
status_t
cache_next_block_in_transaction(void *_cache, int32 id, uint32 *_cookie,
	off_t *_blockNumber, void **_data, void **_unchangedData)
{
	return UserlandFS::HaikuKernelEmu::cache_next_block_in_transaction(_cache,
		id, _cookie, _blockNumber, _data, _unchangedData);
}

// cache_blocks_in_transaction
int32
cache_blocks_in_transaction(void *_cache, int32 id)
{
	return UserlandFS::HaikuKernelEmu::cache_blocks_in_transaction(_cache, id);
}

// cache_blocks_in_sub_transaction
int32
cache_blocks_in_sub_transaction(void *_cache, int32 id)
{
	return UserlandFS::HaikuKernelEmu::cache_blocks_in_sub_transaction(_cache,
		id);
}


// #pragma mark - Block Cache


// block_cache_delete
void
block_cache_delete(void *_cache, bool allowWrites)
{
	UserlandFS::HaikuKernelEmu::block_cache_delete(_cache, allowWrites);
}

// block_cache_create
void *
block_cache_create(int fd, off_t numBlocks, size_t blockSize, bool readOnly)
{
	return UserlandFS::HaikuKernelEmu::block_cache_create(fd, numBlocks,
		blockSize, readOnly);
}

// block_cache_sync
status_t
block_cache_sync(void *_cache)
{
	return UserlandFS::HaikuKernelEmu::block_cache_sync(_cache);
}

// block_cache_make_writable
status_t
block_cache_make_writable(void *_cache, off_t blockNumber, int32 transaction)
{
	return UserlandFS::HaikuKernelEmu::block_cache_make_writable(_cache,
		blockNumber, transaction);
}

// block_cache_get_writable_etc
void *
block_cache_get_writable_etc(void *_cache, off_t blockNumber, off_t base,
	off_t length, int32 transaction)
{
	return UserlandFS::HaikuKernelEmu::block_cache_get_writable_etc(_cache,
		blockNumber, base, length, transaction);
}

// block_cache_get_writable
void *
block_cache_get_writable(void *_cache, off_t blockNumber, int32 transaction)
{
	return UserlandFS::HaikuKernelEmu::block_cache_get_writable(_cache,
		blockNumber, transaction);
}

// block_cache_get_empty
void *
block_cache_get_empty(void *_cache, off_t blockNumber, int32 transaction)
{
	return UserlandFS::HaikuKernelEmu::block_cache_get_empty(_cache,
		blockNumber, transaction);
}

// block_cache_get_etc
const void *
block_cache_get_etc(void *_cache, off_t blockNumber, off_t base, off_t length)
{
	return UserlandFS::HaikuKernelEmu::block_cache_get_etc(_cache, blockNumber,
		base, length);
}

// block_cache_get
const void *
block_cache_get(void *_cache, off_t blockNumber)
{
	return UserlandFS::HaikuKernelEmu::block_cache_get(_cache, blockNumber);
}

// block_cache_set_dirty
status_t
block_cache_set_dirty(void *_cache, off_t blockNumber, bool isDirty,
	int32 transaction)
{
	return UserlandFS::HaikuKernelEmu::block_cache_set_dirty(_cache,
		blockNumber, isDirty, transaction);
}

// block_cache_put
void
block_cache_put(void *_cache, off_t blockNumber)
{
	UserlandFS::HaikuKernelEmu::block_cache_put(_cache, blockNumber);
}


// #pragma mark - File Cache

// file_cache_create
void *
file_cache_create(mount_id mountID, vnode_id vnodeID, off_t size, int fd)
{
	return UserlandFS::HaikuKernelEmu::file_cache_create(mountID, vnodeID,
		size, fd);
}

// file_cache_delete
void
file_cache_delete(void *_cacheRef)
{
	UserlandFS::HaikuKernelEmu::file_cache_delete(_cacheRef);
}

// file_cache_set_size
status_t
file_cache_set_size(void *_cacheRef, off_t size)
{
	return UserlandFS::HaikuKernelEmu::file_cache_set_size(_cacheRef, size);
}

// file_cache_sync
status_t
file_cache_sync(void *_cache)
{
	return UserlandFS::HaikuKernelEmu::file_cache_sync(_cache);
}

// file_cache_invalidate_file_map
status_t
file_cache_invalidate_file_map(void *_cacheRef, off_t offset, off_t size)
{
	return UserlandFS::HaikuKernelEmu::file_cache_invalidate_file_map(
		_cacheRef, offset, size);
}

// file_cache_read_pages
status_t
file_cache_read_pages(void *_cacheRef, off_t offset, const iovec *vecs,
	size_t count, size_t *_numBytes)
{
	return UserlandFS::HaikuKernelEmu::file_cache_read_pages(_cacheRef,
		offset, vecs, count, _numBytes);
}

// file_cache_write_pages
status_t
file_cache_write_pages(void *_cacheRef, off_t offset, const iovec *vecs,
	size_t count, size_t *_numBytes)
{
	return UserlandFS::HaikuKernelEmu::file_cache_write_pages(_cacheRef,
		offset, vecs, count, _numBytes);
}

// file_cache_read
status_t
file_cache_read(void *_cacheRef, off_t offset, void *bufferBase, size_t *_size)
{
	return UserlandFS::HaikuKernelEmu::file_cache_read(_cacheRef, offset,
		bufferBase, _size);
}

// file_cache_write
status_t
file_cache_write(void *_cacheRef, off_t offset, const void *buffer,
	size_t *_size)
{
	return UserlandFS::HaikuKernelEmu::file_cache_write(_cacheRef, offset,
		buffer, _size);
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
uint32
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
