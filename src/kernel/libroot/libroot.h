#include <libroot_types.h>
#ifndef _LIBROOT_H
#define _LIBROOT_H

#ifdef __cplusplus
 "C" {
#endif

area_id create_area(const char *name, void **start_addr, uint32 addr_spec, size_t size, uint32 lock, uint32 protection);
area_id clone_area(const char *name, void **dest_addr, uint32 addr_spec, uint32 protection, area_id source);
area_id	find_area(const char *name);
area_id	area_for(void *addr);
status_t delete_area(area_id id);
status_t resize_area(area_id id, size_t new_size);
status_t set_area_protection(area_id id, uint32 new_protection);

status_t _get_area_info(area_id id, area_info *ainfo, size_t size);
status_t _get_next_area_info(team_id team, int32 *cookie, area_info *ainfo, size_t size);


port_id	create_port(int32 capacity, const char *name);
port_id	find_port(const char *name);
status_t write_port(port_id port, int32 code, const void *buf, size_t buf_size);
status_t read_port(port_id port, int32 *code, void *buf, size_t buf_size);
status_t write_port_etc(port_id port, int32 code, const void *buf, size_t buf_size, uint32 flags, bigtime_t timeout);
status_t read_port_etc(port_id port, int32 *code, void *buf, size_t buf_size, uint32 flags, bigtime_t timeout);
ssize_t	port_buffer_size(port_id port);
ssize_t	port_buffer_size_etc(port_id port, uint32 flags, bigtime_t timeout);
ssize_t	port_count(port_id port);
status_t set_port_owner(port_id port, team_id team); 
status_t close_port(port_id port);
status_t delete_port(port_id port);
status_t _get_port_info(port_id port, port_info *info, size_t size);
status_t _get_next_port_info(team_id team, int32 *cookie, port_info *info, size_t size);

bigtime_t	set_alarm(bigtime_t when, uint32 flags);

status_t	rename_thread(thread_id thread, const char *new_name);
status_t	set_thread_priority (thread_id thread, int32 new_priority);
void		exit_thread(status_t status);
status_t	wait_for_thread (thread_id thread, status_t *thread_return_value);
status_t	on_exit_thread(void (*callback)(void *), void *data);
status_t	_get_thread_info(thread_id thread, thread_info *info, size_t size);
status_t	_get_next_thread_info(team_id tmid, int32 *cookie, thread_info *info, size_t size);
status_t 	_get_team_usage_info(team_id tmid, int32 who, team_usage_info *ti, size_t size);
thread_id	find_thread(const char *name); 


status_t	send_data(thread_id thread, int32 code, const void *buf, size_t buffer_size);
status_t	receive_data(thread_id *sender, void *buf, size_t buffer_size); 
bool		has_data(thread_id thread);
status_t	snooze(bigtime_t microseconds);
status_t	snooze_until(bigtime_t time, int timebase);
status_t	kill_team(team_id team);  /* see also: send_signal() */
status_t	_get_team_info(team_id team, team_info *info, size_t size);
status_t	_get_next_team_info(int32 *cookie, team_info *info, size_t size);

status_t get_cpuid(cpuid_info* info, uint32 eax_register, uint32 cpu_num);
status_t _get_system_info (system_info *returned_info, size_t size);

int32	is_computer_on(void);
double	is_computer_on_fire(void);

uint32		real_time_clock (void);
void		set_real_time_clock (int32 secs_since_jan1_1970);
bigtime_t	real_time_clock_usecs (void);
status_t	set_timezone(char *str);

bigtime_t	system_time (void);     /* time since booting in microseconds */
void	 	debugger (const char *message);
const int   disable_debugger(int state);

#ifdef __cplusplus
}
#endif

#endif		/* ifdef _LIBROOT_H */
