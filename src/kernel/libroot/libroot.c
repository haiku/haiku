#include <OS.h>
#include <libroot.h>
#include <Errors.h>
#include <vm_types.h>
#include <syscalls.h>

// Names don't match, but basic function does. Little work needed on addr_spec.
area_id create_area(const char *name, void **start_addr, uint32 addr_spec, size_t size, uint32 lock, uint32 protection)
	{ return sys_vm_create_anonymous_region(name,start_addr,addr_spec,size,lock,protection); }

// Not 100% sure about the REGION_PRIVATE_MAP
area_id clone_area(const char *name, void **dest_addr, uint32 addr_spec, uint32 protection, area_id source)
	{ return sys_vm_clone_region(name,dest_addr, addr_spec,source, REGION_PRIVATE_MAP,protection); }

// TO DO - add a syscall interface
area_id	find_area(const char *name)
	{ return sys_find_region_by_name(name);}

// TO DO
area_id	area_for(void *addr);

// This is ok.
status_t delete_area(area_id id)
	{ return sys_vm_delete_region(id);}

// TO DO
status_t resize_area(area_id id, size_t new_size);
// TO DO
status_t set_area_protection(area_id id, uint32 new_protection);

// TO DO - convert region_info in VM land to area_info...
status_t _get_area_info(area_id id, area_info *ainfo, size_t size)
	{ 
	if (size < sizeof(ainfo)) 
		return B_ERROR; 
	else 
		return sys_vm_get_region_info(id, (void *)ainfo);
	}

// TO DO
status_t _get_next_area_info(team_id team, int32 *cookie, area_info *ainfo, size_t size);


// OK
port_id	create_port(int32 capacity, const char *name)
	{ return sys_port_create(capacity,name); }

// OK
port_id	find_port(const char *name)
	{ return sys_port_find(name); }

// OK
status_t write_port(port_id port, int32 code, const void *buf, size_t buf_size)
	{ return sys_port_write(port, code, buf, buf_size); }

// OK
status_t read_port(port_id port, int32 *code, void *buf, size_t buf_size)
	{ return sys_port_read(port,code,buf,buf_size); }

// Alter flags (?)
status_t write_port_etc(port_id port, int32 code, const void *buf, size_t buf_size, uint32 flags, bigtime_t timeout)
	{ return sys_port_write_etc(port,code,buf,buf_size,flags,timeout); }

// Alter flags (?)
status_t read_port_etc(port_id port, int32 *code, void *buf, size_t buf_size, uint32 flags, bigtime_t timeout)
	{ return sys_port_read_etc(port,code,buf,buf_size,flags,timeout); }

// OK
ssize_t	port_buffer_size(port_id port) { return sys_port_buffer_size(port); }

// Change the flags?
ssize_t	port_buffer_size_etc(port_id port, uint32 flags, bigtime_t timeout) { return sys_port_buffer_size_etc(port,flags,timeout); }

// OK
ssize_t	port_count(port_id port) 
{ return sys_port_count(port); }

// OK
int set_port_owner(port_id port, team_id team) 
{ return sys_port_set_owner(port, team); }

// OK
status_t close_port(port_id port) { return sys_port_close(port); }

// OK
status_t delete_port(port_id port) { return sys_port_delete(port); }

// OK - this works and is necessary for bin compatability.
status_t _get_next_port_info(team_id team, int32 *cookie, port_info *info, size_t size) 
{ return sys_port_get_next_port_info(team,cookie,info); }

// OK
//status_t get_next_port_info (team_id team, int32 *cookie, port_info *info ) 
//{ return sys_port_get_next_port_info(team, cookie, info); }

// OK
//status_t get_port_info(port_id port, port_info *info ) 
//{ return sys_port_get_info(port,info); }

// OK - this works and is necessary for bin compatability.
status_t _get_port_info(port_id port, port_info *info, size_t size) { return sys_port_get_info(port,info); }

// OK
sem_id		create_sem(int32 count, const char *name) 
{ return kern_create_sem(count,name); }

// OK
status_t	delete_sem(sem_id sem) 
{ return kern_delete_sem(sem); }

// OK
status_t	acquire_sem(sem_id sem) 
{ return kern_acquire_sem(sem); }

// Have to modify flags ???
int acquire_sem_etc(sem_id sem, int32 count, int32 flags, bigtime_t timeout)
{ return kern_acquire_sem_etc(sem, count, flags, timeout); }

// OK
status_t	release_sem(sem_id sem)
{ return kern_release_sem(sem); }

// Have to modify flags ???
status_t	release_sem_etc(sem_id sem, int32 count, int32 flags)
{ return kern_release_sem_etc(sem, count, flags); }

