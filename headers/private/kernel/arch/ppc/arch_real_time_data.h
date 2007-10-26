/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_REAL_TIME_DATA_H
#define _KERNEL_ARCH_REAL_TIME_DATA_H

#include <StorageDefs.h>
#include <SupportDefs.h>


struct ppc_real_time_data {
	vint64	system_time_offset;
};

struct arch_real_time_data {
	struct ppc_real_time_data	data[2];
	vint32						system_time_conversion_factor;
	vint32						version;
		// Since there're no cheap atomic_{set,get,add}64() on PPC 32 (i.e. one
		// that doesn't involve a syscall), we can't have just a single
		// system_time_offset and set/get it atomically.
		// That's why have our data twice. One set is current (indexed by
		// version % 2). When setting the offset, we do that with disabled
		// interrupts and protected by a spinlock. We write the new values
		// into the other array element and increment the version.
		// A reader first reads the version, then the date of interest, and
		// finally rechecks the version. If it hasn't changed in the meantime,
		// the read value is fine, otherwise it runs the whole procedure again.
		//
		// system_time_conversion_factor is currently consider constant,
		// although that is not necessarily true. We simply don't support
		// changing conversion factors at the moment.
};

#endif	/* _KERNEL_ARCH_REAL_TIME_DATA_H */
