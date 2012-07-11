/*
 * Copyright 2004-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_SYSCALLS_H
#define _SYSTEM_SYSCALLS_H


#include <arch_config.h>
#include <DiskDeviceDefs.h>
#include <image.h>
#include <OS.h>

#include <signal.h>
#include <sys/socket.h>


#ifdef __cplusplus
extern "C" {
#endif

struct attr_info;
struct dirent;
struct Elf32_Sym;
struct fd_info;
struct fd_set;
struct fs_info;
struct iovec;
struct msqid_ds;
struct net_stat;
struct pollfd;
struct rlimit;
struct scheduling_analysis;
struct _sem_t;
struct sembuf;
union semun;
struct sigaction;
struct signal_frame_data;
struct stat;
struct system_profiler_parameters;
struct user_timer_info;

struct disk_device_job_progress_info;
struct partitionable_space_data;
struct thread_creation_attributes;
struct user_disk_device_data;
struct user_disk_device_job_info;
struct user_disk_system_info;

// This marks the beginning of the syscalls prototypes for gensyscallinfos.
// NOTE:
// * Nothing but those prototypes may live here.
// * The arguments of the functions must be named to be processed properly.

#ifdef GEN_SYSCALL_INFOS_PROCESSING
#	define __NO_RETURN
#	pragma syscalls begin
#else
#	define __NO_RETURN	__attribute__((noreturn))
#endif

extern int			_kern_is_computer_on(void);
extern status_t		_kern_generic_syscall(const char *subsystem, uint32 function,
						void *buffer, size_t bufferSize);

extern int			_kern_getrlimit(int resource, struct rlimit * rlp);
extern int			_kern_setrlimit(int resource, const struct rlimit * rlp);

extern status_t		_kern_shutdown(bool reboot);
extern status_t		_kern_get_safemode_option(const char *parameter,
						char *buffer, size_t *_bufferSize);

extern ssize_t		_kern_wait_for_objects(object_wait_info* infos, int numInfos,
						uint32 flags, bigtime_t timeout);

/* user mutex functions */
extern status_t		_kern_mutex_lock(int32* mutex, const char* name,
						uint32 flags, bigtime_t timeout);
extern status_t		_kern_mutex_unlock(int32* mutex, uint32 flags);
extern status_t		_kern_mutex_switch_lock(int32* fromMutex, int32* toMutex,
						const char* name, uint32 flags, bigtime_t timeout);

/* sem functions */
extern sem_id		_kern_create_sem(int count, const char *name);
extern status_t		_kern_delete_sem(sem_id id);
extern status_t		_kern_switch_sem(sem_id releaseSem, sem_id id);
extern status_t		_kern_switch_sem_etc(sem_id releaseSem, sem_id id,
						uint32 count, uint32 flags, bigtime_t timeout);
extern status_t		_kern_acquire_sem(sem_id id);
extern status_t		_kern_acquire_sem_etc(sem_id id, uint32 count, uint32 flags,
						bigtime_t timeout);
extern status_t		_kern_release_sem(sem_id id);
extern status_t		_kern_release_sem_etc(sem_id id, uint32 count, uint32 flags);
extern status_t		_kern_get_sem_count(sem_id id, int32* thread_count);
extern status_t		_kern_get_sem_info(sem_id semaphore, struct sem_info *info,
						size_t size);
extern status_t		_kern_get_next_sem_info(team_id team, int32 *cookie,
						struct sem_info *info, size_t size);
extern status_t		_kern_set_sem_owner(sem_id id, team_id proc);

/* POSIX realtime sem syscalls */
extern status_t		_kern_realtime_sem_open(const char* name,
						int openFlagsOrShared, mode_t mode, uint32 semCount,
						struct _sem_t* userSem, struct _sem_t** _usedUserSem);
extern status_t		_kern_realtime_sem_close(sem_id semID,
						struct _sem_t** _deleteUserSem);
extern status_t		_kern_realtime_sem_unlink(const char* name);

extern status_t		_kern_realtime_sem_get_value(sem_id semID, int* value);
extern status_t		_kern_realtime_sem_post(sem_id semID);
extern status_t		_kern_realtime_sem_wait(sem_id semID, bigtime_t timeout);

