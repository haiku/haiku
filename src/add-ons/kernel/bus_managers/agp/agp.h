#if !defined(AGP_PROTO_H)
#define AGP_PROTO_H


#include <PCI.h>
#include "AGP.h"

//#define AGP_DEBUG
#ifdef AGP_DEBUG
#define TRACE dprintf
#else
#define TRACE silent
#endif
void silent(const char *, ... );

status_t init(void);
void uninit(void);
void enable_agp (uint32 *command);
long get_nth_agp_info (long index, agp_info *info);


#endif
