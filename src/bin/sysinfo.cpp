/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright (c) 2002, Carlos Hasan, for Haiku.
 *
 * Distributed under the terms of the MIT license.
 */


#include <OS.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cpu_type.h>


// ToDo: -disable_cpu_sn option is not yet implemented
// ToDo: most of this file should go into an architecture dependent source file
#ifdef __INTEL__

struct cache_description {
	uint8		code;
	const char	*description;
} static sIntelCacheDescriptions[] = {
	{0x01, "Instruction TLB: 4k-byte pages, 4-way set associative, 32 entries"},
	{0x02, "Instruction TLB: 4M-byte pages, fully associative, 2 entries"},
	{0x03, "Data TLB: 4k-byte pages, 4-way set associative, 64 entries"},
	{0x04, "Data TLB: 4M-byte pages, 4-way set associative, 8 entries"},
	{0x05, "Data TLB: 4M-byte pages, 4-way set associative, 32 entries"},
	{0x06, "L1 inst cache: 8 KB, 4-way set associative, 32 bytes/line"},
	{0x08, "L1 inst cache: 16 KB, 4-way set associative, 32 bytes/line"},
	{0x0A, "L1 data cache: 8 KB, 2-way set associative, 32 bytes/line"},
	{0x0C, "L1 data cache: 16 KB, 4-way set associative, 32 bytes/line"},
	{0x0D, "L1 data cache: 16 KB, 4-way set associative, 64-bytes/line, ECC"},
	{0x0E, "L1 data cache, 24 KB, 6-way set associative, 64-bytes/line"},
	{0x10, /* IA-64 */ "L1 data cache: 16 KB, 4-way set associative, 32 bytes/line"},
	{0x15, /* IA-64 */ "L1 inst cache: 16 KB, 4-way set associative, 32 bytes/line"},
	{0x1A, /* IA-64 */ "L2 cache: 96 KB, 6-way set associative, 64 bytes/line"},
	{0x21, "L2 cache: 256 KB (MLC), 8-way set associative, 64-bytes/line"},
	{0x22, "L3 cache: 512 KB, 4-way set associative (!), 64 bytes/line, dual-sectored"},
	{0x23, "L3 cache: 1 MB, 8-way set associative, 64 bytes/line, dual-sectored"},
	{0x25, "L3 cache: 2 MB, 8-way set associative, 64 bytes/line, dual-sectored"},
	{0x29, "L3 cache: 4 MB, 8-way set associative, 64 bytes/line, dual-sectored"},
	{0x2c, "L1 data cache: 32 KB, 8-way set associative, 64 bytes/line"},
	{0x30, "L1 inst cache: 32 KB, 8-way set associative, 64 bytes/line"},
	{0x39, "L2 cache: 128 KB, 4-way set associative, 64 bytes/line, sectored"},
	{0x3A, "L2 cache: 192 KB, 4-way set associative, 64 bytes/line, sectored"},
	{0x3B, "L2 cache: 128 KB, 2-way set associative, 64 bytes/line, sectored"},
	{0x3C, "L2 cache: 256 KB, 4-way set associative, 64 bytes/line, sectored"},
	{0x3D, "L2 cache: 384 KB, 6-way set associative, 64 bytes/line, sectored"},
	{0x3E, "L2 cache: 512 KB, 4-way set associative, 64 bytes/line, sectored"},
	{0x40, NULL /*"No integrated L2 cache (P6 core) or L3 cache (P4 core)"*/},
		// this one is separately handled
	{0x41, "L2 cache: 128 KB, 4-way set associative, 32 bytes/line"},
	{0x42, "L2 cache: 256 KB, 4-way set associative, 32 bytes/line"},
	{0x43, "L2 cache: 512 KB, 4-way set associative, 32 bytes/line"},
	{0x44, "L2 cache: 1024 KB, 4-way set associative, 32 bytes/line"},
	{0x45, "L2 cache: 2048 KB, 4-way set associative, 32 bytes/line"},
	{0x46, "L3 cache: 4096 KB, 4-way set associative, 64 bytes/line"},
	{0x47, "L3 cache: 8192 KB, 8-way set associative, 64 bytes/line"},
	{0x48, "L2 cache: 3072 KB, 12-way set associative, 64 bytes/line, unified on-die"},
	// 0x49 requires special handling, either 4M L3 (Xeon MP, 0F06; otherwise 4M L2
	{0x4A, "L3 cache: 6144 KB, 12-way set associative, 64 bytes/line"},
	{0x4B, "L3 cache: 8192 KB, 16-way set associative, 64 bytes/line"},
	{0x4C, "L3 cache: 12288 KB, 12-way set associative, 64 bytes/line"},
	{0x4D, "L3 cache: 16384 KB, 16-way set associative, 64 bytes/line"},
	{0x4E, "L2 cache: 6144 KB, 24-way set associative, 64 bytes/line"},
	{0x4F, "Inst TLB, 4K-bytes pages, 32 entries"},
	{0x50, "Inst TLB: 4K/4M/2M-bytes pages, fully associative, 64 entries"},
	{0x51, "Inst TLB: 4K/4M/2M-bytes pages, fully associative, 128 entries"},
	{0x52, "Inst TLB: 4K/4M/2M-bytes pages, fully associative, 256 entries"},
	{0x55, "Inst TLB: 2M/4M-bytes pages, fully associative, 7 entries"},
	{0x56, "L1 Data TLB: 4M-bytes pages, 4-way set associative, 16 entries"},
	{0x57, "L1 Data TLB: 4K-bytes pages, 4-way set associative, 16 entries"},
	{0x59, "L0 Data TLB, 4K-bytes pages, fully associative, 16 entries"},
	{0x5A, "Data TLB: 2M/4M-bytes pages, 4-way set associative, 32 entries"},
	{0x5B, "Data TLB: 4K/4M-bytes pages, fully associative, 64 entries"},
	{0x5C, "Data TLB: 4K/4M-bytes pages, fully associative, 128 entries"},
	{0x5D, "Data TLB: 4K/4M-bytes pages, fully associative, 256 entries"},
	{0x66, "L1 data cache: 8 KB, 4-way set associative, 64 bytes/line, sectored"},
	{0x67, "L1 data cache: 16 KB, 4-way set associative, 64 bytes/line, sectored"},
	{0x68, "L1 data cache: 32 KB, 4-way set associative, 64 bytes/line, sectored"},
	{0x70, "Inst trace cache: 12K µOPs, 8-way set associative"},
	{0x71, "Inst trace cache: 16K µOPs, 8-way set associative"},
	{0x72, "Inst trace cache: 32K µOPs, 8-way set associative"},
	{0x77, /* IA-64 */ "L1 inst cache: 16 KB, 4-way set associative, 64 bytes/line, sectored"},
	{0x79, "L2 cache: 128 KB, 8-way set associative, 64 bytes/line, dual-sectored"},
	{0x7A, "L2 cache: 256 KB, 8-way set associative, 64 bytes/line, dual-sectored"},
	{0x7B, "L2 cache: 512 KB, 8-way set associative, 64 bytes/line, dual-sectored"},
	{0x7C, "L2 cache: 1024 KB, 8-way set associative, 64 bytes/line, dual-sectored"},
	{0x7D, "L2 cache: 2048 KB, 8-way set associative, 64 bytes/line"},
	{0x7E, /* IA-64 */ "L2 cache: 256 KB, 8-way set associative, 128 bytes/line, sectored"},
	{0x7F, "L2 cache: 512 KB, 2-way set associative, 64 bytes/line"},
	{0x81, "L2 cache: 128 KB, 8-way set associative, 32 bytes/line"},
	{0x82, "L2 cache: 256 KB, 8-way set associative, 32 bytes/line"},
	{0x83, "L2 cache: 512 KB, 8-way set associative, 32 bytes/line"},
	{0x84, "L2 cache: 1024 KB, 8-way set associative, 32 bytes/line"},
	{0x85, "L2 cache: 2048 KB, 8-way set associative, 32 bytes/line"},
	{0x86, "L2 cache: 512 KB, 4-way set associative, 64 bytes/line"},
	{0x87, "L2 cache: 1024 KB, 8-way set associative, 64 bytes/line"},
	{0x88, /* IA-64 */ "L3 cache: 2 MB, 4-way set associative, 64 bytes/line"},
	{0x89, /* IA-64 */ "L3 cache: 4 MB, 4-way set associative, 64 bytes/line"},
	{0x8A, /* IA-64 */ "L3 cache: 8 MB, 4-way set associative, 64 bytes/line"},
	{0x8D, /* IA-64 */ "L3 cache: 3 MB, 12-way set associative, 128 bytes/line"},
	{0x90, /* IA-64 */ "Inst TLB: 4K-256Mbytes pages, fully associative, 64 entries"},
	{0x96, /* IA-64 */ "L1 data TLB: 4K-256M bytes pages, fully associative, 32 entries"},
	{0x9B, /* IA-64 */ "L2 data TLB: 4K-256M bytes pages, fully associative, 96 entries"},
//	{0x70, "Cyrix specific: Code and data TLB: 4k-bytes pages, 4-way set associative, 32 entries"},
//	{0x74, "Cyrix specific: ???"},
//	{0x77, "Cyrix specific: ???"},
	{0x80, /* Cyrix specific */ "L1 cache: 16 KB, 4-way set associative, 16 bytes/line"},
//	{0x82, "Cyrix specific: ???"},
//	{0x84, "Cyrix specific: ???"},
	{0xB0, "Inst TLB: 4K-bytes pages, 4-way set associative, 128 entries"},
	{0xB1, "Inst TLB: 2M-bytes pages, 4-way set associative, 8 entries OR 4M, 4-way, 4 entries"},
		// Intel doesn't give any details how to determine which of the two options is the case
		// as per Intel Application Note 485, November 2008.
	{0xB2, "Inst TLB: 4K-bytes pages, 4-way set associative, 64 entries"},
	{0xB3, "Data TLB: 4K-bytes pages, 4-way set associative, 128 entries"},
	{0xB4, "Data TLB: 4K-bytes pages, 4-way set associative, 256 entries"},
	{0xBA, "Data TLB, 4K-bytes pages, 4-way set associative, 64 entries"},
	{0xC0, "Data TLB, 4K-4M bytes pages, 4-way set associative, 8 entries"},
	{0xCA, "Shared 2nd-level TLB: 4K, 4-way set associative, 512 entries"},
	{0xD0, "L3 cache: 512 KB, 4-way set associative, 64-bytes/line"},
	{0xD1, "L3 cache: 1024 KB, 4-way set associative, 64-bytes/line"},
	{0xD2, "L3 cache: 2048 KB, 4-way set associative, 64-bytes/line"},
	{0xD6, "L3 cache: 1024 KB, 8-way set associative, 64-bytes/line"},
	{0xD7, "L3 cache: 2048 KB, 8-way set associative, 64-bytes/line"},
	{0xD8, "L3 cache: 4096 KB, 8-way set associative, 64-bytes/line"},
	{0xDC, "L3 cache: 2048 KB, 12-way set associative, 64-bytes/line"},
	{0xDD, "L3 cache: 4096 KB, 12-way set associative, 64-bytes/line"},
	{0xDE, "L3 cache: 8192 KB, 12-way set associative, 64-bytes/line"},
	{0xE2, "L3 cache: 2048 KB, 16-way set associative, 64-bytes/line"},
	{0xE3, "L3 cache: 4096 KB, 16-way set associative, 64-bytes/line"},
	{0xE4, "L3 cache: 8192 KB, 16-way set associative, 64-bytes/line"},
	{0xF0, "64-byte Prefetching"},
	{0xF1, "128-byte Prefetching"},
	{0, NULL}
};


