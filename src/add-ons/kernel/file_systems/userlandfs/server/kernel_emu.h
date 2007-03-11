// kernel_emu.h

#include <stdarg.h>

#include <OS.h>

struct selectsync;

namespace UserlandFS {
namespace KernelEmu {

typedef int32	mount_id;
typedef int64	vnode_id;
typedef void*	fs_vnode;

int new_path(const char *path, char **copy);
void free_path(char *p);

status_t notify_listener(int32 operation, uint32 details, mount_id device,
	vnode_id oldDirectory, vnode_id directory, vnode_id node,
	const char* oldName, const char* name);
status_t notify_select_event(selectsync *sync, uint32 ref, uint8 event,
	bool unspecifiedEvent);
status_t notify_query(port_id port, int32 token, int32 operation,
	mount_id device, vnode_id directory, const char* name, vnode_id node);

status_t get_vnode(mount_id nsid, vnode_id vnid, fs_vnode* data);
status_t put_vnode(mount_id nsid, vnode_id vnid);
status_t new_vnode(mount_id nsid, vnode_id vnid, fs_vnode data);
status_t publish_vnode(mount_id nsid, vnode_id vnid, fs_vnode data);
status_t remove_vnode(mount_id nsid, vnode_id vnid);
status_t unremove_vnode(mount_id nsid, vnode_id vnid);
status_t get_vnode_removed(mount_id nsid, vnode_id vnid, bool* removed);

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

thread_id spawn_kernel_thread(thread_entry function, const char *threadName,
	long priority, void *arg);


}	// namespace KernelEmu
}	// namespace UserlandFS

