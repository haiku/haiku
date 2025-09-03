/*
* Copyright 2023 Haiku, Inc. All Rights Reserved.
* Distributed under the terms of the MIT License.
*/
#ifndef _GNU_SCHED_H_
#define _GNU_SCHED_H_


#include_next <sched.h>
#include <features.h>

#include <stdint.h>
#include <sys/types.h>


#ifdef _DEFAULT_SOURCE


#ifndef CPU_SETSIZE
#define CPU_SETSIZE 1024
#endif


typedef __haiku_uint32 cpuset_mask;

#ifndef _howmany
#define _howmany(x, y) (((x) + ((y) - 1)) / (y))
#endif

#define NCPUSETBITS (sizeof(cpuset_mask) * 8) /* bits per mask */

typedef struct _cpuset {
	cpuset_mask bits[_howmany(CPU_SETSIZE, NCPUSETBITS)];
} cpuset_t;
typedef cpuset_t cpu_set_t;


#define _CPUSET_BITSINDEX(cpu) ((cpu) / NCPUSETBITS)
#define _CPUSET_BIT(cpu) (1L << ((cpu) % NCPUSETBITS))

/* FD_ZERO uses memset */
#include <string.h>


static inline unsigned int
__cpu_count(cpuset_t *set)
{
	unsigned int count = 0;
	unsigned int i;
	for (i = 0; i < _howmany(CPU_SETSIZE, NCPUSETBITS); i++) {
		cpuset_mask mask = set->bits[i];
#if __GNUC__ > 2
		count += __builtin_popcount(mask);
#else
		while (mask > 0) {
			if ((mask & 1) == 1)
				count++;
			mask >>= 1;
		}
#endif
	}
	return count;
}

#define CPU_ZERO(set) memset((set), 0, sizeof(cpuset_t))
#define CPU_SET(cpu, set) ((set)->bits[_CPUSET_BITSINDEX(cpu)] |= _CPUSET_BIT(cpu))
#define CPU_CLR(cpu, set) ((set)->bits[_CPUSET_BITSINDEX(cpu)] &= ~_CPUSET_BIT(cpu))
#define CPU_ISSET(cpu, set) ((set)->bits[_CPUSET_BITSINDEX(cpu)] & _CPUSET_BIT(cpu))
#define CPU_COPY(source, target) (*(target) = *(source))
#define CPU_COUNT(set)	__cpu_count(set)

#ifdef __cplusplus
extern "C" {
#endif

extern int sched_getcpu(void);
extern int sched_setaffinity(pid_t id, size_t cpusetsize, const cpuset_t* mask);
extern int sched_getaffinity(pid_t id, size_t cpusetsize, cpuset_t* mask);

#ifdef __cplusplus
}
#endif


#endif


#endif  /* _GNU_SCHED_H_ */
