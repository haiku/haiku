/*
 * sysinfo.c
 * (c) 2002, Carlos Hasan, for OpenBeOS.
 */

/*
 * NOTES:
 * UNFINISHED (-cpu and -disable_cpu_sn options)
 * Francois Revol, 12/08/2002: changed option check, to be like the original
 * (one char options, except -te, and "-" is like -id)
 */

#define OBOS_CPU_TYPES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/OS.h>

static const int cache_desc_values[] = {
0x01, 0x02, 0x03, 0x04, 0x06, 0x08, 0x0A, 0x0C, 
0x10, 0x15, 0x1A, 0x22, 0x23, 0x25, 0x29, 0x39, 
0x3B, 0x3C, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 
0x50, 0x51, 0x52, 0x5B, 0x5C, 0x5D, 0x66, 0x67, 
0x68, 0x70, 0x71, 0x72, 0x77, 0x79, 0x7A, 0x7B, 
0x7C, 0x7E, 0x81, 0x82, 0x83, 0x84, 0x85, 0x88, 
0x89, 0x8A, 0x8D, 0x90, 0x96, 0x9B, 0x70, 0x74, 
0x77, 0x80, 0x82, 0x84, 0
};

static const char *cache_desc_strings[] = {
"Instruction TLB: 4k-byte pages, 4-way set associative, 32 entries",
"Instruction TLB: 4M-byte pages, fully associative, 2 entries",
"Data TLB: 4k-byte pages, 4-way set associative, 64 entries",
"Data TLB: 4M-byte pages, 4-way set associative, 8 entries",
"L1I cache: 8 kbytes, 4-way set associative, 32 bytes/line",
"L1I cache: 16 kbytes, 4-way set associative, 32 bytes/line",
"L1D cache: 8 kbytes, 2-way set associative, 32 bytes/line",
"L1D cache: 16 kbytes, 4-way set associative, 32 bytes/line",
"L1D cache: 16 kbytes, 4-way set associative, 32 bytes/line (IA-64)",
"L1I cache: 16 kbytes, 4-way set associative, 32 bytes/line (IA-64)",
"L2 cache: 96 kbytes, 6-way set associative, 64 bytes/line (IA-64)",
"L3 cache: 512 kbytes, 4-way set associative (!), 64 bytes/line, dual-sectored",
"L3 cache: 1024 kbytes, 8-way set associative, 64 bytes/line, dual-sectored",
"L3 cache: 2048 kbytes, 8-way set associative, 64 bytes/line, dual-sectored",
"L3 cache: 4096 kbytes, 8-way set associative, 64 bytes/line, dual-sectored",
"L2 cache: 128 kbytes, 4-way set associative, 64 bytes/line, sectored",
"L2 cache: 128 kbytes, 2-way set associative, 64 bytes/line, sectored",
"L2 cache: 256 kbytes, 4-way set associative, 64 bytes/line, sectored",
"No integrated L2 cache (P6 core) or L3 cache (P4 core)",
"L2 cache: 128 kbytes, 4-way set associative, 32 bytes/line",
"L2 cache: 256 kbytes, 4-way set associative, 32 bytes/line",
"L2 cache: 512 kbytes, 4-way set associative, 32 bytes/line",
"L2 cache: 1024 kbytes, 4-way set associative, 32 bytes/line",
"L2 cache: 2048 kbytes, 4-way set associative, 32 bytes/line",
"Instruction TLB: 4K/4M/2M-bytes pages, fully associative, 64 entries",
"Instruction TLB: 4K/4M/2M-bytes pages, fully associative, 128 entries",
"Instruction TLB: 4K/4M/2M-bytes pages, fully associative, 256 entries",
"Data TLB: 4K/4M-bytes pages, fully associative, 64 entries",
"Data TLB: 4K/4M-bytes pages, fully associative, 128 entries",
"Data TLB: 4K/4M-bytes pages, fully associative, 256 entries",
"L1D cache: 8 kbytes, 4-way set associative, 64 bytes/line, sectored",
"L1D cache: 16 kbytes, 4-way set associative, 64 bytes/line, sectored",
"L1D cache: 32 kbytes, 4-way set associative, 64 bytes/line, sectored",
"trace L1 cache: 12 KµOPs, 8-way set associative",
"trace L1 cache: 16 KµOPs, 8-way set associative",
"trace L1 cache: 32 KµOPs, 8-way set associative",
"L1I cache: 16 kbytes, 4-way set associative, 64 bytes/line, sectored (IA-64)",
"L2 cache: 128 kbytes, 8-way set associative, 64 bytes/line, dual-sectored",
"L2 cache: 256 kbytes, 8-way set associative, 64 bytes/line, dual-sectored",
"L2 cache: 512 kbytes, 8-way set associative, 64 bytes/line, dual-sectored",
"L2 cache: 1024 kbytes, 8-way set associative, 64 bytes/line, dual-sectored",
"L2 cache: 256 kbytes, 8-way set associative, 128 bytes/line, sect. (IA-64)",
"L2 cache: 128 kbytes, 8-way set associative, 32 bytes/line",
"L2 cache: 256 kbytes, 8-way set associative, 32 bytes/line",
"L2 cache: 512 kbytes, 8-way set associative, 32 bytes/line",
"L2 cache: 1024 kbytes, 8-way set associative, 32 bytes/line",
"L2 cache: 2048 kbytes, 8-way set associative, 32 bytes/line",
"L3 cache: 2048 kbytes, 4-way set associative, 64 bytes/line (IA-64)",
"L3 cache: 4096 kbytes, 4-way set associative, 64 bytes/line (IA-64)",
"L3 cache: 8192 kbytes, 4-way set associative, 64 bytes/line (IA-64)",
"L3 cache: 3096 kbytes, 12-way set associative, 128 bytes/line (IA-64)",
"Instruction TLB: 4K...256M-bytes pages, fully associative, 64 entries (IA-64)",
"L1D TLB: 4K...256M-bytes pages, fully associative, 32 entries (IA-64)",
"L2D TLB: 4K...256M-bytes pages, fully associative, 96 entries (IA-64)",
"Cyrix specific: Code and data TLB: 4k-bytes pages, 4-way set associative, 32 entries",
"Cyrix specific: ???",
"Cyrix specific: ???",
"Cyrix specific: L1 cache: 16 kbytes, 4-way set associative, 16 bytes/line",
"Cyrix specific: ???",
"Cyrix specific: ???",
NULL
};

