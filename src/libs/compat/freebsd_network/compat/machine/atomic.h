/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_MACHINE_ATOMIC_H_
#define _FBSD_COMPAT_MACHINE_ATOMIC_H_


#include <KernelExport.h>


#define atomic_add_int(ptr, value) \
	atomic_add((int32 *)(ptr), value)

#define atomic_subtract_int(ptr, value) \
	atomic_add((int32 *)(ptr), -value)

#define atomic_load_int(ptr) \
	atomic_get((int32 *)ptr)

#define atomic_set_acq_32(ptr, value) \
	atomic_set_int(ptr, value)

#define atomic_set_int(ptr, value) \
	atomic_or((int32 *)(ptr), value)

#define atomic_readandclear_int(ptr) \
	atomic_set((int32 *)(ptr), 0)

#define atomic_cmpset_int(ptr, old, new) \
	(atomic_test_and_set((int32 *)(ptr), new, old) == (int32)old)

#define atomic_add_32			atomic_add_int
#define atomic_subtract_32		atomic_subtract_int
#define atomic_load_acq_32		atomic_load_int
#define atomic_store_rel_int	atomic_set_acq_32
#define atomic_cmpset_acq_int	atomic_cmpset_int


#define mb()    memory_full_barrier()
#define wmb()   memory_write_barrier_inline()
#define rmb()   memory_read_barrier()

#endif	/* _FBSD_COMPAT_MACHINE_ATOMIC_H_ */
