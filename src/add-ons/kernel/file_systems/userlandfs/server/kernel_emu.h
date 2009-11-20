// kernel_emu.h

#include <stdarg.h>

#include <OS.h>

#include "FSCapabilities.h"


struct selectsync;
struct file_io_vec;

namespace UserlandFS {
namespace KernelEmu {

int new_path(const char *path, char **copy);
void free_path(char *p);

status_t notify_listener(int32 operation, uint32 details, dev_t device,
	ino_t oldDirectory, ino_t directory, ino_t node,
	const char* oldName, const char* name);
status_t notify_select_event(selectsync *sync, uint8 event,
	bool unspecifiedEvent);
status_t notify_query(port_id port, int32 token, int32 operation,
	dev_t device, ino_t directory, const char* name, ino_t node);

status_t get_vnode(dev_t nsid, ino_t vnid, void** node);
status_t put_vnode(dev_t nsid, ino_t vnid);
status_t acquire_vnode(dev_t nsid, ino_t vnodeID);
status_t new_vnode(dev_t nsid, ino_t vnid, void* data,
	const FSVNodeCapabilities& capabilities);
status_t publish_vnode(dev_t nsid, ino_t vnid, void* data, int type,
	uint32 flags, const FSVNodeCapabilities& capabilities);
status_t publish_vnode(dev_t nsid, ino_t vnid, void* data,
	const FSVNodeCapabilities& capabilities);
status_t remove_vnode(dev_t nsid, ino_t vnid);
status_t unremove_vnode(dev_t nsid, ino_t vnid);
status_t get_vnode_removed(dev_t nsid, ino_t vnid, bool* removed);

status_t file_cache_create(dev_t mountID, ino_t vnodeID, off_t size);
status_t file_cache_delete(dev_t mountID, ino_t vnodeID);
status_t file_cache_set_enabled(dev_t mountID, ino_t vnodeID, bool enabled);
status_t file_cache_set_size(dev_t mountID, ino_t vnodeID, off_t size);
status_t file_cache_sync(dev_t mountID, ino_t vnodeID);

status_t file_cache_read(dev_t mountID, ino_t vnodeID, void *cookie,
	off_t offset, void *bufferBase, size_t *_size);
status_t file_cache_write(dev_t mountID, ino_t vnodeID, void *cookie,
	off_t offset, const void *buffer, size_t *_size);

status_t do_iterative_fd_io(dev_t volumeID, int fd, int32 requestID,
	void* cookie, const file_io_vec* vecs, uint32 vecCount);
status_t read_from_io_request(dev_t volumeID, int32 requestID, void* buffer,
	size_t size);
status_t write_to_io_request(dev_t volumeID, int32 requestID, const void* buffer,
	size_t size);
status_t notify_io_request(dev_t volumeID, int32 requestID, status_t status);

status_t add_node_listener(dev_t device, ino_t node, uint32 flags,
	void* listener);
status_t remove_node_listener(dev_t device, ino_t node, void* listener);

void kernel_debugger(const char *message);
void vpanic(const char *format, va_list args);
void panic(const char *format, ...) __attribute__ ((format (__printf__, 1, 2)));

void vdprintf(const char *format, va_list args);
void dprintf(const char *format, ...)
	__attribute__ ((format (__printf__, 1, 2)));

void dump_block(const char *buffer, int size, const char *prefix);

int add_debugger_command(char *name, int (*func)(int argc, char **argv),
	char *help);
int remove_debugger_command(char *name, int (*func)(int argc, char **argv));
uint32 parse_expression(const char *string);

thread_id spawn_kernel_thread(thread_entry function, const char *threadName,
	long priority, void *arg);


}	// namespace KernelEmu
}	// namespace UserlandFS

