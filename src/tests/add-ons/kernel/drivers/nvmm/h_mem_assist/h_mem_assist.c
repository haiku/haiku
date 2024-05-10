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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <machine/segments.h>
#include <machine/psl.h>

#include <nvmm.h>

#ifdef __NetBSD__

#include <machine/pte.h>
#define PAGE_SIZE 4096

#else /* DragonFly */

#include <machine/pmap.h>
#define PTE_P		X86_PG_V	/* 0x001: P (Valid) */
#define PTE_W		X86_PG_RW	/* 0x002: R/W (Read/Write) */
#define PSL_MBO		PSL_RESERVED_DEFAULT	/* 0x00000002 */
#define SDT_SYS386BSY	SDT_SYSBSY	/* 11: system 64-bit TSS busy */

#endif /* __NetBSD__ */

static uint8_t mmiobuf[PAGE_SIZE];
static uint8_t *instbuf;

/* -------------------------------------------------------------------------- */

static void
mem_callback(struct nvmm_mem *mem)
{
	size_t off;

	if (mem->gpa < 0x1000 || mem->gpa + mem->size > 0x1000 + PAGE_SIZE) {
		printf("Out of page\n");
		exit(-1);
	}

	off = mem->gpa - 0x1000;

	printf("-> gpa = %p\n", (void *)mem->gpa);

	if (mem->write) {
		memcpy(mmiobuf + off, mem->data, mem->size);
	} else {
		memcpy(mem->data, mmiobuf + off, mem->size);
	}
}

static int
handle_memory(struct nvmm_machine *mach, struct nvmm_vcpu *vcpu)
{
	int ret;

	ret = nvmm_assist_mem(mach, vcpu);
	if (ret == -1) {
		err(errno, "nvmm_assist_mem");
	}

	return 0;
}

static void
run_machine(struct nvmm_machine *mach, struct nvmm_vcpu *vcpu)
{
	struct nvmm_vcpu_exit *exit = vcpu->exit;

	while (1) {
		if (nvmm_vcpu_run(mach, vcpu) == -1)
			err(errno, "nvmm_vcpu_run");

		switch (exit->reason) {
		case NVMM_VCPU_EXIT_NONE:
			break;

		case NVMM_VCPU_EXIT_RDMSR:
			/* Stop here. */
			return;

		case NVMM_VCPU_EXIT_MEMORY:
			handle_memory(mach, vcpu);
			break;

		case NVMM_VCPU_EXIT_SHUTDOWN:
			printf("Shutting down!\n");
			return;

		default:
			printf("Invalid VMEXIT: 0x%lx\n", exit->reason);
			return;
		}
	}
}

static struct nvmm_assist_callbacks callbacks = {
	.io = NULL,
	.mem = mem_callback
};

/* -------------------------------------------------------------------------- */

struct test {
	const char *name;
	uint8_t *code_begin;
	uint8_t *code_end;
	uint64_t wanted;
	uint64_t off;
};

static void
run_test(struct nvmm_machine *mach, struct nvmm_vcpu *vcpu,
    const struct test *test)
{
	uint64_t *res;
	size_t size;

	size = (size_t)test->code_end - (size_t)test->code_begin;

	memset(mmiobuf, 0, PAGE_SIZE);
	memcpy(instbuf, test->code_begin, size);

	run_machine(mach, vcpu);

	res = (uint64_t *)(mmiobuf + test->off);
	if (*res == test->wanted) {
		printf("Test '%s' passed\n", test->name);
	} else {
		printf("Test '%s' failed, wanted 0x%lx, got 0x%lx\n", test->name,
		    test->wanted, *res);
		errx(-1, "run_test failed");
	}
}

/* -------------------------------------------------------------------------- */

extern uint8_t test1_begin, test1_end;
extern uint8_t test2_begin, test2_end;
extern uint8_t test3_begin, test3_end;
extern uint8_t test4_begin, test4_end;
extern uint8_t test5_begin, test5_end;
extern uint8_t test6_begin, test6_end;
extern uint8_t test7_begin, test7_end;
extern uint8_t test8_begin, test8_end;
extern uint8_t test9_begin, test9_end;
extern uint8_t test10_begin, test10_end;
extern uint8_t test11_begin, test11_end;
extern uint8_t test12_begin, test12_end;
extern uint8_t test13_begin, test13_end;
extern uint8_t test14_begin, test14_end;
extern uint8_t test_64bit_15_begin, test_64bit_15_end;
extern uint8_t test_64bit_16_begin, test_64bit_16_end;

