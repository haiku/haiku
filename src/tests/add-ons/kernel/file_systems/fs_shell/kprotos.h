#ifndef KPROTOS_H
#define KPROTOS_H

#include "myfs.h"

#define my_stat stat
#define my_dirent dirent

#ifdef __cplusplus
extern "C" {
#endif

int sys_symlink(bool kernel, const char *oldpath, int nfd,
			const char *newpath);
ssize_t sys_readlink(bool kernel, int fd, const char *path, char *buf,
			size_t bufsize);
int sys_mkdir(bool kernel, int fd, const char *path, int perms);
int sys_open(bool kernel, int fd, const char *path, int omode,
			int perms, bool coe);
int sys_open_entry_ref(bool kernel, nspace_id device, vnode_id parent,
			const char *name, int openMode, bool coe);
int sys_close(bool kernel, int fd);
fs_off_t sys_lseek(bool kernel, int fd, fs_off_t pos, int whence);
ssize_t sys_read(bool kernel, int fd, void *buf, size_t len);
ssize_t sys_write(bool kernel, int fd, void *buf, size_t len);
int sys_unlink(bool kernel, int fd, const char *path);
int sys_link(bool kernel, int ofd, const char *oldpath, int nfd,
             const char *newpath);
int sys_rmdir(bool kernel, int fd, const char *path);
int sys_rename(bool glb, int fd, const char *oldpath,
                  int nfd, const char *newpath);
void *sys_mount(bool kernel, const char *filesystem, int fd,
                const char *where, const char *device, ulong flags,
                void *parms, size_t len);
int sys_unmount(bool kernel, int fd, const char *where);
int sys_rstat(bool kernel, int fd, const char *path, struct my_stat *st,
              bool eatlink);
int sys_wstat(bool kernel, int fd, const char *path, struct my_stat *st,
              long mask, bool eatlink);
int sys_ioctl(bool kernel, int fd, int cmd, void *arg, size_t sz);

int sys_opendir(bool kernel, int fd, const char *path, bool coe);
int  sys_readdir(bool kernel, int fd, struct my_dirent *buf, size_t bufsize,
                 long count);
int sys_rewinddir(bool kernel, int fd);
int sys_closedir(bool kernel, int fd);
int sys_chdir(bool kernel, int fd, const char *path);
int sys_access(bool kernel, int fd, const char *path, int mode);

int sys_sync(void);

ssize_t sys_read_attr(bool kernel, int fd, const char *name, int type, void *buffer, size_t len, off_t pos);
ssize_t sys_write_attr(bool kernel, int fd, const char *name, int type, void *buffer, size_t len, off_t pos);
ssize_t sys_remove_attr(bool kernel, int fd, const char *name);

int sys_open_query(bool kernel, int fd, const char *path, const char *query, ulong flags, port_id port, ulong token, void **cookie);
int sys_close_query(bool kernel, int fd, const char *path, void *cookie);
int sys_read_query(bool kernel, int fd, const char *path, void *cookie,struct dirent *dent,size_t bufferSize,long num);

struct nspace;
struct fsystem;
void kill_device_vnodes(dev_t id);
struct nspace *allocate_nspace(void);
void remove_nspace(struct nspace *);
status_t add_nspace(struct nspace *, struct fsystem *, const char *fileSystem, dev_t, ino_t);
	// for initialize() hack use only :)

int init_vnode_layer(void);
void *install_file_system(vnode_ops *ops, const char *name,
                          bool fixed, image_id aid);

#ifdef __cplusplus
}
#endif

#endif	/* KPROTOS_H */
