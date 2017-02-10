/* Modules Definitions
** 
** Distributed under the terms of the MIT License.
*/

#ifndef _FSSH_MODULE_H
#define _FSSH_MODULE_H


#include "fssh_os.h"


/* Every module exports a list of module_info structures.
 * It defines the interface of the module and the name
 * that is used to access the interface.
 */

typedef struct fssh_module_info {
	const char		*name;
	uint32_t		flags;
	fssh_status_t	(*std_ops)(int32_t, ...);
} fssh_module_info;

/* module standard operations */
#define	FSSH_B_MODULE_INIT		1
#define	FSSH_B_MODULE_UNINIT	2

/* module flags */
#define	FSSH_B_KEEP_LOADED	0x00000001


/* Use the module_dependency structure to let the
 * kernel automatically load modules yet depend on
 * before B_MODULE_INIT is called.
 */

typedef struct fssh_module_dependency {
	const char			*name;
	fssh_module_info	**info;
} fssh_module_dependency;


#ifdef __cplusplus
extern "C" {
#endif

extern fssh_status_t	fssh_get_module(const char *path,
								fssh_module_info **_info);
extern fssh_status_t	fssh_put_module(const char *path);

#ifdef __cplusplus
}
#endif

#endif	/* _FSSH_MODULE_H */