/* POSIX XSI semaphore syscalls */
extern int			_kern_xsi_semget(key_t key, int numSems, int flags);
extern int			_kern_xsi_semctl(int semID, int semNumber, int command,
						union semun* args);
extern status_t		_kern_xsi_semop(int semID, struct sembuf *semOps,
						size_t numSemOps);

/* POSIX XSI message queue syscalls */
extern int			_kern_xsi_msgctl(int messageQueueID, int command,
						struct msqid_ds *buffer);
extern int			_kern_xsi_msgget(key_t key, int messageQueueFlags);
extern ssize_t		_kern_xsi_msgrcv(int messageQueueID, void *messagePointer,
						size_t messageSize, long messageType, int messageFlags);
extern int			_kern_xsi_msgsnd(int messageQueueID,
						const void *messagePointer, size_t messageSize,
						int messageFlags);

/* team & thread syscalls */
extern thread_id	_kern_load_image(const char* const* flatArgs,
						size_t flatArgsSize, int32 argCount, int32 envCount,
						int32 priority, uint32 flags, port_id errorPort,
						uint32 errorToken);
extern void __NO_RETURN _kern_exit_team(status_t returnValue);
extern status_t		_kern_kill_team(team_id team);
extern team_id		_kern_get_current_team();
extern status_t		_kern_wait_for_team(team_id team, status_t *_returnCode);
extern pid_t		_kern_wait_for_child(thread_id child, uint32 flags,
						siginfo_t* info);
extern status_t		_kern_exec(const char *path, const char* const* flatArgs,
						size_t flatArgsSize, int32 argCount, int32 envCount,
						mode_t umask);
extern thread_id	_kern_fork(void);
extern pid_t		_kern_process_info(pid_t process, int32 which);
extern pid_t		_kern_setpgid(pid_t process, pid_t group);
extern pid_t		_kern_setsid(void);
extern status_t		_kern_change_root(const char *path);

extern thread_id	_kern_spawn_thread(
						struct thread_creation_attributes* attributes);
extern thread_id	_kern_find_thread(const char *name);
extern status_t		_kern_suspend_thread(thread_id thread);
extern status_t		_kern_resume_thread(thread_id thread);
extern status_t		_kern_rename_thread(thread_id thread, const char *newName);
extern status_t		_kern_set_thread_priority(thread_id thread,
						int32 newPriority);
extern status_t		_kern_kill_thread(thread_id thread);
extern void			_kern_exit_thread(status_t returnValue);
extern status_t		_kern_cancel_thread(thread_id threadID,
						void (*cancelFunction)(int));
extern void			_kern_thread_yield(void);
extern status_t		_kern_wait_for_thread(thread_id thread,
						status_t *_returnCode);
extern bool			_kern_has_data(thread_id thread);
extern status_t		_kern_send_data(thread_id thread, int32 code,
						const void *buffer, size_t bufferSize);
extern int32		_kern_receive_data(thread_id *_sender, void *buffer,
						size_t bufferSize);
extern int64		_kern_restore_signal_frame(
						struct signal_frame_data* signalFrameData);

extern status_t		_kern_get_thread_info(thread_id id, thread_info *info);
extern status_t		_kern_get_next_thread_info(team_id team, int32 *cookie,
						thread_info *info);
extern status_t		_kern_get_team_info(team_id id, team_info *info);
extern status_t		_kern_get_next_team_info(int32 *cookie, team_info *info);
extern status_t		_kern_get_team_usage_info(team_id team, int32 who,
						team_usage_info *info, size_t size);
extern status_t		_kern_get_extended_team_info(team_id teamID, uint32 flags,
						void* buffer, size_t size, size_t* _sizeNeeded);

extern status_t		_kern_start_watching_system(int32 object, uint32 flags,
						port_id port, int32 token);
extern status_t		_kern_stop_watching_system(int32 object, uint32 flags,
						port_id port, int32 token);

extern status_t		_kern_block_thread(uint32 flags, bigtime_t timeout);
extern status_t		_kern_unblock_thread(thread_id thread, status_t status);
extern status_t		_kern_unblock_threads(thread_id* threads, uint32 count,
						status_t status);

extern bigtime_t	_kern_estimate_max_scheduling_latency(thread_id thread);

// user/group functions
extern gid_t		_kern_getgid(bool effective);
extern uid_t		_kern_getuid(bool effective);
extern status_t		_kern_setregid(gid_t rgid, gid_t egid,
						bool setAllIfPrivileged);
