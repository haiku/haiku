/*
 * Copyright 2004-2011, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TEAM_H
#define _TEAM_H


#include <thread_types.h>


// team notifications
#define TEAM_MONITOR	'_Tm_'
#define TEAM_ADDED		0x01
#define TEAM_REMOVED	0x02
#define TEAM_EXEC		0x04


#ifdef __cplusplus
extern "C" {
#endif

status_t team_init(struct kernel_args *args);
status_t wait_for_team(team_id id, status_t *returnCode);

void team_remove_team(Team *team, pid_t& _signalGroup);
port_id team_shutdown_team(Team *team);
void team_delete_team(Team *team, port_id debuggerPort);

Team *team_get_kernel_team(void);
team_id team_get_kernel_team_id(void);
team_id team_get_current_team_id(void);
status_t team_get_address_space(team_id id,
			struct VMAddressSpace **_addressSpace);
char **user_team_get_arguments(void);
int user_team_get_arg_count(void);
struct job_control_entry* team_get_death_entry(Team *team,
			thread_id child, bool* _deleteEntry);
void team_init_exit_info_on_error(Team* team);
bool team_is_valid(team_id id);
Team *team_get_team_struct_locked(team_id id);
int32 team_max_teams(void);
int32 team_used_teams(void);

thread_id load_image_etc(int32 argCount, const char* const* args,
	const char* const* env, int32 priority, team_id parentID, uint32 flags);

void team_set_job_control_state(Team* team, job_control_state newState,
			Signal* signal);
void team_set_controlling_tty(int32 index);
int32 team_get_controlling_tty();
status_t team_set_foreground_process_group(int32 ttyIndex, pid_t processGroup);
uid_t team_geteuid(team_id id);

status_t start_watching_team(team_id team, void (*hook)(team_id, void *),
			void *data);
status_t stop_watching_team(team_id team, void (*hook)(team_id, void *),
			void *data);

struct user_thread* team_allocate_user_thread(Team* team);
void team_free_user_thread(Team* team, struct user_thread* userThread);

bool team_associate_data(AssociatedData* data);
bool team_dissociate_data(AssociatedData* data);

// used in syscalls.c
thread_id _user_load_image(const char* const* flatArgs, size_t flatArgsSize,
			int32 argCount, int32 envCount, int32 priority, uint32 flags,
			port_id errorPort, uint32 errorToken);
status_t _user_wait_for_team(team_id id, status_t *_returnCode);
void _user_exit_team(status_t returnValue);
status_t _user_kill_team(thread_id thread);
pid_t _user_wait_for_child(thread_id child, uint32 flags, siginfo_t* info,
			team_usage_info* usageInfo);
status_t _user_exec(const char *path, const char* const* flatArgs,
			size_t flatArgsSize, int32 argCount, int32 envCount, mode_t umask);
thread_id _user_fork(void);
team_id _user_get_current_team(void);
pid_t _user_process_info(pid_t process, int32 which);
pid_t _user_setpgid(pid_t process, pid_t group);
pid_t _user_setsid(void);

status_t _user_get_team_info(team_id id, team_info *info);
status_t _user_get_next_team_info(int32 *cookie, team_info *info);
status_t _user_get_team_usage_info(team_id team, int32 who,
			team_usage_info *info, size_t size);
status_t _user_get_extended_team_info(team_id teamID, uint32 flags,
			void* buffer, size_t size, size_t* _sizeNeeded);

#ifdef __cplusplus
}
#endif

#endif /* _TEAM_H */