static void
print_intel_cache_descriptors(enum cpu_types type, cpuid_info *info)
{
	uint8 cacheDescriptors[15];	// Max

	int maxDesc = 0;
	int i = 0;

	// put valid values into array
	if ((info->regs.eax & 0x80000000) == 0) {
		// eax is valid, include values
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
	} else {
		i += 3;
	}
	if ((info->regs.ebx & 0x80000000) == 0) {
		// ebx is valid, include values
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
	} else {
		i += 4;
	}
	if ((info->regs.edx & 0x80000000) == 0) {
		// edx is valid, include values
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
	} else {
		i += 4;
	}
	if ((info->regs.ecx & 0x80000000) == 0) {
		// ecx is valid, include values
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
		cacheDescriptors[maxDesc++] = info->eax_2.cache_descriptors[i++];
	}

	putchar('\n');

	for (int i = 0; i < maxDesc; i++) {
		// ignore NULL descriptors
		if (cacheDescriptors[i] == 0)
			continue;

		int j;
		for (j = 0; sIntelCacheDescriptions[j].code; j++) {
			if (cacheDescriptors[i] == sIntelCacheDescriptions[j].code) {
				if (cacheDescriptors[i] == 0x40) {
					printf("\tNo integrated L%u cache\n",
						type >= B_CPU_INTEL_PENTIUM_IV
						&& (type & B_CPU_x86_VENDOR_MASK) == B_CPU_INTEL_x86 ?
							3 : 2);
				} else
					printf("\t%s\n", sIntelCacheDescriptions[j].description);
				break;
			}
		}

		// Reached the end without finding a descriptor
		if (sIntelCacheDescriptions[j].code == 0)
			printf("\tUnknown cache descriptor 0x%02x\n", cacheDescriptors[i]);
	}
}