extern status_t		_kern_setreuid(uid_t ruid, uid_t euid,
						bool setAllIfPrivileged);
extern ssize_t		_kern_getgroups(int groupCount, gid_t* groupList);
extern status_t		_kern_setgroups(int groupCount, const gid_t* groupList);

// signal functions
extern status_t		_kern_send_signal(int32 id, uint32 signal,
						const union sigval* userValue, uint32 flags);
extern status_t		_kern_set_signal_mask(int how, const sigset_t *set,
						sigset_t *oldSet);
extern status_t		_kern_sigaction(int sig, const struct sigaction *action,
						struct sigaction *oldAction);
extern status_t		_kern_sigwait(const sigset_t *set, siginfo_t *info,
						uint32 flags, bigtime_t timeout);
extern status_t		_kern_sigsuspend(const sigset_t *mask);
extern status_t		_kern_sigpending(sigset_t *set);
extern status_t		_kern_set_signal_stack(const stack_t *newStack,
						stack_t *oldStack);

// image functions
extern image_id		_kern_register_image(image_info *info, size_t size);
extern status_t		_kern_unregister_image(image_id id);
extern void			_kern_image_relocated(image_id id);
extern void			_kern_loading_app_failed(status_t error);
extern status_t		_kern_get_image_info(image_id id, image_info *info,
						size_t size);
extern status_t		_kern_get_next_image_info(team_id team, int32 *cookie,
						image_info *info, size_t size);
extern status_t		_kern_read_kernel_image_symbols(image_id id,
						struct Elf32_Sym* symbolTable, int32* _symbolCount,
						char* stringTable, size_t* _stringTableSize,
						addr_t* _imageDelta);

// VFS functions
extern dev_t		_kern_mount(const char *path, const char *device,
						const char *fs_name, uint32 flags, const char *args,
						size_t argsLength);
extern status_t		_kern_unmount(const char *path, uint32 flags);
extern status_t		_kern_read_fs_info(dev_t device, struct fs_info *info);
extern status_t		_kern_write_fs_info(dev_t device, const struct fs_info *info,
						int mask);
extern dev_t		_kern_next_device(int32 *_cookie);
extern status_t		_kern_sync(void);
extern status_t		_kern_entry_ref_to_path(dev_t device, ino_t inode,
						const char *leaf, char *userPath, size_t pathLength);
extern status_t		_kern_normalize_path(const char* userPath,
						bool traverseLink, char* buffer);
extern int			_kern_open_entry_ref(dev_t device, ino_t inode,
						const char *name, int openMode, int perms);
extern int			_kern_open(int fd, const char *path, int openMode,
						int perms);
extern int			_kern_open_dir_entry_ref(dev_t device, ino_t inode,
						const char *name);
extern int			_kern_open_dir(int fd, const char *path);
extern int			_kern_open_parent_dir(int fd, char *name,
						size_t nameLength);
extern status_t		_kern_fcntl(int fd, int op, uint32 argument);
extern status_t		_kern_fsync(int fd);
extern status_t		_kern_flock(int fd, int op);
extern off_t		_kern_seek(int fd, off_t pos, int seekType);
extern status_t		_kern_create_dir_entry_ref(dev_t device, ino_t inode,
						const char *name, int perms);
extern status_t		_kern_create_dir(int fd, const char *path, int perms);
extern status_t		_kern_remove_dir(int fd, const char *path);
extern status_t		_kern_read_link(int fd, const char *path, char *buffer,
						size_t *_bufferSize);
extern status_t		_kern_create_symlink(int fd, const char *path,
						const char *toPath, int mode);
extern status_t		_kern_create_link(int pathFD, const char *path, int toFD,
						const char *toPath, bool traverseLeafLink);
extern status_t		_kern_unlink(int fd, const char *path);
extern status_t		_kern_rename(int oldDir, const char *oldpath, int newDir,
						const char *newpath);
extern status_t		_kern_create_fifo(int fd, const char *path, mode_t perms);
extern status_t		_kern_create_pipe(int *fds);
extern status_t		_kern_access(int fd, const char *path, int mode,
						bool effectiveUserGroup);
