// kernel_emu.h

#include <fs_interface.h>
#include <KernelExport.h>
#include <OS.h>

namespace UserlandFS {
namespace KernelEmu {

int new_path(const char *path, char **copy);
void free_path(char *p);

int notify_listener(int op, mount_id nsid, vnode_id vnida, vnode_id vnidb,
	vnode_id vnidc, const char *name);
void notify_select_event(selectsync *sync, uint32 ref);
int send_notification(port_id targetPort, long token, ulong what, long op,
	mount_id nsida, mount_id nsidb, vnode_id vnida, vnode_id vnidb,
	vnode_id vnidc, const char *name);

int get_vnode(mount_id nsid, vnode_id vnid, void** data);
int put_vnode(mount_id nsid, vnode_id vnid);
int new_vnode(mount_id nsid, vnode_id vnid, void* data);
int remove_vnode(mount_id nsid, vnode_id vnid);
int unremove_vnode(mount_id nsid, vnode_id vnid);
int is_vnode_removed(mount_id nsid, vnode_id vnid);

void kernel_debugger(const char *message);
void panic(const char *format, ...);

int add_debugger_command(char *name, int (*func)(int argc, char **argv),
	char *help);
int remove_debugger_command(char *name, int (*func)(int argc, char **argv));

thread_id spawn_kernel_thread(thread_entry function, const char *threadName,
	long priority, void *arg);


}	// namespace KernelEmu
}	// namespace UserlandFS