#endif	// __INTEL__


static void
print_TLB(uint32 reg, const char *pages)
{
	int entries[2];
	int ways[2];
	const char *name[2] = { "Inst TLB", "Data TLB" };

	entries[0] = (reg & 0xff);
	ways[0] = ((reg >> 8) & 0xff);
	entries[1] = ((reg >> 16) & 0xff);
	ways[1] = ((reg >> 24) & 0xff);

	for (int num = 0; num < 2; num++) {
		printf("\t%s: %s%s%u entries, ", name[num],
			pages ? pages : "", pages ? " pages, " : "", entries[num]);

		if (ways[num] == 0xff)
			printf("fully associative\n");
		else
			printf("%u-way set associative\n", ways[num]);
	}
}


static void
print_level2_cache(uint32 reg, const char *name)
{
	uint32 size = (reg >> 16) & 0xffff;
	uint32 ways = (reg >> 12) & 0xf;
	uint32 linesPerTag = (reg >> 8) & 0xf;		// intel does not define this
	uint32 lineSize = reg & 0xff;

	printf("\t%s: %lu KB, ", name, size);
	if (ways == 0xf)
		printf("fully associative, ");
	else if (ways == 0x1)
		printf("direct-mapped, ");
	else
		printf("%lu-way set associative, ", 1UL << (ways / 2));
	printf("%lu lines/tag, %lu bytes/line\n", linesPerTag, lineSize);
}