extern ssize_t		_kern_select(int numfds, struct fd_set *readSet,
						struct fd_set *writeSet, struct fd_set *errorSet,
						bigtime_t timeout, const sigset_t *sigMask);
extern ssize_t		_kern_poll(struct pollfd *fds, int numFDs,
						bigtime_t timeout);

extern int			_kern_open_attr_dir(int fd, const char *path,
						bool traverseLeafLink);
extern ssize_t		_kern_read_attr(int fd, const char *attribute, off_t pos,
						void *buffer, size_t readBytes);
extern ssize_t		_kern_write_attr(int fd, const char *attribute, uint32 type,
						off_t pos, const void *buffer, size_t readBytes);
extern status_t		_kern_stat_attr(int fd, const char *attribute,
						struct attr_info *attrInfo);
extern int			_kern_open_attr(int fd, const char* path, const char *name,
						uint32 type, int openMode);
extern status_t		_kern_remove_attr(int fd, const char *name);
extern status_t		_kern_rename_attr(int fromFile, const char *fromName,
						int toFile, const char *toName);
extern int			_kern_open_index_dir(dev_t device);
extern status_t		_kern_create_index(dev_t device, const char *name,
						uint32 type, uint32 flags);
extern status_t		_kern_read_index_stat(dev_t device, const char *name,
						struct stat *stat);
extern status_t		_kern_remove_index(dev_t device, const char *name);
extern status_t		_kern_getcwd(char *buffer, size_t size);
extern status_t		_kern_setcwd(int fd, const char *path);
extern int			_kern_open_query(dev_t device, const char *query,
						size_t queryLength, uint32 flags, port_id port,
						int32 token);

// file descriptor functions
extern ssize_t		_kern_read(int fd, off_t pos, void *buffer,
						size_t bufferSize);
extern ssize_t		_kern_readv(int fd, off_t pos, const struct iovec *vecs,
						size_t count);
extern ssize_t		_kern_write(int fd, off_t pos, const void *buffer,
						size_t bufferSize);
extern ssize_t		_kern_writev(int fd, off_t pos, const struct iovec *vecs,
						size_t count);
extern status_t		_kern_ioctl(int fd, uint32 cmd, void *data, size_t length);
extern ssize_t		_kern_read_dir(int fd, struct dirent *buffer,
						size_t bufferSize, uint32 maxCount);
extern status_t		_kern_rewind_dir(int fd);
extern status_t		_kern_read_stat(int fd, const char *path, bool traverseLink,
						struct stat *stat, size_t statSize);
extern status_t		_kern_write_stat(int fd, const char *path,
						bool traverseLink, const struct stat *stat,
						size_t statSize, int statMask);
extern status_t		_kern_close(int fd);
extern int			_kern_dup(int fd);
extern int			_kern_dup2(int ofd, int nfd);
extern status_t		_kern_lock_node(int fd);
extern status_t		_kern_unlock_node(int fd);
extern status_t		_kern_get_next_fd_info(team_id team, uint32 *_cookie,
						struct fd_info *info, size_t infoSize);

// socket functions
extern int			_kern_socket(int family, int type, int protocol);
extern status_t		_kern_bind(int socket, const struct sockaddr *address,
						socklen_t addressLength);
extern status_t		_kern_shutdown_socket(int socket, int how);
extern status_t		_kern_connect(int socket, const struct sockaddr *address,
						socklen_t addressLength);
extern status_t		_kern_listen(int socket, int backlog);
extern int			_kern_accept(int socket, struct sockaddr *address,
						socklen_t *_addressLength);
extern ssize_t		_kern_recv(int socket, void *data, size_t length,
						int flags);
extern ssize_t		_kern_recvfrom(int socket, void *data, size_t length,
						int flags, struct sockaddr *address,
						socklen_t *_addressLength);
extern ssize_t		_kern_recvmsg(int socket, struct msghdr *message,
						int flags);
extern ssize_t		_kern_send(int socket, const void *data, size_t length,
						int flags);
extern ssize_t		_kern_sendto(int socket, const void *data, size_t length,
						int flags, const struct sockaddr *address,
						socklen_t addressLength);
extern ssize_t		_kern_sendmsg(int socket, const struct msghdr *message,
						int flags);
extern status_t		_kern_getsockopt(int socket, int level, int option,
						void *value, socklen_t *_length);
extern status_t		_kern_setsockopt(int socket, int level, int option,
						const void *value, socklen_t length);