static void print_cache_descriptors(cpuid_info *cpuii)
{
	int i, j;
	for (i = 0; i < 15; i++) {
		if (cpuii->eax_2.cache_descriptors[i] == 0)
			continue;
		for (j = 0; cache_desc_values[j]; j++) {
			if (cpuii->eax_2.cache_descriptors[i] == cache_desc_values[j]) {
				printf("\t%s\n", cache_desc_strings[j]);
				break;
			}
		}
		if (cache_desc_values[j] == 0)
			printf("\tUnknown cache descriptor 0x%02x\n", cpuii->eax_2.cache_descriptors[i]);
	}
}

static void print_amd_TLB(uint32 reg)
{
	int num;
	int entries[2];
	int ways[2];
	char *name[2] = { "instruction TLB:", "data TLB       :" };

	entries[0] = (reg & 0xff);
	ways[0] = ((reg >> 8) & 0xff);
	entries[1] = ((reg >> 16) & 0xff);
	ways[1] = ((reg >> 24) & 0xff);
	for (num=0; num < 2; num++) {
		printf("\t%s %u entries, ", name[num], entries[num]);
		if (ways[num] == 0xff)
			printf("fully associative\n");
		else
			printf("%u-way set associative\n", ways[num]);
	}
}

static void print_amd_cache(uint32 reg, int code)
{
	int size;
	int ways;
	int lines_per_tag;
	int linesize;
	char *name[2] = { "L1I cache:", "L1D cache:" };

	size = ((reg >> 24) & 0xff);
	ways = ((reg >> 16) & 0xff);
	lines_per_tag = ((reg >> 8) & 0xff);
	linesize = (reg & 0xff);
	printf("\t%s %u kbytes,", name[code?1:0], size);
	if (ways == 0xff)
		printf("fully associative, ");
	else
		printf("%u-way set associative, ", ways);
	printf("%u lines/tag, %u bytes/line\n", lines_per_tag, linesize);
}

