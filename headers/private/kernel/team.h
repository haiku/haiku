/*
** Copyright 2004, The OpenBeOS Team. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
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
void team_remove_team(struct team *team);
void team_delete_team(struct team *team);
struct team *team_get_kernel_team(void);
team_id team_get_kernel_team_id(void);
team_id team_get_current_team_id(void);
char **user_team_get_arguments(void);
int user_team_get_arg_count(void);
bool team_is_valid(team_id id);
struct team *team_get_team_struct_locked(team_id id);

// used in syscalls.c
team_id _user_create_team(const char *path, const char *name, char **args, int argc, char **envp, int envc, int priority);
status_t _user_wait_for_team(team_id id, status_t *_returnCode);
status_t _user_kill_team(thread_id thread);
team_id _user_get_current_team(void);

status_t _user_get_team_info(team_id id, team_info *info);
status_t _user_get_next_team_info(int32 *cookie, team_info *info);

// ToDo: please move the "env" setter/getter out of the kernel!
int _user_setenv(const char *name, const char *value, int overwrite);
int _user_getenv(const char *name, char **value);

#ifdef __cplusplus
}
#endif

#endif /* _TIME_H */