static const struct test tests64[] = {
	{ "64bit test1 - MOV", &test1_begin, &test1_end, 0x3004, 0 },
	{ "64bit test2 - OR",  &test2_begin, &test2_end, 0x16FF, 0 },
	{ "64bit test3 - AND", &test3_begin, &test3_end, 0x1FC0, 0 },
	{ "64bit test4 - XOR", &test4_begin, &test4_end, 0x10CF, 0 },
	{ "64bit test5 - Address Sizes", &test5_begin, &test5_end, 0x1F00, 0 },
	{ "64bit test6 - DMO", &test6_begin, &test6_end, 0xFFAB, 0 },
	{ "64bit test7 - STOS", &test7_begin, &test7_end, 0x00123456, 0 },
	{ "64bit test8 - LODS", &test8_begin, &test8_end, 0x12345678, 0 },
	{ "64bit test9 - MOVS", &test9_begin, &test9_end, 0x12345678, 0 },
	{ "64bit test10 - MOVZXB", &test10_begin, &test10_end, 0x00000078, 0 },
	{ "64bit test11 - MOVZXW", &test11_begin, &test11_end, 0x00005678, 0 },
	{ "64bit test12 - CMP", &test12_begin, &test12_end, 0x00000001, 0 },
	{ "64bit test13 - SUB", &test13_begin, &test13_end, 0x0000000F0000A0FF, 0 },
	{ "64bit test14 - TEST", &test14_begin, &test14_end, 0x00000001, 0 },
	{ "64bit test15 - XCHG", &test_64bit_15_begin, &test_64bit_15_end, 0x123456, 0 },
	{ "64bit test16 - XCHG", &test_64bit_16_begin, &test_64bit_16_end,
	  0x123456, 0 },
	{ NULL, NULL, NULL, -1, 0 }
};

static void
init_seg(struct nvmm_x64_state_seg *seg, int type, int sel)
{
	seg->selector = sel;
	seg->attrib.type = type;
	seg->attrib.s = (type & 0b10000) != 0;
	seg->attrib.dpl = 0;
	seg->attrib.p = 1;
	seg->attrib.avl = 1;
	seg->attrib.l = 1;
	seg->attrib.def = 0;
	seg->attrib.g = 1;
	seg->limit = 0x0000FFFF;
	seg->base = 0x00000000;
}

static void
reset_machine64(struct nvmm_machine *mach, struct nvmm_vcpu *vcpu)
{
	struct nvmm_x64_state *state = vcpu->state;

	if (nvmm_vcpu_getstate(mach, vcpu, NVMM_X64_STATE_ALL) == -1)
		err(errno, "nvmm_vcpu_getstate");

	memset(state, 0, sizeof(*state));

	/* Default. */
	state->gprs[NVMM_X64_GPR_RFLAGS] = PSL_MBO;
	init_seg(&state->segs[NVMM_X64_SEG_CS], SDT_MEMERA, GSEL(GCODE_SEL, SEL_KPL));
	init_seg(&state->segs[NVMM_X64_SEG_SS], SDT_MEMRWA, GSEL(GDATA_SEL, SEL_KPL));
	init_seg(&state->segs[NVMM_X64_SEG_DS], SDT_MEMRWA, GSEL(GDATA_SEL, SEL_KPL));
	init_seg(&state->segs[NVMM_X64_SEG_ES], SDT_MEMRWA, GSEL(GDATA_SEL, SEL_KPL));
	init_seg(&state->segs[NVMM_X64_SEG_FS], SDT_MEMRWA, GSEL(GDATA_SEL, SEL_KPL));
	init_seg(&state->segs[NVMM_X64_SEG_GS], SDT_MEMRWA, GSEL(GDATA_SEL, SEL_KPL));

	/* Blank. */
	init_seg(&state->segs[NVMM_X64_SEG_GDT], 0, 0);
	init_seg(&state->segs[NVMM_X64_SEG_IDT], 0, 0);
	init_seg(&state->segs[NVMM_X64_SEG_LDT], SDT_SYSLDT, 0);
	init_seg(&state->segs[NVMM_X64_SEG_TR], SDT_SYS386BSY, 0);

	/* Protected mode enabled. */
	state->crs[NVMM_X64_CR_CR0] = CR0_PG|CR0_PE|CR0_NE|CR0_TS|CR0_MP|CR0_WP|CR0_AM;

	/* 64bit mode enabled. */
	state->crs[NVMM_X64_CR_CR4] = CR4_PAE;
	state->msrs[NVMM_X64_MSR_EFER] = EFER_LME | EFER_SCE | EFER_LMA;

	/* Stolen from x86/pmap.c */
#define	PATENTRY(n, type)	(type << ((n) * 8))
#define	PAT_UC		0x0ULL
#define	PAT_WC		0x1ULL
#define	PAT_WT		0x4ULL
#define	PAT_WP		0x5ULL
#define	PAT_WB		0x6ULL
#define	PAT_UCMINUS	0x7ULL
	state->msrs[NVMM_X64_MSR_PAT] =
	    PATENTRY(0, PAT_WB) | PATENTRY(1, PAT_WT) |
	    PATENTRY(2, PAT_UCMINUS) | PATENTRY(3, PAT_UC) |
	    PATENTRY(4, PAT_WB) | PATENTRY(5, PAT_WT) |
	    PATENTRY(6, PAT_UCMINUS) | PATENTRY(7, PAT_UC);

	/* Page tables. */
	state->crs[NVMM_X64_CR_CR3] = 0x3000;

	state->gprs[NVMM_X64_GPR_RIP] = 0x2000;

	if (nvmm_vcpu_setstate(mach, vcpu, NVMM_X64_STATE_ALL) == -1)
		err(errno, "nvmm_vcpu_setstate");
}

