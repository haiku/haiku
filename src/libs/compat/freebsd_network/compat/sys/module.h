#ifndef _FBSD_COMPAT_SYS_MODULE_H_
#define _FBSD_COMPAT_SYS_MODULE_H_


#include <sys/linker_set.h>


typedef struct module* module_t;

typedef enum modeventtype {
	MOD_LOAD,
	MOD_UNLOAD,
	MOD_SHUTDOWN,
	MOD_QUIESCE
} modeventtype_t;


#define DECLARE_MODULE(name, data, sub, order)

#define MODULE_VERSION(name, version)
#define MODULE_DEPEND(module, mdepend, vmin, vpref, vmax)
#define	MODULE_PNP_INFO(d, b, unique, t, n)

#endif