extern status_t		_kern_getpeername(int socket, struct sockaddr *address,
						socklen_t *_addressLength);
extern status_t		_kern_getsockname(int socket, struct sockaddr *address,
						socklen_t *_addressLength);
extern int			_kern_sockatmark(int socket);
extern status_t		_kern_socketpair(int family, int type, int protocol,
						int *socketVector);
extern status_t		_kern_get_next_socket_stat(int family, uint32 *cookie,
						struct net_stat *stat);

// node monitor functions
extern status_t		_kern_stop_notifying(port_id port, uint32 token);
extern status_t		_kern_start_watching(dev_t device, ino_t node, uint32 flags,
						port_id port, uint32 token);
extern status_t		_kern_stop_watching(dev_t device, ino_t node, port_id port,
						uint32 token);

// time functions
extern status_t		_kern_set_real_time_clock(bigtime_t time);
extern status_t		_kern_set_timezone(int32 timezoneOffset, const char *name,
						size_t nameLength);
extern status_t		_kern_get_timezone(int32 *_timezoneOffset, char *name,
						size_t nameLength);
extern status_t		_kern_set_real_time_clock_is_gmt(bool isGMT);
extern status_t		_kern_get_real_time_clock_is_gmt(bool *_isGMT);

extern status_t		_kern_get_clock(clockid_t clockID, bigtime_t* _time);
extern status_t		_kern_set_clock(clockid_t clockID, bigtime_t time);

extern bigtime_t	_kern_system_time();
extern status_t		_kern_snooze_etc(bigtime_t time, int timebase, int32 flags,
						bigtime_t* _remainingTime);

extern int32		_kern_create_timer(clockid_t clockID, thread_id threadID,
						uint32 flags, const struct sigevent* event,
						const struct thread_creation_attributes*
							threadAttributes);
extern status_t		_kern_delete_timer(int32 timerID, thread_id threadID);
extern status_t		_kern_get_timer(int32 timerID, thread_id threadID,
						struct user_timer_info* info);
extern status_t		_kern_set_timer(int32 timerID, thread_id threadID,
						bigtime_t startTime, bigtime_t interval, uint32 flags,
						struct user_timer_info* oldInfo);

// area functions
extern area_id		_kern_create_area(const char *name, void **address,
						uint32 addressSpec, size_t size, uint32 lock,
						uint32 protection);
extern status_t		_kern_delete_area(area_id area);
extern area_id		_kern_area_for(void *address);
extern area_id		_kern_find_area(const char *name);
extern status_t		_kern_get_area_info(area_id area, area_info *info);
extern status_t		_kern_get_next_area_info(team_id team, int32 *cookie,
						area_info *info);
extern status_t		_kern_resize_area(area_id area, size_t newSize);
extern area_id		_kern_transfer_area(area_id area, void **_address,
						uint32 addressSpec, team_id target);
extern status_t		_kern_set_area_protection(area_id area,
						uint32 newProtection);
extern area_id		_kern_clone_area(const char *name, void **_address,
						uint32 addressSpec, uint32 protection,
						area_id sourceArea);
extern status_t		_kern_reserve_address_range(addr_t* _address,
						uint32 addressSpec, addr_t size);
extern status_t		_kern_unreserve_address_range(addr_t address, addr_t size);

extern area_id		_kern_map_file(const char *name, void **address,
						uint32 addressSpec, size_t size, uint32 protection,
						uint32 mapping, bool unmapAddressRange, int fd,
						off_t offset);
extern status_t		_kern_unmap_memory(void *address, size_t size);
extern status_t		_kern_set_memory_protection(void *address, size_t size,
						uint32 protection);
extern status_t		_kern_sync_memory(void *address, size_t size, int flags);
extern status_t		_kern_memory_advice(void *address, size_t size,
						uint32 advice);

extern status_t		_kern_get_memory_properties(team_id teamID,
						const void *address, uint32* _protected, uint32* _lock);

/* kernel port functions */
extern port_id		_kern_create_port(int32 queue_length, const char *name);
extern status_t		_kern_close_port(port_id id);
extern status_t		_kern_delete_port(port_id id);
extern port_id		_kern_find_port(const char *port_name);
extern status_t		_kern_get_port_info(port_id id, struct port_info *info);
extern status_t	 	_kern_get_next_port_info(team_id team, int32 *cookie,
						struct port_info *info);
