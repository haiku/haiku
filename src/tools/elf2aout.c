/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2002 Jake Burkholder
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <elf.h>

int elf2aout32(void *v, int fd);
int elf2aout64(void *v, int fd);

#define	xe16toh(x)	((data == ELFDATA2MSB) ? be16toh(x) : le16toh(x))
#define	xe32toh(x)	((data == ELFDATA2MSB) ? be32toh(x) : le32toh(x))
#define	xe64toh(x)	((data == ELFDATA2MSB) ? be64toh(x) : le64toh(x))
#define	htoxe32(x)	((data == ELFDATA2MSB) ? htobe32(x) : htole32(x))

struct exec {
	u_int32_t	a_magic;
	u_int32_t	a_text;
	u_int32_t	a_data;
	u_int32_t	a_bss;
	u_int32_t	a_syms;
	u_int32_t	a_entry;
	u_int32_t	a_trsize;
	u_int32_t	a_drsize;
};

/* we only support OMAGIC */
#define OMAGIC 0407

static void usage(void);

/* parts from NetBSD */

#define	MID_ZERO	0x000	/* unknown - implementation dependent */
#define	MID_SUN010	0x001	/* sun 68010/68020 binary */
#define	MID_SUN020	0x002	/* sun 68020-only binary */

#define	MID_PC386	0x064	/* 386 PC binary. (so quoth BFD) */

#define	MID_I386	0x086	/* i386 BSD binary */
#define	MID_M68K	0x087	/* m68k BSD binary with 8K page sizes */
#define	MID_M68K4K	0x088	/* m68k BSD binary with 4K page sizes */
#define	MID_NS32532	0x089	/* ns32532 */
#define	MID_SPARC	0x08a	/* sparc */
#define	MID_PMAX	0x08b	/* pmax */
#define	MID_VAX1K	0x08c	/* VAX 1K page size binaries */
#define	MID_ALPHA	0x08d	/* Alpha BSD binary */
#define	MID_MIPS	0x08e	/* big-endian MIPS */
#define	MID_ARM6	0x08f	/* ARM6 */
#define	MID_M680002K	0x090	/* m68000 with 2K page sizes */
#define	MID_SH3		0x091	/* SH3 */

#define	MID_POWERPC64	0x094	/* big-endian PowerPC 64 */
#define	MID_POWERPC	0x095	/* big-endian PowerPC */
#define	MID_VAX		0x096	/* VAX */
#define	MID_MIPS1	0x097	/* MIPS1 */
#define	MID_MIPS2	0x098	/* MIPS2 */
#define	MID_M88K	0x099	/* m88k BSD */
#define	MID_HPPA	0x09a	/* HP PARISC */
#define	MID_SH5_64	0x09b	/* LP64 SH5 */
#define	MID_SPARC64	0x09c	/* LP64 sparc */
#define	MID_X86_64	0x09d	/* AMD x86-64 */
#define	MID_SH5_32	0x09e	/* ILP32 SH5 */
#define	MID_IA64	0x09f	/* Itanium */

#define	MID_AARCH64	0x0b7	/* ARM AARCH64 */
#define	MID_OR1K	0x0b8	/* OpenRISC 1000 */
#define	MID_RISCV	0x0b9	/* Risc-V */

#define	MID_HP200	0x0c8	/* hp200 (68010) BSD binary */

#define	MID_HP300	0x12c	/* hp300 (68020+68881) BSD binary */

#define	MID_HPUX800     0x20b   /* hp800 HP-UX binary */
#define	MID_HPUX	0x20c	/* hp200/300 HP-UX binary */

//(ex->e_machine, ex->e_ident[EI_DATA], ex->e_ident[EI_CLASS])
static uint32_t
get_mid(int m, int e, int c)
{
	switch (m) {
	case EM_AARCH64:
		return MID_AARCH64;
	case EM_ALPHA:
		return MID_ALPHA;
	case EM_ARM:
		return MID_ARM6;
	case EM_PARISC:
		return MID_HPPA;
	case EM_386:
		return MID_I386;
	case EM_68K:
		return MID_M68K;
/*	case EM_OR1K:
		return MID_OR1K;*/
	case EM_MIPS:
		if (e == ELFDATA2LSB)
			return MID_PMAX;
		else
			return MID_MIPS;
	case EM_PPC:
		return MID_POWERPC;
	case EM_PPC64:
		return MID_POWERPC64;
		break;
	case EM_RISCV:
		return MID_RISCV;
	case EM_SH:
		return MID_SH3;
	case EM_SPARC:
	case EM_SPARC32PLUS:
	case EM_SPARCV9:
		if (c == ELFCLASS32)
			return MID_SPARC;
		return MID_SPARC64;
	case EM_X86_64:
		return MID_X86_64;
	case EM_VAX:
		return MID_VAX;
	case EM_NONE:
		return MID_ZERO;
	default:
		break;
	}
	return MID_ZERO;
}

