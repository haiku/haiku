#if !defined(_GRAPHIC_DRIVER_H_)
#define _GRAPHIC_DRIVER_H_

#include <Drivers.h>

/* The API for driver access is C, not C++ */

#ifdef __cplusplus
extern "C" {
#endif

enum {
	B_GET_ACCELERANT_SIGNATURE = B_GRAPHIC_DRIVER_BASE
};

#ifdef __cplusplus
}
#endif

#endif
