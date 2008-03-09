#ifndef _FSSH_KERNEL_EXPORT_H
#define _FSSH_KERNEL_EXPORT_H


#include "fssh_defs.h"
#include "fssh_os.h"


#ifdef __cplusplus
extern "C" {
#endif 


/* kernel threads */

extern fssh_thread_id	fssh_spawn_kernel_thread(fssh_thread_func function,
								const char *threadName,  int32_t priority,
								void *arg);

/* misc */

extern fssh_status_t	fssh_user_memcpy(void *dest, const void *source,
								fssh_size_t length);

/* primitive kernel debugging facilities */

extern void			fssh_dprintf(const char *format, ...)	/* just like printf */
							__attribute__ ((format (__printf__, 1, 2)));
extern void			fssh_kprintf(const char *fmt, ...)			/* only for debugger cmds */
							__attribute__ ((format (__printf__, 1, 2)));

extern void 		fssh_dump_block(const char *buffer, int size,
							const char *prefix);

extern void			fssh_panic(const char *format, ...)
							__attribute__ ((format (__printf__, 1, 2)));

extern void			fssh_kernel_debugger(const char *message);	/* enter kernel debugger */
extern uint32_t		fssh_parse_expression(const char *string);	/* utility for debugger cmds */

typedef int (*fssh_debugger_command_hook)(int argc, char **argv);

extern int			fssh_add_debugger_command(const char *name,
							fssh_debugger_command_hook hook, const char *help);
extern int			fssh_remove_debugger_command(char *name,
							fssh_debugger_command_hook hook); 


#ifdef __cplusplus
}
#endif


#endif	// _FSSH_KERNEL_EXPORT_H
