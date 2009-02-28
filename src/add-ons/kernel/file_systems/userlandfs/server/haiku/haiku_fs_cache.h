/*
 * Copyright 2004-2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_HAIKU_FS_CACHE_H
#define USERLAND_FS_HAIKU_FS_CACHE_H

//! File System File and Block Caches


#include <fs_cache.h>


namespace UserlandFS {

class HaikuKernelVolume;

namespace HaikuKernelEmu {

// interface for the emulation layer
status_t file_cache_init();
status_t file_cache_register_volume(HaikuKernelVolume* volume);
status_t file_cache_unregister_volume(HaikuKernelVolume* volume);

}	// namespace HaikuKernelEmu
}	// namespace UserlandFS

#endif	/* USERLAND_FS_HAIKU_FS_CACHE_H */
