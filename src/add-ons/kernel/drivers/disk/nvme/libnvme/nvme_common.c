/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2016 Intel Corporation. All rights reserved.
 *   Copyright(c) 2012-2014 6WIND S.A. All rights reserved.
 *   Copyright (c) 2017, Western Digital Corporation or its affiliates.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "nvme_pci.h"
#include "nvme_common.h"
#include "nvme_mem.h"
#ifndef __HAIKU__
#include "nvme_cpu.h"
#endif

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>

#if defined(NVME_ARCH_X86)
#include <sys/io.h>
#endif
#include <sys/ioctl.h>
#include <sys/types.h>
#ifndef __HAIKU__
#include <linux/fs.h>
#include <execinfo.h>
#endif

/*
 * Trim whitespace from a string in place.
 */
void nvme_str_trim(char *s)
{
	char *p, *q;

	/* Remove header */
	p = s;
	while (*p != '\0' && isspace(*p))
		p++;

	/* Remove tailer */
	q = p + strlen(p);
	while (q - 1 >= p && isspace(*(q - 1))) {
		q--;
		*q = '\0';
	}

	/* if remove header, move */
	if (p != s) {
		q = s;
		while (*p != '\0')
			*q++ = *p++;
		*q = '\0';
	}
}

/*
 * Split string into tokens
 */
int nvme_str_split(char *string, int stringlen,
		   char **tokens, int maxtokens, char delim)
{
	int i, tok = 0;
	int tokstart = 1;

	if (string == NULL || tokens == NULL) {
		errno = EINVAL;
		return -1;
	}

	for (i = 0; i < stringlen; i++) {
		if (string[i] == '\0' || tok >= maxtokens)
			break;
		if (tokstart) {
			tokstart = 0;
			tokens[tok++] = &string[i];
		}
		if (string[i] == delim) {
			string[i] = '\0';
			tokstart = 1;
		}
	}

	return tok;
}

#ifndef __HAIKU__
/*
 * Parse a sysfs (or other) file containing one integer value
 */
int nvme_parse_sysfs_value(const char *filename,
			   unsigned long *val)
{
	FILE *f;
	char buf[BUFSIZ];
	char *end = NULL;

	if ((f = fopen(filename, "r")) == NULL) {
		nvme_err("%s(): cannot open sysfs value %s\n",
			 __func__, filename);
		return -1;
	}

	if (fgets(buf, sizeof(buf), f) == NULL) {
		nvme_err("%s(): cannot read sysfs value %s\n",
			 __func__, filename);
		fclose(f);
		return -1;
	}
	*val = strtoul(buf, &end, 0);
	if ((buf[0] == '\0') || (end == NULL) || (*end != '\n')) {
		nvme_err("%s(): cannot parse sysfs value %s\n",
			 __func__, filename);
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

/*
 * Get a block device block size in Bytes.
 */
ssize_t nvme_dev_get_blocklen(int fd)
{
	uint32_t blocklen = 0;

	if (ioctl(fd, BLKSSZGET, &blocklen) < 0) {
		nvme_err("iioctl BLKSSZGET failed %d (%s)\n",
			 errno,
			 strerror(errno));
		return -1;
	}

	return blocklen;
}
#endif

/*
 * Get a file size in Bytes.
 */
uint64_t nvme_file_get_size(int fd)
{
	struct stat st;

	if (fstat(fd, &st) != 0)
		return 0;

	if (S_ISLNK(st.st_mode))
		return 0;

	if (S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode)) {
#ifndef __HAIKU__
		uint64_t size;
		if (ioctl(fd, BLKGETSIZE64, &size) == 0)
			return size;
		else
#endif
			return 0;
	}

	if (S_ISREG(st.st_mode))
		return st.st_size;

	/* Not REG, CHR or BLK */
	return 0;
}

#ifndef __HAIKU__
/*
 * Dump the stack of the calling core.
 */
static void nvme_dump_stack(void)
{
#define BACKTRACE_SIZE	256
        void *func[BACKTRACE_SIZE];
        char **symb = NULL;
        int size;

        size = backtrace(func, BACKTRACE_SIZE);
        symb = backtrace_symbols(func, size);

        if (symb == NULL)
                return;

        while (size > 0) {
                nvme_crit("%d: [%s]\n", size, symb[size - 1]);
                size --;
        }

        free(symb);
}
#endif

#ifndef __HAIKU__
void
/*
 * call abort(), it will generate a coredump if enabled.
 */
void __nvme_panic(const char *funcname, const char *format, ...)
{
        va_list ap;

        nvme_crit("PANIC in %s():\n", funcname);
        va_start(ap, format);
        nvme_vlog(NVME_LOG_CRIT, format, ap);
        va_end(ap);
        nvme_dump_stack();
        abort();
}
#endif

/**
 * Library initialization: must be run first by any application
 * before calling any libnvme API.
 */
int nvme_lib_init(enum nvme_log_level level,
		  enum nvme_log_facility facility, const char *path)
{
	int ret;

#ifndef __HAIKU__
	/* Set log level and facility first */
	nvme_set_log_level(level);
	nvme_set_log_facility(facility, path);

	/* Gather CPU information */
	ret = nvme_cpu_init();
	if (ret != 0) {
		nvme_crit("Failed to gather CPU information\n");
		goto out;
	}
#endif

	/* PCI subsystem initialization (libpciaccess) */
	ret = nvme_pci_init();
	if (ret != 0) {
		nvme_crit("PCI subsystem initialization failed\n");
		goto out;
	}

	/* Initialize memory management */
	ret = nvme_mem_init();
	if (ret != 0)
		nvme_crit("Memory management initialization failed\n");

out:

	return ret;
}

/*
 * Will be executed automatically last on termination of the user application.
 */
__attribute__((destructor)) void nvme_lib_exit(void)
{

	nvme_ctrlr_cleanup();

	nvme_mem_cleanup();

}
