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

#include <stdio.h>
#include <stdlib.h>
#include <kernel/OS.h>

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
	char buff_unknown[40];
	char *cpuname = NULL;

	cpuname = info->cpu_type == B_CPU_INTEL_PENTIUM ? "Pentium" :
		info->cpu_type == B_CPU_INTEL_PENTIUM75 ? "Pentium" :
		info->cpu_type == B_CPU_INTEL_PENTIUM_486_OVERDRIVE ? "Pentium 486 Overdrive" :
		info->cpu_type == B_CPU_INTEL_PENTIUM_MMX ? "Pentium MMX" :
		info->cpu_type == B_CPU_INTEL_PENTIUM_MMX_MODEL_4 ? "Pentium MMX" :
		info->cpu_type == B_CPU_INTEL_PENTIUM_MMX_MODEL_8 ? "Pentium MMX" :
		info->cpu_type == B_CPU_INTEL_PENTIUM75_486_OVERDRIVE ? "Pentium MMX" :
		info->cpu_type == B_CPU_INTEL_PENTIUM_PRO ? "Pentium Pro" :
		info->cpu_type == B_CPU_INTEL_PENTIUM_II ? "Pentium II" :
		info->cpu_type == B_CPU_INTEL_PENTIUM_II_MODEL_3 ? "Pentium II model 3" :
		info->cpu_type == B_CPU_INTEL_PENTIUM_II_MODEL_5 ? "Pentium II model 5" :
		info->cpu_type == B_CPU_INTEL_CELERON ? "Celeron" :
		info->cpu_type == B_CPU_INTEL_PENTIUM_III ? "Pentium III" :
		info->cpu_type == B_CPU_INTEL_PENTIUM_III_MODEL_8 ? "Pentium III" :
		(info->cpu_type >= B_CPU_AMD_K5_MODEL0 && info->cpu_type <= B_CPU_AMD_K5_MODEL3) ? "K5" :
		(info->cpu_type >= B_CPU_AMD_K6_MODEL6 && info->cpu_type <= B_CPU_AMD_K6_MODEL7) ? "K6" :
		info->cpu_type == B_CPU_AMD_K6_2 ? "K6-2" :
		info->cpu_type == B_CPU_AMD_K6_III ? "K6-III" :
		info->cpu_type == B_CPU_AMD_ATHLON_MODEL1 ? "Athlon" :
		info->cpu_type == B_CPU_CYRIX_GXm ? "GXm" :
		info->cpu_type == B_CPU_CYRIX_6x86MX ? "6x86MX" :
		info->cpu_type == B_CPU_IDT_WINCHIP_C6 ? "WinChip C6" :
		info->cpu_type == B_CPU_IDT_WINCHIP_2 ? "WinChip 2" :
		info->cpu_type == B_CPU_RISE_mP6 ? "mP6" :
#ifdef B_CPU_AMD_ATHLON_THUNDERBIRD /* R5 doesn't have those defines */
		info->cpu_type == B_CPU_AMD_ATHLON_THUNDERBIRD ? "Athlon TBird" :
		info->cpu_type == B_CPU_NATIONAL_GEODE_GX1 ? "Geode GX1" :
#endif
		NULL;
	
	if (cpuname == NULL && info->cpu_type > B_CPU_INTEL_X86 && info->cpu_type < B_CPU_AMD_X86)
		sprintf((cpuname = buff_unknown), "(unknown Intel x86: 0x%08ld)", info->cpu_type);
	if (cpuname == NULL && info->cpu_type > B_CPU_AMD_X86 && info->cpu_type < B_CPU_CYRIX_X86)
		sprintf((cpuname = buff_unknown), "(unknown AMD x86: 0x%08ld)", info->cpu_type);
	if (cpuname == NULL && info->cpu_type & B_CPU_X86_VENDOR_MASK)
		sprintf((cpuname = buff_unknown), "(unknown x86: 0x%08ld)", info->cpu_type);
	if (cpuname == NULL)
		sprintf((cpuname = buff_unknown), "(unknown: 0x%08ld)", info->cpu_type);
	printf("%ld %s revision %04lx running at %LdMHz (ID: 0x%08lx 0x%08lx)\n",
		info->cpu_count,
		cpuname,
		info->cpu_revision,
		info->cpu_clock_speed / 1000000,
		info->id[0], info->id[1]);

	for (i = 0; i < info->cpu_count; i++) {
		/* note: the R5 cpuid_info() doesn't failed when asked for a cpu > cpu_count */
		/* indeed it should return EINVAL */
		/* references:
		 * http://grafi.ii.pw.edu.pl/gbm/x86/cpuid.html
		 */
		cpuid_info cpuii[5];
		cpuid_info cpuiie[5]; /* Extended CPUID */
		int max_eax, max_eeax = 0;
		status_t ret;
		int j;


		ret = get_cpuid(&cpuii[0], 0, i);
		if (ret != B_OK) {
			fprintf(stderr, "cpuid_info(, %ld, %ld): error 0x%08lx\n", 0, i, ret);
			break;
		}
		max_eax = cpuii[0].eax_0.max_eax;
		if (max_eax >= 500)
			max_eax = 0; /* old Pentium sample chips has cpu signature here */
		for (j = 1; j <= max_eax && j < 5; j++) {
			ret = get_cpuid(&cpuii[j], j, i);
			if (ret != B_OK) {
				fprintf(stderr, "cpuid_info(, %ld, %ld): error 0x%08lx\n", j, i, ret);
				break;
			}
		}
		/* Extended CPUID: add more checks */
		if ((!strncmp(cpuii[0].eax_0.vendorid, "AuthenticAMD", 12) 
					&& cpuii[1].eax_1.family >= 5, cpuii[1].eax_1.model >= 1)) {
			ret = get_cpuid(&cpuiie[0], 0x80000000, i);
			if (ret != B_OK) {
				fprintf(stderr, "cpuid_info(, %ld, %ld): error 0x%08lx\n", 0, i, ret);
				break;
			}
			max_eeax = cpuiie[0].eax_0.max_eax & 0x0ff;
		}
		for (j = 1; j <= max_eeax && j < 5; j++) {
			ret = get_cpuid(&cpuiie[j], 0x80000000+j, i);
			if (ret != B_OK) {
				fprintf(stderr, "cpuid_info(, %ld, %ld): error 0x%08lx\n", 0x80000000+j, i, ret);
				break;
			}
		}

//printf("max_eax=%ld, max_eeax=%ld\n", max_eax, max_eeax);

		printf("CPU #%d: %.12s\n", i, cpuii[0].eax_0.vendorid);
		if (max_eax == 0)
			break;
		printf("\ttype %u, family %u, model %u, stepping %u, features 0x%08x\n", 
				cpuii[1].eax_1.type, 
				cpuii[1].eax_1.family, 
				cpuii[1].eax_1.model, 
				cpuii[1].eax_1.stepping, 
				cpuii[1].eax_1.features);
		/* Extended CPUID */
		if (max_eeax > 0) {
			printf("\tExtended information:\n");
			printf("\ttype %u, family %u, model %u, stepping %u, features 0x%08x\n",
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
		if (max_eax == 1)
			break;
		if (max_eax == 2)
			break;
//Serial number: %04X-%04X-%04X-%04X-%04X-%04X
		if (max_eax == 3)
			break;


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

