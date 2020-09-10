/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_SMP_H
#define KERNEL_SMP_H


#include <arch/atomic.h>
#include <boot/kernel_args.h>
#include <kernel.h>

#include <KernelExport.h>

#include <string.h>


struct kernel_args;


// intercpu messages
enum {
	SMP_MSG_INVALIDATE_PAGE_RANGE = 0,
	SMP_MSG_INVALIDATE_PAGE_LIST,
	SMP_MSG_USER_INVALIDATE_PAGES,
	SMP_MSG_GLOBAL_INVALIDATE_PAGES,
	SMP_MSG_CPU_HALT,
	SMP_MSG_CALL_FUNCTION,
	SMP_MSG_RESCHEDULE
};

enum {
	SMP_MSG_FLAG_ASYNC		= 0x0,
	SMP_MSG_FLAG_SYNC		= 0x1,
	SMP_MSG_FLAG_FREE_ARG	= 0x2,
};

typedef void (*smp_call_func)(addr_t data1, int32 currentCPU, addr_t data2, addr_t data3);

class CPUSet {
public:
	inline				CPUSet();

	inline	void		ClearAll();
	inline	void		SetAll();

	inline	void		SetBit(int32 cpu);
	inline	void		ClearBit(int32 cpu);

	inline	void		SetBitAtomic(int32 cpu);
	inline	void		ClearBitAtomic(int32 cpu);

	inline	bool		GetBit(int32 cpu) const;

	inline	bool		IsEmpty() const;

private:
	static	const int	kArraySize = ROUNDUP(SMP_MAX_CPUS, 32) / 32;

			uint32		fBitmap[kArraySize];
};


#ifdef __cplusplus
extern "C" {
#endif

bool try_acquire_spinlock(spinlock* lock);

status_t smp_init(struct kernel_args *args);
status_t smp_per_cpu_init(struct kernel_args *args, int32 cpu);
status_t smp_init_post_generic_syscalls(void);
bool smp_trap_non_boot_cpus(int32 cpu, uint32* rendezVous);
void smp_wake_up_non_boot_cpus(void);
void smp_cpu_rendezvous(uint32* var);
void smp_send_ici(int32 targetCPU, int32 message, addr_t data, addr_t data2, addr_t data3,
		void *data_ptr, uint32 flags);
void smp_send_multicast_ici(CPUSet& cpuMask, int32 message, addr_t data,
		addr_t data2, addr_t data3, void *data_ptr, uint32 flags);
void smp_send_broadcast_ici(int32 message, addr_t data, addr_t data2, addr_t data3,
		void *data_ptr, uint32 flags);
void smp_send_broadcast_ici_interrupts_disabled(int32 currentCPU, int32 message,
		addr_t data, addr_t data2, addr_t data3, void *data_ptr, uint32 flags);

int32 smp_get_num_cpus(void);
void smp_set_num_cpus(int32 numCPUs);
int32 smp_get_current_cpu(void);

int smp_intercpu_int_handler(int32 cpu);

void call_single_cpu(uint32 targetCPU, void (*func)(void*, int), void* cookie);
void call_single_cpu_sync(uint32 targetCPU, void (*func)(void*, int),
	void* cookie);


#ifdef __cplusplus
}
#endif


inline
CPUSet::CPUSet()
{
	memset(fBitmap, 0, sizeof(fBitmap));
}


inline void
CPUSet::ClearAll()
{
	memset(fBitmap, 0, sizeof(fBitmap));
}


inline void
CPUSet::SetAll()
{
	memset(fBitmap, ~uint8(0), sizeof(fBitmap));
}


inline void
CPUSet::SetBit(int32 cpu)
{
	int32* element = (int32*)&fBitmap[cpu % kArraySize];
	*element |= 1u << (cpu / kArraySize);
}


inline void
CPUSet::ClearBit(int32 cpu)
{
	int32* element = (int32*)&fBitmap[cpu % kArraySize];
	*element &= ~uint32(1u << (cpu / kArraySize));
}


inline void
CPUSet::SetBitAtomic(int32 cpu)
{
	int32* element = (int32*)&fBitmap[cpu % kArraySize];
	atomic_or(element, 1u << (cpu / kArraySize));
}


inline void
CPUSet::ClearBitAtomic(int32 cpu)
{
	int32* element = (int32*)&fBitmap[cpu % kArraySize];
	atomic_and(element, ~uint32(1u << (cpu / kArraySize)));
}