// OK
status_t	get_sem_count(sem_id sem, int32 *count)
{ return sys_sem_get_count(sem,count); }

// OK
status_t	set_sem_owner(sem_id sem, team_id team)
{ return sys_set_sem_owner(sem, team); }

// OK
status_t	_get_sem_info(sem_id sem, sem_info *info, size_t size)
{ return kern_get_sem_info(sem,info, size); }

// OK
int _get_next_sem_info(proc_id team, uint32 *cookie, sem_info *info, size_t size)
{ return kern_get_next_sem_info(team,cookie,info, size); }

// TO DO
bigtime_t	set_alarm(bigtime_t when, uint32 flags);

//		case SYSCALL_GET_CURRENT_THREAD_ID: *call_ret = thread_get_current_thread_id(); break;
//		case SYSCALL_THREAD_CREATE_THREAD: *call_ret = user_thread_create_user_thread((char *)arg0, thread_get_current_thread()->proc->id, (addr)arg1, (void *)arg2); break;
//		case SYSCALL_PROC_CREATE_PROC: *call_ret = user_proc_create_proc((const char *)arg0, (const char *)arg1, (char **)arg2, (int )arg3, (int)arg4); break;
//		case SYSCALL_GET_CURRENT_PROC_ID: *call_ret = proc_get_current_proc_id(); break;
//		case SYSCALL_PROC_WAIT_ON_PROC: *call_ret = user_proc_wait_on_proc((proc_id)arg0, (int *)arg1); break;

// OK
status_t	kill_thread(thread_id thread)
	{ return kern_kill_thread(thread); }

// OK
status_t	resume_thread(thread_id thread)
	{ return kern_resume_thread(thread); }

// OK
status_t	suspend_thread(thread_id thread)
	{ return kern_suspend_thread(thread); }

// TO DO
status_t	rename_thread(thread_id thread, const char *new_name);
// TO DO
status_t	set_thread_priority (thread_id thread, int32 new_priority);

// TO DO
void		exit_thread(status_t status)
	{ /* return sys_exit_thread(thread); */ }

// OK
status_t	wait_for_thread (thread_id thread, status_t *thread_return_value)
	{ return sys_thread_wait_on_thread(thread,thread_return_value); }

// TO DO
status_t	on_exit_thread(void (*callback)(void *), void *data);
// TO DO
status_t	_get_thread_info(thread_id thread, thread_info *info, size_t size);
// TO DO
status_t	_get_next_thread_info(team_id tmid, int32 *cookie, thread_info *info, size_t size);
// TO DO
status_t 	_get_team_usage_info(team_id tmid, int32 who, team_usage_info *ti, size_t size);
// TO DO
thread_id	find_thread(const char *name); 

#define get_thread_info(thread, info) _get_thread_info((thread), (info), sizeof(*(info)))
#define get_next_thread_info(tmid, cookie, info) _get_next_thread_info((tmid), (cookie), (info), sizeof(*(info)))
#define get_team_usage_info(tmid, who, info) _get_team_usage_info((tmid), (who), (info), sizeof(*(info)))

// TO DO
status_t	send_data(thread_id thread, int32 code, const void *buf, size_t buffer_size);
// TO DO
status_t	receive_data(thread_id *sender, void *buf, size_t buffer_size); 
// TO DO
bool		has_data(thread_id thread);
// TO DO
status_t	snooze(bigtime_t microseconds);
// TO DO
status_t	snooze_until(bigtime_t time, int timebase);

// OK
status_t	kill_team(team_id team)  
	{ return sys_proc_kill_proc(team); }

// TO DO
status_t	_get_team_info(team_id team, team_info *info, size_t size);
// TO DO
status_t	_get_next_team_info(int32 *cookie, team_info *info, size_t size);
//#define get_team_info(team, info) _get_team_info((team), (info), sizeof(*(info)))
//#define get_next_team_info(cookie, info)   _get_next_team_info((cookie), (info), sizeof(*(info)))

// TO DO
status_t get_cpuid(cpuid_info* info, uint32 eax_register, uint32 cpu_num);
// TO DO
status_t _get_system_info (system_info *returned_info, size_t size);
#define get_system_info(info)  _get_system_info((info), sizeof(*(info)))

int32	is_computer_on(void) {return 1;}
double	is_computer_on_fire(void) {return 0;}

uint32		real_time_clock (void);
// TO DO
void		set_real_time_clock (int32 secs_since_jan1_1970);
// TO DO
bigtime_t	real_time_clock_usecs (void);
// TO DO
status_t	set_timezone(char *str);
// TO DO

// OK
bigtime_t	system_time (void)     /* time since booting in microseconds */
	{ return sys_system_time(); }

void	 	debugger (const char *message);
const int   disable_debugger(int state);