extern ssize_t		_kern_port_buffer_size_etc(port_id port, uint32 flags,
						bigtime_t timeout);
extern int32		_kern_port_count(port_id port);
extern ssize_t		_kern_read_port_etc(port_id port, int32 *msgCode,
						void *msgBuffer, size_t bufferSize, uint32 flags,
						bigtime_t timeout);
extern status_t		_kern_set_port_owner(port_id port, team_id team);
extern status_t		_kern_write_port_etc(port_id port, int32 msgCode,
						const void *msgBuffer, size_t bufferSize, uint32 flags,
						bigtime_t timeout);
extern status_t		_kern_writev_port_etc(port_id id, int32 msgCode,
						const struct iovec *msgVecs, size_t vecCount,
						size_t bufferSize, uint32 flags, bigtime_t timeout);
extern status_t		_kern_get_port_message_info_etc(port_id port,
						port_message_info *info, size_t infoSize, uint32 flags,
						bigtime_t timeout);

// debug support functions
extern status_t		_kern_kernel_debugger(const char *message);
extern void			_kern_debugger(const char *message);
extern int			_kern_disable_debugger(int state);

extern status_t		_kern_install_default_debugger(port_id debuggerPort);
extern port_id		_kern_install_team_debugger(team_id team,
						port_id debuggerPort);
extern status_t		_kern_remove_team_debugger(team_id team);
extern status_t		_kern_debug_thread(thread_id thread);
extern void			_kern_wait_for_debugger(void);

extern status_t		_kern_set_debugger_breakpoint(void *address, uint32 type,
						int32 length, bool watchpoint);
extern status_t		_kern_clear_debugger_breakpoint(void *address,
						bool watchpoint);

extern status_t		_kern_system_profiler_start(
						struct system_profiler_parameters* parameters);
extern status_t		_kern_system_profiler_next_buffer(size_t bytesRead,
						uint64* _droppedEvents);
extern status_t		_kern_system_profiler_stop();
extern status_t		_kern_system_profiler_recorded(
						struct system_profiler_parameters* parameters);

/* atomic_* ops (needed for CPUs that don't support them directly) */
#ifdef ATOMIC_FUNCS_ARE_SYSCALLS
extern int32		_kern_atomic_set(vint32 *value, int32 newValue);
extern int32		_kern_atomic_test_and_set(vint32 *value, int32 newValue,
						int32 testAgainst);
extern int32		_kern_atomic_add(vint32 *value, int32 addValue);
extern int32		_kern_atomic_and(vint32 *value, int32 andValue);
extern int32		_kern_atomic_or(vint32 *value, int32 orValue);
extern int32		_kern_atomic_get(vint32 *value);
#endif	// ATOMIC_FUNCS_ARE_SYSCALLS

#ifdef ATOMIC64_FUNCS_ARE_SYSCALLS
extern int64		_kern_atomic_set64(vint64 *value, int64 newValue);
extern int64		_kern_atomic_test_and_set64(vint64 *value, int64 newValue,
						int64 testAgainst);
extern int64		_kern_atomic_add64(vint64 *value, int64 addValue);
extern int64		_kern_atomic_and64(vint64 *value, int64 andValue);
extern int64		_kern_atomic_or64(vint64 *value, int64 orValue);
extern int64		_kern_atomic_get64(vint64 *value);
#endif	// ATOMIC64_FUNCS_ARE_SYSCALLS

/* System informations */
extern status_t		_kern_get_system_info(system_info *info, size_t size);
extern status_t		_kern_get_system_info_etc(int32 id, void *buffer,
						size_t bufferSize);
extern status_t		_kern_analyze_scheduling(bigtime_t from, bigtime_t until,
						void* buffer, size_t size,
						struct scheduling_analysis* analysis);

/* Debug output */
extern void			_kern_debug_output(const char *message);
extern void			_kern_ktrace_output(const char *message);
extern status_t		_kern_frame_buffer_update(addr_t baseAddress, int32 width,
						int32 height, int32 depth, int32 bytesPerRow);

/* messaging service */
extern area_id		_kern_register_messaging_service(sem_id lockingSem,
						sem_id counterSem);
extern status_t		_kern_unregister_messaging_service();

