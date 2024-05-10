/*
 * Copyright (c) 2018-2021 Maxime Villard, m00nbsd.net
 * All rights reserved.
 *
 * This code is part of the NVMM hypervisor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NVMM_INTERNAL_H_
#define _NVMM_INTERNAL_H_

#ifndef _KERNEL
#error "This file should not be included by userland programs."
#endif

#include "nvmm_os.h"

#define NVMM_MAX_MACHINES	128
#define NVMM_MAX_VCPUS		128
#define NVMM_MAX_HMAPPINGS	32

#if defined(__NetBSD__)
#define NVMM_MAX_RAM		(128ULL * (1 << 30))
#elif defined(__DragonFly__)
#define NVMM_MAX_RAM		(127ULL * 1024ULL * (1 << 30))
#else
#error "OS dependency for NVMM_MAX_RAM required"
#endif

#define NVMM_COMM_PAGE_SIZE	\
	(roundup(sizeof(struct nvmm_comm_page), PAGE_SIZE))

struct nvmm_owner {
	pid_t pid;
};

struct nvmm_cpu {
	/* Shared. */
	bool present;
	nvmm_cpuid_t cpuid;
	os_mtx_t lock;

	/* Comm page. */
	struct nvmm_comm_page *comm;

	/* Last host CPU on which the VCPU ran. */
	int hcpu_last;

	/* Implementation-specific. */
	void *cpudata;
};

struct nvmm_hmapping {
	bool present;
	uintptr_t hva;
	size_t size;
	os_vmobj_t *vmobj;
};

struct nvmm_machine {
	bool present;
	nvmm_machid_t machid;
	time_t time;
	struct nvmm_owner *owner;
	os_rwl_t lock;

	/* Comm */
	os_vmobj_t *commvmobj;

	/* Kernel */
	struct vmspace *vm;
	gpaddr_t gpa_begin;
	gpaddr_t gpa_end;

	/* Host Mappings */
	struct nvmm_hmapping hmap[NVMM_MAX_HMAPPINGS];

	/* CPU */
	volatile unsigned int ncpus;
	struct nvmm_cpu cpus[NVMM_MAX_VCPUS];

	/* Implementation-specific */
	void *machdata;
};

struct nvmm_impl {
	const char *name;
	bool (*ident)(void);
	void (*init)(void);
	void (*fini)(void);
	void (*capability)(struct nvmm_capability *);

	size_t mach_conf_max;
	const size_t *mach_conf_sizes;

	size_t vcpu_conf_max;
	const size_t *vcpu_conf_sizes;

	size_t state_size;

	void (*machine_create)(struct nvmm_machine *);
	void (*machine_destroy)(struct nvmm_machine *);
	int (*machine_configure)(struct nvmm_machine *, uint64_t, void *);

	int (*vcpu_create)(struct nvmm_machine *, struct nvmm_cpu *);
	void (*vcpu_destroy)(struct nvmm_machine *, struct nvmm_cpu *);
	int (*vcpu_configure)(struct nvmm_cpu *, uint64_t, void *);
	void (*vcpu_setstate)(struct nvmm_cpu *);
	void (*vcpu_getstate)(struct nvmm_cpu *);
	int (*vcpu_inject)(struct nvmm_cpu *);
	int (*vcpu_run)(struct nvmm_machine *, struct nvmm_cpu *,
	    struct nvmm_vcpu_exit *);
};

#if defined(__x86_64__)
extern const struct nvmm_impl nvmm_x86_svm;
extern const struct nvmm_impl nvmm_x86_vmx;
#endif

extern struct nvmm_owner nvmm_root_owner;
extern volatile unsigned int nmachines;
extern const struct nvmm_impl *nvmm_impl;

const struct nvmm_impl *nvmm_ident(void);
int	nvmm_init(void);
void	nvmm_fini(void);
int	nvmm_ioctl(struct nvmm_owner *, unsigned long, void *);
void	nvmm_kill_machines(struct nvmm_owner *);

#endif /* _NVMM_INTERNAL_H_ */