static void print_cache_descriptors_newway(cpuid_info *cpuii)
{
	if (cpuii->regs.eax)
		print_amd_TLB(cpuii->regs.eax);
	if (cpuii->regs.ebx)
		print_amd_TLB(cpuii->regs.ebx);
	print_amd_cache(cpuii->regs.ecx, 0);
	print_amd_cache(cpuii->regs.edx, 1);
}

static char *get_cpu_type_string(uint32 cpu_type, char *buffer)
{
	char *cpuname = NULL;

	cpuname = cpu_type == B_CPU_INTEL_PENTIUM ? "Pentium" :
		cpu_type == B_CPU_INTEL_PENTIUM75 ? "Pentium" :
		cpu_type == B_CPU_INTEL_PENTIUM_486_OVERDRIVE ? "Pentium 486 Overdrive" :
		cpu_type == B_CPU_INTEL_PENTIUM_MMX ? "Pentium MMX" :
		cpu_type == B_CPU_INTEL_PENTIUM_MMX_MODEL_4 ? "Pentium MMX" :
		cpu_type == B_CPU_INTEL_PENTIUM_MMX_MODEL_8 ? "Pentium MMX" :
		cpu_type == B_CPU_INTEL_PENTIUM75_486_OVERDRIVE ? "Pentium MMX" :
		cpu_type == B_CPU_INTEL_PENTIUM_PRO ? "Pentium Pro" :
		cpu_type == B_CPU_INTEL_PENTIUM_II ? "Pentium II" :
		cpu_type == B_CPU_INTEL_PENTIUM_II_MODEL_3 ? "Pentium II model 3" :
		cpu_type == B_CPU_INTEL_PENTIUM_II_MODEL_5 ? "Pentium II model 5" :
		cpu_type == B_CPU_INTEL_CELERON ? "Celeron" :
		cpu_type == B_CPU_INTEL_PENTIUM_III ? "Pentium III" :
		cpu_type == B_CPU_INTEL_PENTIUM_III_MODEL_8 ? "Pentium III" :
		cpu_type == B_CPU_INTEL_PENTIUM_IV ? "Pentium IV" :
		cpu_type == B_CPU_INTEL_PENTIUM_IV_MODEL2 ? "Pentium IV" :
		(cpu_type >= B_CPU_AMD_K5_MODEL0 && cpu_type <= B_CPU_AMD_K5_MODEL3) ? "K5" :
		(cpu_type >= B_CPU_AMD_K6_MODEL6 && cpu_type <= B_CPU_AMD_K6_MODEL7) ? "K6" :
		cpu_type == B_CPU_AMD_K6_2 ? "K6-2" :
		cpu_type == B_CPU_AMD_K6_III ? "K6-III" :
		cpu_type == B_CPU_AMD_K6_III_MODEL2 ? "K6-III" :
		cpu_type == B_CPU_AMD_ATHLON_MODEL1 ? "Athlon" :
		cpu_type == B_CPU_AMD_ATHLON_MODEL2 ? "Athlon" :
		cpu_type == B_CPU_AMD_DURON ? "Duron" :
		cpu_type == B_CPU_AMD_ATHLON_THUNDERBIRD ? "Athlon TBird" :
		cpu_type == B_CPU_AMD_ATHLON_XP ? "Athlon XP" :
		cpu_type == B_CPU_AMD_ATHLON_XP_MODEL2 ? "Athlon XP" :
		cpu_type == B_CPU_AMD_ATHLON_XP_MODEL3 ? "Athlon XP" :
		cpu_type == B_CPU_CYRIX_GXm ? "GXm" :
		cpu_type == B_CPU_CYRIX_6x86MX ? "6x86MX" :
		cpu_type == B_CPU_IDT_WINCHIP_C6 ? "WinChip C6" :
		cpu_type == B_CPU_IDT_WINCHIP_2 ? "WinChip 2" :
		cpu_type == B_CPU_RISE_mP6 ? "mP6" :
		cpu_type == B_CPU_NATIONAL_GEODE_GX1 ? "Geode GX1" :
		NULL;
	if (cpuname)
		return cpuname;
	if (cpuname == NULL && cpu_type > B_CPU_INTEL_X86 && cpu_type < B_CPU_AMD_X86)
		sprintf((cpuname = buffer), "(unknown Intel x86: 0x%08ld)", cpu_type);
	if (cpuname == NULL && cpu_type > B_CPU_AMD_X86 && cpu_type < B_CPU_CYRIX_X86)
		sprintf((cpuname = buffer), "(unknown AMD x86: 0x%08ld)", cpu_type);
	if (cpuname == NULL && cpu_type & B_CPU_X86_VENDOR_MASK)
		sprintf((cpuname = buffer), "(unknown x86: 0x%08ld)", cpu_type);
	if (cpuname == NULL)
		sprintf((cpuname = buffer), "(unknown: 0x%08ld)", cpu_type);
	return cpuname;
}

