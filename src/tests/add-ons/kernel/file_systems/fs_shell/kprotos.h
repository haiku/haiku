#ifndef KPROTOS_H
#define KPROTOS_H

#include "compat.h"
#include "myfs.h"

#ifdef __cplusplus
extern "C" {
#endif

int sys_symlink(int kernel, const char *oldpath, int nfd,
			const char *newpath);
ssize_t sys_readlink(int kernel, int fd, const char *path, char *buf,
			size_t bufsize);
int sys_mkdir(int kernel, int fd, const char *path, int perms);
int sys_open(int kernel, int fd, const char *path, int omode,
			int perms, int coe);
int sys_open_entry_ref(int kernel, nspace_id device, vnode_id parent,
			const char *name, int openMode, int coe);
int sys_close(int kernel, int fd);
fs_off_t sys_lseek(int kernel, int fd, fs_off_t pos, int whence);
ssize_t sys_read(int kernel, int fd, void *buf, size_t len);
ssize_t sys_write(int kernel, int fd, const void *buf, size_t len);
int sys_unlink(int kernel, int fd, const char *path);
int sys_link(int kernel, int ofd, const char *oldpath, int nfd,
             const char *newpath);
int sys_rmdir(int kernel, int fd, const char *path);
int sys_rename(int glb, int fd, const char *oldpath,
                  int nfd, const char *newpath);
void *sys_mount(int kernel, const char *filesystem, int fd,
                const char *where, const char *device, ulong flags,
                void *parms, size_t len);
int sys_unmount(int kernel, int fd, const char *where);
int sys_write_fs_info(int kernel, dev_t device, struct fs_info *info, int mask);
int sys_rstat(int kernel, int fd, const char *path, struct my_stat *st,
              int eatlink);
int sys_wstat(int kernel, int fd, const char *path, struct my_stat *st,
              long mask, int eatlink);
int sys_ioctl(int kernel, int fd, int cmd, void *arg, size_t sz);

int sys_opendir(int kernel, int fd, const char *path, int coe);
int  sys_readdir(int kernel, int fd, struct my_dirent *buf, size_t bufsize,
                 long count);
int sys_rewinddir(int kernel, int fd);
int sys_closedir(int kernel, int fd);
int sys_chdir(int kernel, int fd, const char *path);
int sys_access(int kernel, int fd, const char *path, int mode);

int sys_sync(void);

int sys_open_attr_dir(int kernel, int fd, const char *path);
ssize_t sys_read_attr(int kernel, int fd, const char *name, int type,
			void *buffer, size_t len, off_t pos);
ssize_t sys_write_attr(int kernel, int fd, const char *name, int type,
			const void *buffer, size_t len, off_t pos);
ssize_t sys_remove_attr(int kernel, int fd, const char *name);
int sys_stat_attr(int kernel, int fd, const char *path, const char *name,
			my_attr_info *info);

int sys_mkindex(int kernel, dev_t device, const char *index, int type, int flags);
 
int sys_open_query(int kernel, int fd, const char *path, const char *query, ulong flags, port_id port, ulong token, void **cookie);
int sys_close_query(int kernel, int fd, const char *path, void *cookie);
int sys_read_query(int kernel, int fd, const char *path, void *cookie, struct my_dirent *dent,size_t bufferSize,long num);

struct nspace;
struct fsystem;
void kill_device_vnodes(dev_t id);
struct nspace *allocate_nspace(void);
void remove_nspace(struct nspace *);
status_t add_nspace(struct nspace *, struct fsystem *, const char *fileSystem, dev_t, ino_t);
	// for initialize() hack use only :)

int init_vnode_layer(void);
void *install_file_system(vnode_ops *ops, const char *name,
                          int fixed, image_id aid);

status_t initialize_file_system(const char *device, const char *fs,
			void *params, int paramLength);

#ifdef __cplusplus
}
#endif

#endif	/* KPROTOS_H */
