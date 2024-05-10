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

#define IO_SIZE	128

static char iobuf[IO_SIZE];

static char *databuf;
static uint8_t *instbuf;

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
reset_machine(struct nvmm_machine *mach, struct nvmm_vcpu *vcpu)
{
	struct nvmm_x64_state *state = vcpu->state;

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
map_pages(struct nvmm_machine *mach)
{
	pt_entry_t *L4, *L3, *L2, *L1;
	int ret;

	instbuf = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (instbuf == MAP_FAILED)
		err(errno, "mmap");
	databuf = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (databuf == MAP_FAILED)
		err(errno, "mmap");

	if (nvmm_hva_map(mach, (uintptr_t)instbuf, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");
	if (nvmm_hva_map(mach, (uintptr_t)databuf, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");
	ret = nvmm_gpa_map(mach, (uintptr_t)instbuf, 0x2000, PAGE_SIZE,
	    PROT_READ|PROT_EXEC);
	if (ret == -1)
		err(errno, "nvmm_gpa_map");
	ret = nvmm_gpa_map(mach, (uintptr_t)databuf, 0x1000, PAGE_SIZE,
	    PROT_READ|PROT_WRITE);
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

/* -------------------------------------------------------------------------- */

static size_t iobuf_off = 0;

static void
io_callback(struct nvmm_io *io)
{
	if (io->port != 123) {
		printf("Wrong port\n");
		exit(-1);
	}

	if (io->in) {
		memcpy(io->data, iobuf + iobuf_off, io->size);
	} else {
		memcpy(iobuf + iobuf_off, io->data, io->size);
	}
	iobuf_off += io->size;

}

static int
handle_io(struct nvmm_machine *mach, struct nvmm_vcpu *vcpu)
{
	int ret;

	ret = nvmm_assist_io(mach, vcpu);
	if (ret == -1) {
		err(errno, "nvmm_assist_io");
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

		case NVMM_VCPU_EXIT_IO:
			handle_io(mach, vcpu);
			break;

		case NVMM_VCPU_EXIT_SHUTDOWN:
			printf("Shutting down!\n");
			return;

		default:
			printf("Invalid!\n");
			return;
		}
	}
}

/* -------------------------------------------------------------------------- */

struct test {
	const char *name;
	uint8_t *code_begin;
	uint8_t *code_end;
	const char *wanted;
	bool in;
};

static void
run_test(struct nvmm_machine *mach, struct nvmm_vcpu *vcpu,
    const struct test *test)
{
	size_t size;
	char *res;

	size = (size_t)test->code_end - (size_t)test->code_begin;

	reset_machine(mach, vcpu);

	iobuf_off = 0;
	memset(iobuf, 0, IO_SIZE);
	memset(databuf, 0, PAGE_SIZE);
	memcpy(instbuf, test->code_begin, size);

	if (test->in) {
		strcpy(iobuf, test->wanted);
	} else {
		strcpy(databuf, test->wanted);
	}

	run_machine(mach, vcpu);

	if (test->in) {
		res = databuf;
	} else {
		res = iobuf;
	}

	if (!strcmp(res, test->wanted)) {
		printf("Test '%s' passed\n", test->name);
	} else {
		printf("Test '%s' failed, wanted '%s', got '%s'\n", test->name,
		    test->wanted, res);
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

static const struct test tests[] = {
	{ "test1 - INB", &test1_begin, &test1_end, "12", true },
	{ "test2 - INW", &test2_begin, &test2_end, "1234", true },
	{ "test3 - INL", &test3_begin, &test3_end, "12345678", true },
	{ "test4 - INSB+REP", &test4_begin, &test4_end, "12345", true },
	{ "test5 - INSW+REP", &test5_begin, &test5_end,
	  "Comment est votre blanquette", true },
	{ "test6 - INSL+REP", &test6_begin, &test6_end,
	  "123456789abcdefghijklmnopqrs", true },
	{ "test7 - OUTB", &test7_begin, &test7_end, "12", false },
	{ "test8 - OUTW", &test8_begin, &test8_end, "1234", false },
	{ "test9 - OUTL", &test9_begin, &test9_end, "12345678", false },
	{ "test10 - OUTSB+REP", &test10_begin, &test10_end, "12345", false },
	{ "test11 - OUTSW+REP", &test11_begin, &test11_end,
	  "Ah, Herr Bramard", false },
	{ "test12 - OUTSL+REP", &test12_begin, &test12_end,
	  "123456789abcdefghijklmnopqrs", false },
	{ NULL, NULL, NULL, NULL, false }
};

static struct nvmm_assist_callbacks callbacks = {
	.io = io_callback,
	.mem = NULL
};

/*
 * 0x1000: Data, mapped
 * 0x2000: Instructions, mapped
 * 0x3000: L4
 * 0x4000: L3
 * 0x5000: L2
 * 0x6000: L1
 */
int main(int argc, char *argv[])
{
	struct nvmm_machine mach;
	struct nvmm_vcpu vcpu;
	size_t i;

	if (nvmm_init() == -1)
		err(errno, "nvmm_init");
	if (nvmm_machine_create(&mach) == -1)
		err(errno, "nvmm_machine_create");
	if (nvmm_vcpu_create(&mach, 0, &vcpu) == -1)
		err(errno, "nvmm_vcpu_create");
	nvmm_vcpu_configure(&mach, &vcpu, NVMM_VCPU_CONF_CALLBACKS, &callbacks);
	map_pages(&mach);

	for (i = 0; tests[i].name != NULL; i++) {
		run_test(&mach, &vcpu, &tests[i]);
	}

	return 0;
}
