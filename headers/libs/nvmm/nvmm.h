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

#ifndef _LIBNVMM_H_
#define _LIBNVMM_H_

#include <stdint.h>
#include <stdbool.h>

#if defined(__NetBSD__)
#include <dev/nvmm/nvmm.h>
#include <dev/nvmm/nvmm_ioctl.h>
#elif defined(__DragonFly__)
#include <dev/virtual/nvmm/nvmm.h>
#include <dev/virtual/nvmm/nvmm_ioctl.h>
#else
#error "Unsupported OS."
#endif

#define NVMM_USER_VERSION	1

struct nvmm_io;
struct nvmm_mem;

struct nvmm_assist_callbacks {
	void (*io)(struct nvmm_io *);
	void (*mem)(struct nvmm_mem *);
};

struct nvmm_machine {
	nvmm_machid_t machid;
	struct nvmm_comm_page **pages;
	void *areas; /* opaque */
};

struct nvmm_vcpu {
	nvmm_cpuid_t cpuid;
	struct nvmm_assist_callbacks cbs;
	struct nvmm_vcpu_state *state;
	struct nvmm_vcpu_event *event;
	struct nvmm_vcpu_exit *exit;
};

struct nvmm_io {
	struct nvmm_machine *mach;
	struct nvmm_vcpu *vcpu;
	uint16_t port;
	bool in;
	size_t size;
	uint8_t *data;
};

struct nvmm_mem {
	struct nvmm_machine *mach;
	struct nvmm_vcpu *vcpu;
	gpaddr_t gpa;
	bool write;
	size_t size;
	uint8_t *data;
};

#define NVMM_VCPU_CONF_CALLBACKS	NVMM_VCPU_CONF_LIBNVMM_BEGIN

#define NVMM_PROT_READ		0x01
#define NVMM_PROT_WRITE		0x02
#define NVMM_PROT_EXEC		0x04
#define NVMM_PROT_USER		0x08
#define NVMM_PROT_ALL		0x0F
typedef uint64_t nvmm_prot_t;

int nvmm_init(void);
int nvmm_root_init(void);

int nvmm_capability(struct nvmm_capability *);

int nvmm_machine_create(struct nvmm_machine *);
int nvmm_machine_destroy(struct nvmm_machine *);
int nvmm_machine_configure(struct nvmm_machine *, uint64_t, void *);

int nvmm_vcpu_create(struct nvmm_machine *, nvmm_cpuid_t, struct nvmm_vcpu *);
int nvmm_vcpu_destroy(struct nvmm_machine *, struct nvmm_vcpu *);
int nvmm_vcpu_configure(struct nvmm_machine *, struct nvmm_vcpu *, uint64_t,
    void *);
int nvmm_vcpu_setstate(struct nvmm_machine *, struct nvmm_vcpu *, uint64_t);
int nvmm_vcpu_getstate(struct nvmm_machine *, struct nvmm_vcpu *, uint64_t);
int nvmm_vcpu_inject(struct nvmm_machine *, struct nvmm_vcpu *);
int nvmm_vcpu_run(struct nvmm_machine *, struct nvmm_vcpu *);

int nvmm_gpa_map(struct nvmm_machine *, uintptr_t, gpaddr_t, size_t, int);
int nvmm_gpa_unmap(struct nvmm_machine *, uintptr_t, gpaddr_t, size_t);
int nvmm_hva_map(struct nvmm_machine *, uintptr_t, size_t);
int nvmm_hva_unmap(struct nvmm_machine *, uintptr_t, size_t);

int nvmm_gva_to_gpa(struct nvmm_machine *, struct nvmm_vcpu *, gvaddr_t, gpaddr_t *,
    nvmm_prot_t *);
int nvmm_gpa_to_hva(struct nvmm_machine *, gpaddr_t, uintptr_t *,
    nvmm_prot_t *);

int nvmm_assist_io(struct nvmm_machine *, struct nvmm_vcpu *);
int nvmm_assist_mem(struct nvmm_machine *, struct nvmm_vcpu *);

int nvmm_ctl(int, void *, size_t);

int nvmm_vcpu_dump(struct nvmm_machine *, struct nvmm_vcpu *);

#endif /* _LIBNVMM_H_ */