static void
print_level1_cache(uint32 reg, const char *name)
{
	uint32 size = (reg >> 24) & 0xff;
	uint32 ways = (reg >> 16) & 0xff;
	uint32 linesPerTag = (reg >> 8) & 0xff;
	uint32 lineSize = reg & 0xff;

	printf("\t%s: %lu KB, ", name, size);
	if (ways == 0xff)
		printf("fully associative, ");
	else
		printf("%lu-way set associative, ", ways);
	printf("%lu lines/tag, %lu bytes/line\n", linesPerTag, lineSize);
}


#ifdef __INTEL__

static void
print_cache_desc(int32 cpu)
{
	cpuid_info info;
	get_cpuid(&info, 0x80000005, cpu);

	putchar('\n');

	if (info.regs.eax)
		print_TLB(info.regs.eax, info.regs.ebx ? "2M/4M-byte" : NULL);
	if (info.regs.ebx)
		print_TLB(info.regs.ebx, info.regs.eax ? "4K-byte" : NULL);

	print_level1_cache(info.regs.ecx, "L1 inst cache");
	print_level1_cache(info.regs.edx, "L1 data cache");

	get_cpuid(&info, 0x80000006, cpu);
	print_level2_cache(info.regs.ecx, "L2 cache");
}

static void
print_intel_cache_desc(int32 cpu)
{
	cpuid_info info;
	
	// A second parameters needs to be passed to CPUID which determines the cache level to query
	get_cpuid(&info, 0x00000004, cpu);

	putchar('\n');

	uint32 type = info.regs.eax & 0xf;
	uint32 level = (info.regs.eax & 0x70) >> 4;
	bool isFullyAssoc = info.regs.eax & 0x100;

	uint32 lineSize = (info.regs.ebx & 0xfff) + 1;
	uint32 linesPerTag = ((info.regs.ebx & 0x3ff000) >> 12) + 1;
	uint32 ways = ((info.regs.ebx & 0xffc00000) >> 22) + 1;

	uint32 sets = info.regs.ecx;

	printf("\tL%ld ",level);

	switch (type) {
		case 1: printf("Data cache "); break;
		case 2: printf("Inst cache "); break;
		case 3: printf("Unified cache "); break;
		default: break;
	}

	if (isFullyAssoc)
		printf("fully associative, ");
	else
		printf("%lu-way set associative, ", ways);
	printf("%lu lines/tag, %lu bytes/line\n", linesPerTag, lineSize);

	get_cpuid(&info, 0x80000006, cpu);
	print_level2_cache(sets, "L2 cache");
}


