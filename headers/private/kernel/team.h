/*
** Copyright 2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef _TEAM_H
#define _TEAM_H


#include <OS.h>
#include <thread_types.h>


#ifdef __cplusplus
extern "C" {
#endif

int team_init(kernel_args *ka);
team_id team_create_team(const char *path, const char *name, char **args, int argc,
			char **envp, int envc, int priority);
status_t wait_for_team(team_id id, status_t *returnCode);
void team_remove_team(struct team *team, struct process_group **_freeGroup);
void team_delete_team(struct team *team);
void team_delete_process_group(struct process_group *group);
struct team *team_get_kernel_team(void);
team_id team_get_kernel_team_id(void);
team_id team_get_current_team_id(void);
status_t team_get_address_space(team_id id, struct vm_address_space **_addressSpace);
char **user_team_get_arguments(void);
int user_team_get_arg_count(void);
bool team_is_valid(team_id id);
struct team *team_get_team_struct_locked(team_id id);

// used in syscalls.c
thread_id _user_load_image(int32 argCount, const char **args, int32 envCount, const char **envp, int32 priority);
status_t _user_wait_for_team(team_id id, status_t *_returnCode);
status_t _user_kill_team(thread_id thread);
thread_id _user_wait_for_child(thread_id child, uint32 flags, int32 *_reason, status_t *_returnCode);
status_t _user_exec(const char *path, int32 argc, char * const *argv, int32 envCount, char * const *environment);
thread_id _user_fork(void);
team_id _user_get_current_team(void);
pid_t _user_process_info(pid_t process, int32 which);
pid_t _user_setpgid(pid_t process, pid_t group);
pid_t _user_setsid(void);

status_t _user_get_team_info(team_id id, team_info *info);
status_t _user_get_next_team_info(int32 *cookie, team_info *info);

// ToDo: please move the "env" setter/getter out of the kernel!
int _user_setenv(const char *name, const char *value, int overwrite);
int _user_getenv(const char *name, char **value);

#ifdef __cplusplus
}
#endif

#endif /* _TIME_H */
