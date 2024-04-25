/*
 * Copyright 2024, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Jérôme Duval <jerome.duval@gmail.com>
 */


#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>


extern const char *__progname;
static const char *kCommandName = __progname;


// usage
static const char *kUsage =
"Usage: %s [ <options> ] [ command ] [ args ]\n"
"\n"
"Options:\n"
"  --count <n>      - Read n registers.\n"
;


static void
print_usage(bool error)
{
	// print usage
	fprintf((error ? stderr : stdout), kUsage, kCommandName);
}


static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}


int
main(int argc, const char *const *argv)
{
	const char *const *programArgs = NULL;
	int32_t programArgCount = 0;
	uint32_t count = 1;

	// parse arguments
	for (int argi = 1; argi < argc; argi++) {
		const char *arg = argv[argi];
		if (arg[0] == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);
			} else if (strcmp(arg, "--count") == 0) {
				count = strtol(argv[++argi], (char **)NULL, 10);
				if (count == 0 && errno != 0)
					print_usage_and_exit(true);
			} else {
				print_usage_and_exit(true);
			}
		} else {
			programArgs = argv + argi;
			programArgCount = argc - argi;
			break;
		}
	}
	
	bool write = false;
	uint32 value = 0;
	if (programArgCount < 2) {
		print_usage_and_exit(true);
	}
	if (strcmp(programArgs[0], "read") == 0) {
		if (programArgCount >= 4) {
			const char *arg = programArgs[1];
			if (arg[0] == '-') {
				if (strcmp(arg, "--count") == 0) {
					count = strtol(programArgs[2], (char **)NULL, 10);
					if (count == 0 && errno != 0)
						print_usage_and_exit(true);
					programArgCount -= 2;
					programArgs += 2;
				} else {
					print_usage_and_exit(true);
				}
			}
		}
	} else if (strcmp(programArgs[0], "write") == 0) {
		write = true;
	} else
		print_usage_and_exit(true);
	programArgCount--;
	programArgs++;

	area_id area = find_area("intel extreme mmio");
	if (area < 0) {
		fprintf(stderr, "couldn't find intel extreme mmio area\n");
		exit(1);
	}

	void* regs;
	uint32 protection = B_READ_AREA | (write ? B_WRITE_AREA : 0);
	area_id clone = clone_area("clone regs", &regs, B_ANY_ADDRESS, protection, area);
	if (clone < B_OK) {
		fprintf(stderr, "couldn't clone intel extreme mmio area\n");
		exit(1);
	}

	while (programArgCount > 0) {
		const char* arg = programArgs[0];
		programArgCount--;
		programArgs++;
		if (write) {
			if (programArgCount < 1)
				print_usage_and_exit(true);
			const char* val = programArgs[0];
			if (val[0] == '0' && (val[1] == 'x' || val[1] == 'X'))
				value = strtol(val + 2, (char **)NULL, 16);
			else
				value = strtol(val, (char **)NULL, 10);
			if (value == 0 && errno != 0)
				print_usage_and_exit(true);
			programArgCount--;
			programArgs++;
		}
		uint64_t address;
		if (arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) {
			address = strtol(arg + 2, (char **)NULL, 16);
		} else
			address = strtol(arg, (char **)NULL, 10);
		if (address == 0 && errno != 0)
			print_usage_and_exit(true);

		for (uint32_t i = 0; i < count; i++) {
			addr_t addr = (addr_t)regs + address + i * sizeof(uint32);
			if (write) {
				*(uint32*)addr = value;
			} else {
				printf("(0x%08" B_PRIxADDR "): 0x%08" B_PRIx32 "\n", address + i * sizeof(uint32), *(uint32*)addr);
			}
		}
	}

	delete_area(clone);
	exit(0);
}
