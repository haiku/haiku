// kernel_emu.h

#include <OS.h>

struct selectsync;

namespace UserlandFS {
namespace KernelEmu {

typedef int32	mount_id;
typedef int64	vnode_id;
typedef void*	fs_vnode;

int new_path(const char *path, char **copy);
void free_path(char *p);

status_t notify_listener(int op, mount_id nsid, vnode_id vnida, vnode_id vnidb,
	vnode_id vnidc, const char *name);
void notify_select_event(selectsync *sync, uint32 ref, uint8 event);
status_t send_notification(port_id targetPort, long token, ulong what, long op,
	mount_id nsida, mount_id nsidb, vnode_id vnida, vnode_id vnidb,
	vnode_id vnidc, const char *name);

status_t get_vnode(mount_id nsid, vnode_id vnid, fs_vnode* data);
status_t put_vnode(mount_id nsid, vnode_id vnid);
status_t new_vnode(mount_id nsid, vnode_id vnid, fs_vnode data);
status_t publish_vnode(mount_id nsid, vnode_id vnid, fs_vnode data);
status_t remove_vnode(mount_id nsid, vnode_id vnid);
status_t unremove_vnode(mount_id nsid, vnode_id vnid);
status_t is_vnode_removed(mount_id nsid, vnode_id vnid);

void kernel_debugger(const char *message);
void panic(const char *format, ...);

int add_debugger_command(char *name, int (*func)(int argc, char **argv),
	char *help);
int remove_debugger_command(char *name, int (*func)(int argc, char **argv));

thread_id spawn_kernel_thread(thread_entry function, const char *threadName,
	long priority, void *arg);


}	// namespace KernelEmu
}	// namespace UserlandFS