static void
print_transmeta_features(uint32 features)
{
	if (features & (1 << 16))
		printf("\t\tFCMOV\n");
}

#endif	// __INTEL__


static void
print_amd_power_management_features(uint32 features)
{
	static const char *kFeatures[6] = {
		"TS", "FID", "VID", "TTP", "TM", "STC",
	};
	int32 found = 4;

	printf("\tPower Management Features:");

	for (int32 i = 0; i < 6; i++) {
		if ((features & (1UL << i)) && kFeatures[i] != NULL) {
			printf("%s%s", found == 0 ? "\t\t" : " ", kFeatures[i]);
			found++;
			if (found > 0 && (found % 16) == 0) {
				putchar('\n');
				found = 0;
			}
		}
	}

	if (found != 0)
		putchar('\n');
}


static void
print_amd_features(uint32 features)
{
	static const char *kFeatures[32] = {
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, "SCE", NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, "NX", NULL, "AMD-MMX", NULL,
		NULL, "FFXSTR", NULL, "RDTSCP", NULL, "64", "3DNow+", "3DNow!"
	};
	int32 found = 0;

	for (int32 i = 0; i < 32; i++) {
		if ((features & (1UL << i)) && kFeatures[i] != NULL) {
			printf("%s%s", found == 0 ? "\t\t" : " ", kFeatures[i]);
			found++;
			if (found > 0 && (found % 16) == 0) {
				putchar('\n');
				found = 0;
			}
		}
	}

	if (found != 0)
		putchar('\n');
}


static void
print_extended_features(uint32 features)
{
	static const char *kFeatures[32] = {
		"SSE3", "PCLMULDQ", "DTES64", "MONITOR", "DS-CPL", "VMX", "SMX", "EST",
		"TM2", "SSSE3", "CNTXT-ID", NULL, NULL, "CX16", "xTPR", "PDCM",
		NULL, NULL, "DCA", "SSE4.1", "SSE4.2", "x2APIC", "MOVEB", "POPCNT",
		NULL, "AES", "XSAVE", "OSXSAVE", NULL, NULL, NULL, NULL
	};
	int32 found = 0;

	for (int32 i = 0; i < 32; i++) {
		if ((features & (1UL << i)) && kFeatures[i] != NULL) {
			printf("%s%s", found == 0 ? "\t\t" : " ", kFeatures[i]);
			found++;
			if (found > 0 && (found % 16) == 0) {
				putchar('\n');
				found = 0;
			}
		}
	}

	if (found != 0)
		putchar('\n');
}


static void
print_features(uint32 features)
{
	static const char *kFeatures[32] = {
		"FPU", "VME", "DE", "PSE",
		"TSC", "MSR", "PAE", "MCE",
		"CX8", "APIC", NULL, "SEP",
		"MTRR", "PGE", "MCA", "CMOV",
		"PAT", "PSE36", "PSN", "CFLUSH",
		NULL, "DS", "ACPI", "MMX",
		"FXSTR", "SSE", "SSE2", "SS",
		"HTT", "TM", NULL, "PBE",
	};
	int32 found = 0;

	for (int32 i = 0; i < 32; i++) {
		if ((features & (1UL << i)) && kFeatures[i] != NULL) {
			printf("%s%s", found == 0 ? "\t\t" : " ", kFeatures[i]);
			found++;
			if (found > 0 && (found % 16) == 0) {
				putchar('\n');
				found = 0;
			}
		}
	}

	if (found != 0)
		putchar('\n');
}


