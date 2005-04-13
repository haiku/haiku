#ifndef _DLFCN_H

#include_next <dlfcn.h>

# define DL_CALL_FCT(fctp, args) \
	(*(fctp)) args

#endif