static void
map_pages64(struct nvmm_machine *mach)
{
	pt_entry_t *L4, *L3, *L2, *L1;
	int ret;

	instbuf = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (instbuf == MAP_FAILED)
		err(errno, "mmap");

	if (nvmm_hva_map(mach, (uintptr_t)instbuf, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");
	ret = nvmm_gpa_map(mach, (uintptr_t)instbuf, 0x2000, PAGE_SIZE,
	    PROT_READ|PROT_EXEC);
	if (ret == -1)
		err(errno, "nvmm_gpa_map");

	L4 = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (L4 == MAP_FAILED)
		err(errno, "mmap");
	L3 = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (L3 == MAP_FAILED)
		err(errno, "mmap");
	L2 = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (L2 == MAP_FAILED)
		err(errno, "mmap");
	L1 = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (L1 == MAP_FAILED)
		err(errno, "mmap");

	if (nvmm_hva_map(mach, (uintptr_t)L4, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");
	if (nvmm_hva_map(mach, (uintptr_t)L3, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");
	if (nvmm_hva_map(mach, (uintptr_t)L2, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");
	if (nvmm_hva_map(mach, (uintptr_t)L1, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");

	ret = nvmm_gpa_map(mach, (uintptr_t)L4, 0x3000, PAGE_SIZE,
	    PROT_READ|PROT_WRITE);
	if (ret == -1)
		err(errno, "nvmm_gpa_map");
	ret = nvmm_gpa_map(mach, (uintptr_t)L3, 0x4000, PAGE_SIZE,
	    PROT_READ|PROT_WRITE);
	if (ret == -1)
		err(errno, "nvmm_gpa_map");
	ret = nvmm_gpa_map(mach, (uintptr_t)L2, 0x5000, PAGE_SIZE,
	    PROT_READ|PROT_WRITE);
	if (ret == -1)
		err(errno, "nvmm_gpa_map");
	ret = nvmm_gpa_map(mach, (uintptr_t)L1, 0x6000, PAGE_SIZE,
	    PROT_READ|PROT_WRITE);
	if (ret == -1)
		err(errno, "nvmm_gpa_map");

	memset(L4, 0, PAGE_SIZE);
	memset(L3, 0, PAGE_SIZE);
	memset(L2, 0, PAGE_SIZE);
	memset(L1, 0, PAGE_SIZE);

	L4[0] = PTE_P | PTE_W | 0x4000;
	L3[0] = PTE_P | PTE_W | 0x5000;
	L2[0] = PTE_P | PTE_W | 0x6000;
	L1[0x2000 / PAGE_SIZE] = PTE_P | PTE_W | 0x2000;
	L1[0x1000 / PAGE_SIZE] = PTE_P | PTE_W | 0x1000;
}

/*
 * 0x1000: MMIO address, unmapped
 * 0x2000: Instructions, mapped
 * 0x3000: L4
 * 0x4000: L3
 * 0x5000: L2
 * 0x6000: L1
 */
static void
test_vm64(void)
{
	struct nvmm_machine mach;
	struct nvmm_vcpu vcpu;
	size_t i;

	if (nvmm_machine_create(&mach) == -1)
		err(errno, "nvmm_machine_create");
	if (nvmm_vcpu_create(&mach, 0, &vcpu) == -1)
		err(errno, "nvmm_vcpu_create");
	nvmm_vcpu_configure(&mach, &vcpu, NVMM_VCPU_CONF_CALLBACKS, &callbacks);
	map_pages64(&mach);

	for (i = 0; tests64[i].name != NULL; i++) {
		reset_machine64(&mach, &vcpu);
		run_test(&mach, &vcpu, &tests64[i]);
	}

	if (nvmm_vcpu_destroy(&mach, &vcpu) == -1)
		err(errno, "nvmm_vcpu_destroy");
	if (nvmm_machine_destroy(&mach) == -1)
		err(errno, "nvmm_machine_destroy");
}

/* -------------------------------------------------------------------------- */

extern uint8_t test_16bit_1_begin, test_16bit_1_end;
extern uint8_t test_16bit_2_begin, test_16bit_2_end;
extern uint8_t test_16bit_3_begin, test_16bit_3_end;
extern uint8_t test_16bit_4_begin, test_16bit_4_end;
extern uint8_t test_16bit_5_begin, test_16bit_5_end;
extern uint8_t test_16bit_6_begin, test_16bit_6_end;

static const struct test tests16[] = {
	{ "16bit test1 - MOV single", &test_16bit_1_begin, &test_16bit_1_end,
	  0x023, 0x10f1 - 0x1000 },
	{ "16bit test2 - MOV dual", &test_16bit_2_begin, &test_16bit_2_end,
	  0x123, 0x10f3 - 0x1000 },
	{ "16bit test3 - MOV dual+disp", &test_16bit_3_begin, &test_16bit_3_end,
	  0x678, 0x10f1 - 0x1000 },
	{ "16bit test4 - Mixed", &test_16bit_4_begin, &test_16bit_4_end,
	  0x1011, 0x10f6 - 0x1000 },
	{ "16bit test5 - disp16-only", &test_16bit_5_begin, &test_16bit_5_end,
	  0x12, 0x1234 - 0x1000 },
	{ "16bit test6 - XCHG", &test_16bit_6_begin, &test_16bit_6_end,
	  0x1234, 0x1234 - 0x1000 },
	{ NULL, NULL, NULL, -1, -1 }
};

static void
reset_machine16(struct nvmm_machine *mach, struct nvmm_vcpu *vcpu)
{
	struct nvmm_x64_state *state = vcpu->state;

	if (nvmm_vcpu_getstate(mach, vcpu, NVMM_X64_STATE_ALL) == -1)
		err(errno, "nvmm_vcpu_getstate");

	state->segs[NVMM_X64_SEG_CS].base = 0;
	state->segs[NVMM_X64_SEG_CS].limit = 0x2FFF;
	state->gprs[NVMM_X64_GPR_RIP] = 0x2000;

	if (nvmm_vcpu_setstate(mach, vcpu, NVMM_X64_STATE_ALL) == -1)
		err(errno, "nvmm_vcpu_setstate");
}

static void
map_pages16(struct nvmm_machine *mach)
{
	int ret;

	instbuf = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (instbuf == MAP_FAILED)
		err(errno, "mmap");

	if (nvmm_hva_map(mach, (uintptr_t)instbuf, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");
	ret = nvmm_gpa_map(mach, (uintptr_t)instbuf, 0x2000, PAGE_SIZE,
	    PROT_READ|PROT_EXEC);
	if (ret == -1)
		err(errno, "nvmm_gpa_map");
}

/*
 * 0x1000: MMIO address, unmapped
 * 0x2000: Instructions, mapped
 */
static void
test_vm16(void)
{
	struct nvmm_machine mach;
	struct nvmm_vcpu vcpu;
	size_t i;

	if (nvmm_machine_create(&mach) == -1)
		err(errno, "nvmm_machine_create");
	if (nvmm_vcpu_create(&mach, 0, &vcpu) == -1)
		err(errno, "nvmm_vcpu_create");
	nvmm_vcpu_configure(&mach, &vcpu, NVMM_VCPU_CONF_CALLBACKS, &callbacks);
	map_pages16(&mach);

	for (i = 0; tests16[i].name != NULL; i++) {
		reset_machine16(&mach, &vcpu);
		run_test(&mach, &vcpu, &tests16[i]);
	}

	if (nvmm_vcpu_destroy(&mach, &vcpu) == -1)
		err(errno, "nvmm_vcpu_destroy");
	if (nvmm_machine_destroy(&mach) == -1)
		err(errno, "nvmm_machine_destroy");
}

/* -------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	if (nvmm_init() == -1)
		err(errno, "nvmm_init");
	test_vm64();
	test_vm16();
	return 0;
}