#ifdef __INTEL__

static void
print_processor_signature(system_info *sys_info, cpuid_info *info, const char *prefix)
{

	if ((sys_info->cpu_type & B_CPU_x86_VENDOR_MASK) == B_CPU_AMD_x86) {
		printf("\t%s%sype %lu, family %lu, model %u, stepping %lu, features 0x%08lx\n",
			prefix ? prefix : "", prefix && prefix[0] ? "t" : "T",
			info->eax_1.type,
			info->eax_1.family + (info->eax_1.family == 0xf ? info->eax_1.extended_family : 0),
			info->eax_1.model + (info->eax_1.model == 0xf ? info->eax_1.extended_model << 4 : 0),
			info->eax_1.stepping,
			info->eax_1.features);
	} else if ((sys_info->cpu_type & B_CPU_x86_VENDOR_MASK) == B_CPU_INTEL_x86) {
		// model calculation is different for INTEL
		printf("\t%s%sype %lu, family %lu, model %u, stepping %lu, features 0x%08lx\n",
			prefix ? prefix : "", prefix && prefix[0] ? "t" : "T",
			info->eax_1.type,
			info->eax_1.family + (info->eax_1.family == 0xf ? info->eax_1.extended_family : 0),
			info->eax_1.model + ((info->eax_1.family == 0xf || info->eax_1.family == 0x6) ? info->eax_1.extended_model << 4 : 0),
			info->eax_1.stepping,
			info->eax_1.features);
	}
}

#endif	// __INTEL__


static void
dump_platform(system_info *info)
{
	printf("%s\n",
		info->platform_type == B_AT_CLONE_PLATFORM ? "IntelArchitecture" :
		info->platform_type == B_MAC_PLATFORM      ? "Macintosh" :
		info->platform_type == B_BEBOX_PLATFORM    ? "BeBox" : "unknown");
}


#ifdef __INTEL__

