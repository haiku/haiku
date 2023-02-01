/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_SYS_SYSTM_H_
#define _OBSD_COMPAT_SYS_SYSTM_H_


#include_next <sys/systm.h>
#include <sys/kernel.h>

#include <int.h>


#define	INFSLP	UINT64_MAX


static inline void
explicit_bzero(void *buf, size_t len)
{
	memset(buf, 0, len);
}


#define tsleep(identifier, priority, wmesg, timeout) \
	msleep(identifier, &Giant, priority, wmesg, timeout)
#define tsleep_nsec(identifier, priority, wmesg, nsecs) \
	tsleep(identifier, priority, wmesg, USEC_2_TICKS(nsecs / 1000))


#ifdef INVARIANTS
#define DIAGNOSTIC
#endif

#ifdef DIAGNOSTIC
#define KASSERT_OPENBSD(x)				ASSERT_ALWAYS(x)
#define KASSERTMSG(x, format, args...)	ASSERT_ALWAYS_PRINT(x, format, args)
#else
#define KASSERT_OPENBSD(x)
#define KASSERTMSG(x, format, args...)
#endif

#undef KASSERT
#define KASSERT KASSERT_OPENBSD


#define KERNEL_LOCK()	mtx_lock(&Giant)
#define KERNEL_UNLOCK() mtx_unlock(&Giant)


/* #pragma mark - interrupts */

#define	IPL_NONE		0
#define	IPL_HIGH		1

#define	IPL_NET			IPL_NONE

#define	IPL_MPSAFE		0x100

#define	splsoft()		splraise(IPL_SOFT)
#define	splsoftclock()	splraise(IPL_SOFTCLOCK)
#define	splsoftnet()	splraise(IPL_SOFTNET)
#define	splsofttty()	splraise(IPL_SOFTTTY)
#define	splbio()		splraise(IPL_BIO)
#define	splnet()		splraise(IPL_NET)
#define	spltty()		splraise(IPL_TTY)
#define	splvm()			splraise(IPL_VM)
#define	splaudio()		splraise(IPL_AUDIO)
#define	splclock()		splraise(IPL_CLOCK)
#define	splsched()		splraise(IPL_SCHED)
#define	splstatclock()	splraise(IPL_STATCLOCK)
#define	splhigh()		splraise(IPL_HIGH)


static inline int
splraise(int ipl)
{
	if (ipl == IPL_NONE)
		return 0;
	return disable_interrupts();
}


static inline void
splx(int ipl)
{
	restore_interrupts(ipl);
}

static void
splassert(int ipl)
{
	const int ints = are_interrupts_enabled();
	if (ints && ipl != IPL_NONE)
		panic("splassert: interrupts enabled but should be disabled");
	else if (!ints && ipl == IPL_NONE)
		panic("splassert: interrupts disabled but should be enabled");
}


#endif	/* _OBSD_COMPAT_SYS_SYSTM_H_ */