static void dump_platform(system_info *info)
{
	printf("%s\n",
		info->platform_type == B_AT_CLONE_PLATFORM ? "IntelArchitecture" :
		info->platform_type == B_MAC_PLATFORM      ? "Macintosh" :
		info->platform_type == B_BEBOX_PLATFORM    ? "BeBox" : "unknown");
}

static void dump_cpu(system_info *info)
{
	int i;
	char buff[40];

	printf("%ld %s revision %04lx running at %LdMHz (ID: 0x%08lx 0x%08lx)\n",
		info->cpu_count,
		get_cpu_type_string(info->cpu_type,  buff),
		info->cpu_revision,
		info->cpu_clock_speed / 1000000,
		info->id[0], info->id[1]);

	for (i = 0; i < info->cpu_count; i++) {
		/* note: the R5 cpuid_info() doesn't fail when asked for a cpu > cpu_count */
		/* indeed it should return EINVAL */
		/* references:
		 * http://grafi.ii.pw.edu.pl/gbm/x86/cpuid.html
		 * http://www.sandpile.org/ia32/cpuid.htm
		 * 
		 * http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/TN13.pdf (Duron erratum)
		 */
		cpuid_info cpuii[5];
		cpuid_info cpuiie[9]; /* Extended CPUID */
		int max_eax, max_eeax = 0;
		status_t ret;
		int j;
		int check_extended = 0;


		ret = get_cpuid(&cpuii[0], 0, i);
		if (ret != B_OK) {
			fprintf(stderr, "cpuid_info(, %d, %d): error 0x%08lx\n", 0, i, ret);
			break;
		}
		max_eax = cpuii[0].eax_0.max_eax;
		if (max_eax >= 500)
			max_eax = 0; /* old Pentium sample chips has cpu signature here */
		for (j = 1; j <= max_eax && j < 5; j++) {
			ret = get_cpuid(&cpuii[j], j, i);
			if (ret != B_OK) {
				fprintf(stderr, "cpuid_info(, %d, %d): error 0x%08lx\n", j, i, ret);
				break;
			}
		}

		/* Extended CPUID - XXX: add more checks -- are they really needed ? */

		if ((!strncmp(cpuii[0].eax_0.vendorid, "AuthenticAMD", 12) 
					&& cpuii[1].eax_1.family >= 5 && cpuii[1].eax_1.model >= 1))
			check_extended = 1;
		// check_extended = 1; // until we get into problems :)
		// actually I ran into problems on my dual :^)

		if (check_extended) {
			ret = get_cpuid(&cpuiie[0], 0x80000000, i);
			if (ret != B_OK) {
				fprintf(stderr, "cpuid_info(, %d, %d): error 0x%08lx\n", 0x80000000, i, ret);
				break;
			}
			max_eeax = cpuiie[0].eax_0.max_eax & 0x0ff;
		}
		for (j = 1; j <= max_eeax && j < 9; j++) {
			ret = get_cpuid(&cpuiie[j], 0x80000000+j, i);
			if (ret != B_OK) {
				fprintf(stderr, "cpuid_info(, %d, %d): error 0x%08lx\n", 0x80000000+j, i, ret);
				break;
			}
		}

		//printf("max_eax=%ld, max_eeax=%ld\n", max_eax, max_eeax);

		printf("CPU #%d: %.12s\n", i, cpuii[0].eax_0.vendorid);
		if (max_eax == 0)
			continue;
		printf("\ttype %u, family %u, model %u, stepping %u, features 0x%08lx\n", 
				cpuii[1].eax_1.type, 
				cpuii[1].eax_1.family, 
				cpuii[1].eax_1.model, 
				cpuii[1].eax_1.stepping, 
				cpuii[1].eax_1.features);
		/* Extended CPUID */
		if (max_eeax > 0) {
			printf("\tExtended information:\n");
			printf("\ttype %u, family %u, model %u, stepping %u, features 0x%08lx\n",
				cpuiie[1].eax_1.type, 
				cpuiie[1].eax_1.family, 
				cpuiie[1].eax_1.model, 
				cpuiie[1].eax_1.stepping, 
				cpuiie[1].eax_1.features);
		}
		if (max_eeax > 1) {
			char cpuname[50];
			memset(cpuname, 0, 50);
			memcpy(cpuname, &cpuiie[2].regs.eax, 4);
			memcpy(cpuname+4, &cpuiie[2].regs.ebx, 4);
			memcpy(cpuname+8, &cpuiie[2].regs.ecx, 4);
			memcpy(cpuname+12, &cpuiie[2].regs.edx, 4);
			memcpy(cpuname+16, &cpuiie[3].regs.eax, 4);
			memcpy(cpuname+20, &cpuiie[3].regs.ebx, 4);
			memcpy(cpuname+24, &cpuiie[3].regs.ecx, 4);
			memcpy(cpuname+28, &cpuiie[3].regs.edx, 4);
			memcpy(cpuname+32, &cpuiie[4].regs.eax, 4);
			memcpy(cpuname+36, &cpuiie[4].regs.ebx, 4);
			memcpy(cpuname+40, &cpuiie[4].regs.ecx, 4);
			memcpy(cpuname+44, &cpuiie[4].regs.edx, 4);
			
			printf("\tName: %s\n", cpuname);
		}


		/* Cache/TLB descriptors */
		if (max_eeax >= 5) {
			if (!strncmp(cpuii[0].eax_0.vendorid, "CyrixInstead", 12))
				print_cache_descriptors(&cpuiie[5]);
			else
				print_cache_descriptors_newway(&cpuiie[5]);
		}

		if (max_eax == 1)
			continue;
		while (cpuii[2].eax_2.call_num > 0) {
			print_cache_descriptors(&cpuii[2]);
			if (cpuii[2].eax_2.call_num == 1)
				break;
			ret = get_cpuid(&cpuii[2], 2, i);
			if (ret != B_OK) {
				fprintf(stderr, "cpuid_info(, %d, %d): error 0x%08lx\n", 2, i, ret);
				break;
			}
			
		}
		if (max_eax == 2)
			continue;
//Serial number: %04X-%04X-%04X-%04X-%04X-%04X
		if (max_eax == 3)
			continue;


		/* TODO: printf("CPU #%d: %s\n", i, ); */
	}
	printf("\n");

}