static void
dump_cpu(system_info *info, int32 cpu)
{
	/* references:
	 *
	 * http://grafi.ii.pw.edu.pl/gbm/x86/cpuid.html
	 * http://www.sandpile.org/ia32/cpuid.htm
	 * http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/TN13.pdf (Duron erratum)
	 */

	cpuid_info baseInfo;
	if (get_cpuid(&baseInfo, 0, cpu) != B_OK) {
		// this CPU doesn't support cpuid
		return;
	}

	int32 maxStandardFunction = baseInfo.eax_0.max_eax;
	if (maxStandardFunction >= 500)
		maxStandardFunction = 0; /* old Pentium sample chips has cpu signature here */

	/* Extended cpuid */

	cpuid_info cpuInfo;
	get_cpuid(&cpuInfo, 0x80000000, cpu);

	// extended cpuid is only supported if max_eax is greater than the service id
	int32 maxExtendedFunction = 0;
	if (cpuInfo.eax_0.max_eax > 0x80000000)
		maxExtendedFunction = cpuInfo.eax_0.max_eax & 0xff;

	if (maxExtendedFunction >=4 ) {
		char buffer[49];
		char *name = buffer;

		memset(buffer, 0, sizeof(buffer));

		for (int32 i = 0; i < 3; i++) {
			cpuid_info nameInfo;
			get_cpuid(&nameInfo, 0x80000002 + i, cpu);

			memcpy(name, &nameInfo.regs.eax, 4);
			memcpy(name + 4, &nameInfo.regs.ebx, 4);
			memcpy(name + 8, &nameInfo.regs.ecx, 4);
			memcpy(name + 12, &nameInfo.regs.edx, 4);
			name += 16;
		}

		// cut off leading spaces (names are right aligned)
		name = buffer;
		while (name[0] == ' ')
			name++;

		// the BIOS may not have set the processor name
		if (name[0])
			printf("CPU #%ld: \"%s\"\n", cpu, name);
		else {
			// Intel CPUs don't seem to have the genuine vendor field
			printf("CPU #%ld: %.12s\n", cpu,
				(info->cpu_type & B_CPU_x86_VENDOR_MASK) == B_CPU_INTEL_x86 ?
					baseInfo.eax_0.vendor_id : cpuInfo.eax_0.vendor_id);
		}
	} else {
		printf("CPU #%ld: %.12s\n", cpu, baseInfo.eax_0.vendor_id);
		if (maxStandardFunction == 0)
			return;
	}

	get_cpuid(&cpuInfo, 1, cpu);
	print_processor_signature(info, &cpuInfo, NULL);
	print_features(cpuInfo.eax_1.features);

	if (maxStandardFunction >= 1) {
		/* Extended features */
		printf("\tExtended Intel: 0x%08lx\n", cpuInfo.eax_1.extended_features);
		print_extended_features(cpuInfo.eax_1.extended_features);
	}

	/* Extended CPUID */
	if (maxExtendedFunction >= 1) {
		get_cpuid(&cpuInfo, 0x80000001, cpu);
		print_processor_signature(info, &cpuInfo, "Extended AMD: ");

		if ((info->cpu_type & B_CPU_x86_VENDOR_MASK) == B_CPU_AMD_x86
			|| (info->cpu_type & B_CPU_x86_VENDOR_MASK) == B_CPU_INTEL_x86) {
			print_amd_features(cpuInfo.regs.edx);
			if (maxExtendedFunction >= 7) {
				get_cpuid(&cpuInfo, 0x80000007, cpu);
				print_amd_power_management_features(cpuInfo.regs.edx);
			}
		} else if ((info->cpu_type & B_CPU_x86_VENDOR_MASK) == B_CPU_TRANSMETA_x86)
			print_transmeta_features(cpuInfo.regs.edx);
	}

	/* Cache/TLB descriptors */
	if (maxExtendedFunction >= 5) {
		if (!strncmp(baseInfo.eax_0.vendor_id, "CyrixInstead", 12)) {
			get_cpuid(&cpuInfo, 0x00000002, cpu);
			print_intel_cache_descriptors(info->cpu_type, &cpuInfo);
		} else if ((info->cpu_type & B_CPU_x86_VENDOR_MASK) == B_CPU_INTEL_x86) {
			// Intel does not support extended function 5 (but it does 6 hmm)
			print_intel_cache_desc(cpu);
		} else {
			print_cache_desc(cpu);
		}
	}

	if (maxStandardFunction >= 2) {
		do {
			get_cpuid(&cpuInfo, 2, cpu);

			if (cpuInfo.eax_2.call_num > 0)
				print_intel_cache_descriptors(info->cpu_type, &cpuInfo);
		} while (cpuInfo.eax_2.call_num > 1);
	}

	/* Serial number */
	if (maxStandardFunction >= 3) {
		cpuid_info flagsInfo;
		get_cpuid(&flagsInfo, 1, cpu);

		if (flagsInfo.eax_1.features & (1UL << 18)) {
			get_cpuid(&cpuInfo, 3, cpu);
			printf("Serial number: %04lx-%04lx-%04lx-%04lx-%04lx-%04lx\n",
				flagsInfo.eax_1.features >> 16, flagsInfo.eax_1.features & 0xffff,
				cpuInfo.regs.edx >> 16, cpuInfo.regs.edx & 0xffff,
				cpuInfo.regs.ecx >> 16, cpuInfo.regs.edx & 0xffff);
		}
	}

	putchar('\n');
}

#endif	// __INTEL__


static void
dump_cpus(system_info *info)
{
	const char *vendor = get_cpu_vendor_string(info->cpu_type);
	const char *model = get_cpu_model_string(info);
	char modelString[32];

	if (model == NULL && vendor == NULL)
		model = "(Unknown)";
	else if (model == NULL) {
		model = modelString;
		snprintf(modelString, 32, "(Unknown %x)", info->cpu_type);
	}

	printf("%ld %s%s%s, revision %04lx running at %LdMHz (ID: 0x%08lx 0x%08lx)\n\n",
		info->cpu_count,
		vendor ? vendor : "", vendor ? " " : "", model,
		info->cpu_revision,
		info->cpu_clock_speed / 1000000,
		info->id[0], info->id[1]);

#ifdef __INTEL__
	for (int32 cpu = 0; cpu < info->cpu_count; cpu++)
		dump_cpu(info, cpu);
#endif	// __INTEL__
}