inline bool
CPUSet::GetBit(int32 cpu) const
{
	int32* element = (int32*)&fBitmap[cpu % kArraySize];
	return ((uint32)atomic_get(element) & (1u << (cpu / kArraySize))) != 0;
}


inline bool
CPUSet::IsEmpty() const
{
	for (int i = 0; i < kArraySize; i++) {
		if (fBitmap[i] != 0)
			return false;
	}

	return true;
}


// Unless spinlock debug features are enabled, try to inline
// {acquire,release}_spinlock().
#if !DEBUG_SPINLOCKS && !B_DEBUG_SPINLOCK_CONTENTION


static inline bool
try_acquire_spinlock_inline(spinlock* lock)
{
	return atomic_get_and_set((int32*)lock, 1) == 0;
}


static inline void
acquire_spinlock_inline(spinlock* lock)
{
	if (try_acquire_spinlock_inline(lock))
		return;
	acquire_spinlock(lock);
}


static inline void
release_spinlock_inline(spinlock* lock)
{
	atomic_set((int32*)lock, 0);
}


#define try_acquire_spinlock(lock)	try_acquire_spinlock_inline(lock)
#define acquire_spinlock(lock)		acquire_spinlock_inline(lock)
#define release_spinlock(lock)		release_spinlock_inline(lock)


static inline bool
try_acquire_write_spinlock_inline(rw_spinlock* lock)
{
	return atomic_test_and_set(&lock->lock, 1u << 31, 0) == 0;
}


static inline void
acquire_write_spinlock_inline(rw_spinlock* lock)
{
	if (try_acquire_write_spinlock(lock))
		return;
	acquire_write_spinlock(lock);
}


static inline void
release_write_spinlock_inline(rw_spinlock* lock)
{
	atomic_set(&lock->lock, 0);
}


static inline bool
try_acquire_read_spinlock_inline(rw_spinlock* lock)
{
	uint32 previous = atomic_add(&lock->lock, 1);
	return (previous & (1u << 31)) == 0;
}


static inline void
acquire_read_spinlock_inline(rw_spinlock* lock)
{
	if (try_acquire_read_spinlock(lock))
		return;
	acquire_read_spinlock(lock);
}


static inline void
release_read_spinlock_inline(rw_spinlock* lock)
{
	atomic_add(&lock->lock, -1);
}


#define try_acquire_read_spinlock(lock)	try_acquire_read_spinlock_inline(lock)
#define acquire_read_spinlock(lock)		acquire_read_spinlock_inline(lock)
#define release_read_spinlock(lock)		release_read_spinlock_inline(lock)
#define try_acquire_write_spinlock(lock) \
	try_acquire_write_spinlock(lock)
#define acquire_write_spinlock(lock)	acquire_write_spinlock_inline(lock)
#define release_write_spinlock(lock)	release_write_spinlock_inline(lock)


static inline bool
try_acquire_write_seqlock_inline(seqlock* lock) {
	bool succeed = try_acquire_spinlock(&lock->lock);
	if (succeed)
		atomic_add((int32*)&lock->count, 1);
	return succeed;
}


static inline void
acquire_write_seqlock_inline(seqlock* lock) {
	acquire_spinlock(&lock->lock);
	atomic_add((int32*)&lock->count, 1);
}


static inline void
release_write_seqlock_inline(seqlock* lock) {
	atomic_add((int32*)&lock->count, 1);
	release_spinlock(&lock->lock);
}


static inline uint32
acquire_read_seqlock_inline(seqlock* lock)
{
	return (uint32)atomic_get((int32*)&lock->count);
}


static inline bool
release_read_seqlock_inline(seqlock* lock, uint32 count)
{
	uint32 current = (uint32)atomic_get((int32*)&lock->count);

	return count % 2 == 0 && current == count;
}


#define try_acquire_write_seqlock(lock)	try_acquire_write_seqlock_inline(lock)
#define acquire_write_seqlock(lock)		acquire_write_seqlock_inline(lock)
#define release_write_seqlock(lock)		release_write_seqlock_inline(lock)
#define acquire_read_seqlock(lock)		acquire_read_seqlock_inline(lock)
#define release_read_seqlock(lock, count)	\
	release_read_seqlock_inline(lock, count)


#endif	// !DEBUG_SPINLOCKS && !B_DEBUG_SPINLOCK_CONTENTION


#endif	/* KERNEL_SMP_H */