static void dump_mem(system_info *info)
{
	printf("%8ld bytes free      (used/max %8ld / %8ld)\n",
		B_PAGE_SIZE * (info->max_pages - info->used_pages),
		B_PAGE_SIZE * info->used_pages,
		B_PAGE_SIZE * info->max_pages);
}

static void dump_sem(system_info *info)
{
	printf("%8ld semaphores free (used/max %8ld / %8ld)\n",
		info->max_sems - info->used_sems,
		info->used_sems, info->max_sems);
}

static void dump_ports(system_info *info)
{
	printf("%8ld ports free      (used/max %8ld / %8ld)\n",
		info->max_ports - info->used_ports,
		info->used_ports, info->max_ports);
}

static void dump_thread(system_info *info)
{
	printf("%8ld threads free    (used/max %8ld / %8ld)\n",
		info->max_threads - info->used_threads,
		info->used_threads, info->max_threads);
}

static void dump_team(system_info *info)
{
	printf("%8ld teams free      (used/max %8ld / %8ld)\n",
		info->max_teams - info->used_teams,
		info->used_teams, info->max_teams);
}

static void dump_kinfo(system_info *info)
{
	printf("Kernel name: %s built on: %s %s version 0x%Lx\n",
		info->kernel_name,
		info->kernel_build_date, info->kernel_build_time,
		info->kernel_version );
}