int
elf2aout32(void *v, int fd)
{
	Elf32_Half phentsize;
	Elf32_Half phnum;
	Elf32_Word filesz;
	Elf32_Word memsz;
	Elf32_Addr entry;
	Elf32_Off offset;
	Elf32_Off phoff;
	Elf32_Word type;

	Elf32_Phdr *p;
	Elf32_Ehdr *e = v;

	unsigned char data = e->e_ident[EI_DATA];
	struct exec a;
	int i;
	uint32_t mid;

	mid = get_mid(xe16toh(e->e_machine), e->e_ident[EI_DATA], e->e_ident[EI_CLASS]);
	phentsize = xe16toh(e->e_phentsize);
	if (phentsize != sizeof(*p))
		errx(1, "phdr size mismatch");

	entry = xe32toh(e->e_entry);
	phoff = xe32toh(e->e_phoff);
	phnum = xe16toh(e->e_phnum);
	p = (Elf32_Phdr *)((char *)e + phoff);
	bzero(&a, sizeof(a));
	for (i = 0; i < phnum; i++) {
		type = xe32toh(p[i].p_type);
		switch (type) {
		case PT_LOAD:
			if (a.a_magic != 0)
				errx(1, "too many loadable segments");
			filesz = xe32toh(p[i].p_filesz);
			memsz = xe32toh(p[i].p_memsz);
			offset = xe32toh(p[i].p_offset);
			a.a_magic = htoxe32(((uint32_t)mid << 16) | OMAGIC);
			a.a_text = htoxe32(filesz);
			a.a_bss = htoxe32(memsz - filesz);
			a.a_entry = htoxe32(entry);
			if (write(fd, &a, sizeof(a)) != sizeof(a) ||
			    write(fd, (char *)e + offset, filesz) != (ssize_t)filesz)
				err(1, NULL);
			break;
		default:
			break;
		}
	}
	return (0);
}


int
elf2aout64(void *v, int fd)
{
	Elf64_Half phentsize;
	Elf64_Half phnum;
	Elf64_Xword filesz;
	Elf64_Xword memsz;
	Elf64_Addr entry;
	Elf64_Off offset;
	Elf64_Off phoff;
	Elf64_Word type;

	Elf64_Phdr *p;
	Elf64_Ehdr *e = v;

	unsigned char data = e->e_ident[EI_DATA];
	struct exec a;
	int i;
	uint32_t mid;

	mid = get_mid(xe16toh(e->e_machine), e->e_ident[EI_DATA], e->e_ident[EI_CLASS]);
	phentsize = xe16toh(e->e_phentsize);
	if (phentsize != sizeof(*p))
		errx(1, "phdr size mismatch");

	entry = xe64toh(e->e_entry);
	phoff = xe64toh(e->e_phoff);
	phnum = xe16toh(e->e_phnum);
	p = (Elf64_Phdr *)((char *)e + phoff);
	bzero(&a, sizeof(a));
	for (i = 0; i < phnum; i++) {
		type = xe32toh(p[i].p_type);
		switch (type) {
		case PT_LOAD:
			if (a.a_magic != 0)
				errx(1, "too many loadable segments");
			filesz = xe64toh(p[i].p_filesz);
			memsz = xe64toh(p[i].p_memsz);
			offset = xe64toh(p[i].p_offset);
			a.a_magic = htoxe32(((uint32_t)mid << 16) | OMAGIC);
			a.a_text = htoxe32(filesz);
			a.a_bss = htoxe32(memsz - filesz);
			a.a_entry = htoxe32(entry);
			if (write(fd, &a, sizeof(a)) != sizeof(a) ||
			    write(fd, (char *)e + offset, filesz) != (ssize_t)filesz)
				err(1, NULL);
			break;
		default:
			break;
		}
	}
	return (0);
}


/*
 * elf to a.out converter for freebsd/sparc64 bootblocks.
 */
int
main(int ac, char **av)
{
	unsigned char data;
	struct stat sb;
	Elf64_Ehdr *e;
	void *v;
	int efd;
	int fd;
	int c;

	fd = STDIN_FILENO;
	while ((c = getopt(ac, av, "o:")) != -1)
		switch (c) {
		case 'o':
			if ((fd = open(optarg, O_CREAT|O_RDWR, 0644)) < 0)
				err(1, "%s", optarg);
			break;
		case '?':
		default:
			usage();
		}
	ac -= optind;
	av += optind;
	if (ac == 0)
		usage();

	if ((efd = open(*av, O_RDONLY)) < 0 || fstat(efd, &sb) < 0)
		err(1, NULL);
	v = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, efd, 0);
	if ((e = v) == MAP_FAILED)
		err(1, NULL);

	if (!IS_ELF(*e))
		errx(1, "not an elf file");
	if (e->e_ident[EI_CLASS] != ELFCLASS64 && e->e_ident[EI_CLASS] != ELFCLASS32)
		errx(1, "wrong class");
	data = e->e_ident[EI_DATA];
	if (data != ELFDATA2MSB && data != ELFDATA2LSB)
		errx(1, "wrong data format");
	if (e->e_ident[EI_VERSION] != EV_CURRENT)
		errx(1, "wrong elf version");

	if (e->e_ident[EI_CLASS] == ELFCLASS64)
		return elf2aout64(v, fd);
	else
		return elf2aout32(v, fd);
}

static void
usage(void)
{

	fprintf(stderr, "usage: elf2aout [-o outfile] infile\n");
	exit(1);
}