static void
dump_mem(system_info *info)
{
	printf("%10lu bytes free      (used/max %10lu / %10lu)\n",
		B_PAGE_SIZE * (uint32)(info->max_pages - info->used_pages),
		B_PAGE_SIZE * (uint32)info->used_pages,
		B_PAGE_SIZE * (uint32)info->max_pages);
	printf("                           (cached   %10lu)\n",
		B_PAGE_SIZE * (uint32)info->cached_pages);
}


static void
dump_sem(system_info *info)
{
	printf("%10ld semaphores free (used/max %10ld / %10ld)\n",
		info->max_sems - info->used_sems,
		info->used_sems, info->max_sems);
}


static void
dump_ports(system_info *info)
{
	printf("%10ld ports free      (used/max %10ld / %10ld)\n",
		info->max_ports - info->used_ports,
		info->used_ports, info->max_ports);
}


static void
dump_thread(system_info *info)
{
	printf("%10ld threads free    (used/max %10ld / %10ld)\n",
		info->max_threads - info->used_threads,
		info->used_threads, info->max_threads);
}


static void
dump_team(system_info *info)
{
	printf("%10ld teams free      (used/max %10ld / %10ld)\n",
		info->max_teams - info->used_teams,
		info->used_teams, info->max_teams);
}


static void
dump_kinfo(system_info *info)
{
	printf("Kernel name: %s built on: %s %s version 0x%Lx\n",
		info->kernel_name,
		info->kernel_build_date, info->kernel_build_time,
		info->kernel_version );
}


static void
dump_system_info(system_info *info)
{
	dump_kinfo(info);
	dump_cpus(info);
	dump_mem(info);
	dump_sem(info);
	dump_ports(info);
	dump_thread(info);
	dump_team(info);
}


int
main(int argc, char *argv[])
{
	if (!is_computer_on()) {
		printf("The computer is not on! No info available\n");
		exit(EXIT_FAILURE);
	}

	system_info info;
	if (get_system_info(&info) != B_OK) {
		printf("Error getting system information!\n");
		return 1;
	}

	if (argc <= 1) {
		dump_system_info(&info);
	} else {
		for (int i = 1; i < argc; i++) {
			const char *opt = argv[i];
			if (strncmp(opt, "-id", strlen(opt)) == 0) {
				/* note: the original also assumes this option on "sysinfo -" */
				printf("0x%.8lx 0x%.8lx\n", info.id[0], info.id[1]);
			} else if (strncmp(opt, "-cpu", strlen(opt)) == 0) {
				dump_cpus(&info);
			} else if (strncmp(opt, "-mem", strlen(opt)) == 0) {
				dump_mem(&info);
			} else if (strncmp(opt, "-semaphores", strlen(opt)) == 0) {
				dump_sem(&info);
			} else if (strncmp(opt, "-ports", strlen(opt)) == 0) {
				dump_ports(&info);
			} else if (strncmp(opt, "-threads", strlen(opt)) == 0) {
				dump_thread(&info);
			} else if (strncmp(opt, "-teams", strlen(opt)) == 0) {
				dump_team(&info);
			} else if (strncmp(opt, "-kinfo", strlen(opt)) == 0) {
				dump_kinfo(&info);
			} else if (strncmp(opt, "-platform", strlen(opt)) == 0) {
				dump_platform(&info);
			} else if (strncmp(opt, "-disable_cpu_sn", strlen(opt)) == 0) {
				/* TODO: printf("CPU #%d serial number:  old state: %s,  new state: %s\n", ... ); */
				fprintf(stderr, "Sorry, not yet implemented\n");
			} else {
				const char *name = strrchr(argv[0], '/');
				if (name == NULL)
					name = argv[0];
				else
					name++;

				fprintf(stderr, "Usage:\n");
				fprintf(stderr, "  %s [-id|-cpu|-mem|-semaphore|-ports|-threads|-teams|-platform|-disable_cpu_sn|-kinfo]\n", name);
				return 0;
			}
		}
	}
	return 0;
}
