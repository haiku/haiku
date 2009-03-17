/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_BEOS_KERNEL_EMU_H
#define USERLAND_FS_BEOS_KERNEL_EMU_H

#include "FSCapabilities.h"


// Implemented in BeOSKernelFileSystem. We can't use BeOSKernelFileSystem
// directly in beos_kernel_emu.cpp, since BeOS and Haiku headers would clash.
void get_beos_file_system_node_capabilities(FSVNodeCapabilities& capabilities);


#endif	// USERLAND_FS_BEOS_KERNEL_EMU_H