static void dump_system_info(system_info *info)
{
	dump_kinfo(info);
	dump_cpu(info);
	dump_mem(info);
	dump_sem(info);
	dump_thread(info);
	dump_team(info);
}


int main(int argc, char *argv[])
{
	system_info info;
	const char *opt;
	int i;

	if (!is_computer_on()) {
		printf("The computer is not on! No info available\n");
		exit(EXIT_FAILURE);
	}

	if (get_system_info(&info) != B_OK) {
		printf("Error getting system information!\n");
		return 1;
	}

	if (argc <= 1) {
		dump_system_info(&info);
	}
	else {
		for (i = 1; i < argc; i++) {
			opt = argv[i];
			if (strncmp(opt, "-id", strlen(opt)) == 0) {
				/* note: the original also assumes this option on "sysinfo -" */
				printf("0x%.8lx 0x%.8lx\n", info.id[0], info.id[1]);
			}
			else if (strncmp(opt, "-cpu", strlen(opt)) == 0) {
				dump_cpu(&info);
			}
			else if (strncmp(opt, "-mem", strlen(opt)) == 0) {
				dump_mem(&info);
			}
			else if (strncmp(opt, "-semaphores", strlen(opt)) == 0) {
				dump_sem(&info);
			}
			else if (strncmp(opt, "-ports", strlen(opt)) == 0) {
				dump_ports(&info);
			}
			else if (strncmp(opt, "-threads", strlen(opt)) == 0) {
				dump_thread(&info);
			}
			else if (strncmp(opt, "-teams", strlen(opt)) == 0) {
				dump_team(&info);
			}
			else if (strncmp(opt, "-kinfo", strlen(opt)) == 0) {
				dump_kinfo(&info);
			}
			else if (strncmp(opt, "-platform", strlen(opt)) == 0) {
				dump_platform(&info);
			}
			else if (strncmp(opt, "-disable_cpu_sn", strlen(opt)) == 0) {

				/* TODO: printf("CPU #%d serial number:  old state: %s,  new state: %s\n", ... ); */

			}
			else {
				fprintf(stderr, "Usage:\n");
				fprintf(stderr, "%s [-id|-cpu|-mem|-semaphore|-ports|-threads|-teams|-platform|-disable_cpu_sn|-kinfo]\n", argv[0]);
				return 0;
			}

		}

	}
	return 0;
}

