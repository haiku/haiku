// kernel_emu.h

#include <stdarg.h>

#include <OS.h>

struct selectsync;

namespace UserlandFS {
namespace KernelEmu {

typedef void*	fs_vnode;

int new_path(const char *path, char **copy);
void free_path(char *p);

status_t notify_listener(int32 operation, uint32 details, dev_t device,
	ino_t oldDirectory, ino_t directory, ino_t node,
	const char* oldName, const char* name);
status_t notify_select_event(selectsync *sync, uint8 event,
	bool unspecifiedEvent);
status_t notify_query(port_id port, int32 token, int32 operation,
	dev_t device, ino_t directory, const char* name, ino_t node);

status_t get_vnode(dev_t nsid, ino_t vnid, fs_vnode* data);
status_t put_vnode(dev_t nsid, ino_t vnid);
status_t new_vnode(dev_t nsid, ino_t vnid, fs_vnode data);
status_t publish_vnode(dev_t nsid, ino_t vnid, fs_vnode data);
status_t remove_vnode(dev_t nsid, ino_t vnid);
status_t unremove_vnode(dev_t nsid, ino_t vnid);
status_t get_vnode_removed(dev_t nsid, ino_t vnid, bool* removed);

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

