/* Modules Definitions
** 
** Distributed under the terms of the OpenBeOS License.
*/

#ifndef _MODULE_H
#define _MODULE_H


#include <OS.h>


/* module info structure
 * Every module exports it - modules are identified
 * by the name given in this structure.
 * It exports the functions that the module support.
 */

typedef struct module_info {
	const char	*name;
	uint32		flags;
	status_t	(*std_ops)(int32, ...);
} module_info;

/* module standard operations */
#define	B_MODULE_INIT	1
#define	B_MODULE_UNINIT	2

/* module flags */
#define	B_KEEP_LOADED	0x00000001


#ifdef __cplusplus
extern "C" {
#endif

extern status_t get_module(const char *path, module_info **_info);
extern status_t put_module(const char *path);
extern status_t get_next_loaded_module_name(uint32 *cookie, char *buffer, size_t *_bufferSize);
extern void *open_module_list(const char *prefix);
extern status_t close_module_list(void *cookie);
extern status_t read_next_module_name(void *cookie, char *buffer, size_t *_bufferSize);

#ifdef __cplusplus
}
#endif

#endif	/* _MODULE_H */
