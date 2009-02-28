/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_HAIKU_BLOCK_CACHE_H
#define USERLAND_FS_HAIKU_BLOCK_CACHE_H


#include <SupportDefs.h>


namespace UserlandFS {
namespace HaikuKernelEmu {

status_t block_cache_init(void);
size_t block_cache_used_memory();

}	// namespace HaikuKernelEmu
}	// namespace UserlandFS


#endif	// USERLAND_FS_HAIKU_BLOCK_CACHE_H