extern void			_kern_clear_caches(void *address, size_t length,
						uint32 flags);
extern bool			_kern_cpu_enabled(int32 cpu);
extern status_t		_kern_set_cpu_enabled(int32 cpu, bool enabled);

#if defined(__INTEL__) || defined(__x86_64__)
// our only x86 only syscall
extern status_t		_kern_get_cpuid(cpuid_info *info, uint32 eax, uint32 cpu);
#endif


/* Disk Device Manager syscalls */

// iterating, retrieving device/partition data
extern partition_id	_kern_get_next_disk_device_id(int32 *cookie,
						size_t *neededSize);
extern partition_id	_kern_find_disk_device(const char *filename,
						size_t *neededSize);
extern partition_id	_kern_find_partition(const char *filename,
						size_t *neededSize);
extern partition_id	_kern_find_file_disk_device(const char *filename,
						size_t *neededSize);
extern status_t		_kern_get_disk_device_data(partition_id deviceID,
						bool deviceOnly, struct user_disk_device_data *buffer,
						size_t bufferSize, size_t *neededSize);
extern partition_id	_kern_register_file_device(const char *filename);
extern status_t		_kern_unregister_file_device(partition_id deviceID,
						const char *filename);
	// Only a valid deviceID or filename need to be passed. The other one
	// is -1/NULL. If both is given only filename is ignored.
extern status_t		_kern_get_file_disk_device_path(partition_id id,
						char* buffer, size_t bufferSize);

// disk systems
extern status_t		_kern_get_disk_system_info(disk_system_id id,
						struct user_disk_system_info *info);
extern status_t		_kern_get_next_disk_system_info(int32 *cookie,
						struct user_disk_system_info *info);
extern status_t		_kern_find_disk_system(const char *name,
						struct user_disk_system_info *info);

// disk device modification
extern status_t		_kern_defragment_partition(partition_id partitionID,
						int32* changeCounter);
extern status_t		_kern_repair_partition(partition_id partitionID,
						int32* changeCounter, bool checkOnly);
extern status_t		_kern_resize_partition(partition_id partitionID,
						int32* changeCounter, partition_id childID,
						int32* childChangeCounter, off_t size,
						off_t contentSize);
extern status_t		_kern_move_partition(partition_id partitionID,
						int32* changeCounter, partition_id childID,
						int32* childChangeCounter, off_t newOffset,
						partition_id* descendantIDs,
						int32* descendantChangeCounters, int32 descendantCount);
extern status_t		_kern_set_partition_name(partition_id partitionID,
						int32* changeCounter, partition_id childID,
						int32* childChangeCounter, const char* name);
extern status_t		_kern_set_partition_content_name(partition_id partitionID,
						int32* changeCounter, const char* name);
extern status_t		_kern_set_partition_type(partition_id partitionID,
						int32* changeCounter, partition_id childID,
						int32* childChangeCounter, const char* type);
extern status_t		_kern_set_partition_parameters(partition_id partitionID,
						int32* changeCounter, partition_id childID,
						int32* childChangeCounter, const char* parameters);
extern status_t		_kern_set_partition_content_parameters(
						partition_id partitionID, int32* changeCounter,
						const char* parameters);
extern status_t		_kern_initialize_partition(partition_id partitionID,
						int32* changeCounter, const char* diskSystemName,
						const char* name, const char* parameters);
extern status_t		_kern_uninitialize_partition(partition_id partitionID,
						int32* changeCounter);
extern status_t		_kern_create_child_partition(partition_id partitionID,
						int32* changeCounter, off_t offset, off_t size,
						const char* type, const char* name,
						const char* parameters, partition_id* childID,
						int32* childChangeCounter);
extern status_t		_kern_delete_child_partition(partition_id partitionID,
						int32* changeCounter, partition_id childID,
						int32 childChangeCounter);

// disk change notification
extern status_t		_kern_start_watching_disks(uint32 eventMask, port_id port,
						int32 token);
extern status_t		_kern_stop_watching_disks(port_id port, int32 token);


// The end mark for gensyscallinfos.
#ifdef GEN_SYSCALL_INFOS_PROCESSING
#pragma syscalls end
#endif

#undef __NO_RETURN

#ifdef __cplusplus
}
#endif

#endif	/* _SYSTEM_SYSCALLS_H */
