#ifndef __UTIL_H
#define __UTIL_H

#include <OS.h>

area_id alloc_mem(void **virt, void **phy, size_t size, uint32 protection, const char *name);
area_id map_mem(void **virt, void *phy, size_t size, uint32 protection, const char *name);

#endif
