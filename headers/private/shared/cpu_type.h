/*
 * Copyright 2004-2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

/* Taken from the Pulse application, and extended.
 * It's used by Pulse, AboutHaiku, and sysinfo.
 */

#include <stdlib.h>
#include <stdio.h>

#include <OS.h>


#ifdef __cplusplus
extern "C" {
#endif

const char *get_cpu_vendor_string(enum cpu_types type);
const char *get_cpu_model_string(system_info *info);
void get_cpu_type(char *vendorBuffer, size_t vendorSize,
		char *modelBuffer, size_t modelSize);
int32 get_rounded_cpu_speed(void);

#ifdef __cplusplus
}
#endif


#if __INTEL__
/*!	Tries to parse an Intel CPU ID string to match our usual naming scheme.
	Note, this function is not thread safe, and must only be called once
	at a time.
*/
static const char*
parse_intel(const char* name)
{
	static char buffer[49];

	// ignore initial spaces
	int index = 0;
	for (; name[index] != '\0'; index++) {
		if (name[index] != ' ')
			break;
	}

	// ignore vendor
	for (; name[index] != '\0'; index++) {
		if (name[index] == ' ') {
			index++;
			break;
		}
	}

	// parse model
	int outIndex = 0;
	for (; name[index] != '\0'; index++) {
		if (!strncmp(&name[index], "(R)", 3)) {
			outIndex += strlcpy(&buffer[outIndex], "®",
				sizeof(buffer) - outIndex);
			index += 2;
		} else if (!strncmp(&name[index], "(TM)", 4)) {
			outIndex += strlcpy(&buffer[outIndex], "™",
				sizeof(buffer) - outIndex);
			index += 3;
		} else if (!strncmp(&name[index], " CPU", 4)) {
			// Cut out the CPU string
			index += 3;
		} else if (!strncmp(&name[index], " @", 2)) {
			// Cut off the remainder
			break;
		} else
			buffer[outIndex++] = name[index];
	}

	buffer[outIndex] = '\0';
	return buffer;
}
#endif


const char *
get_cpu_vendor_string(enum cpu_types type)
{
#if __POWERPC__
	/* We're not that nice here. */
	return "IBM/Motorola";
#endif
#if __INTEL__
	/* Determine x86 vendor name */
	switch (type & B_CPU_x86_VENDOR_MASK) {
		case B_CPU_INTEL_x86:
			return "Intel";
		case B_CPU_AMD_x86:
			return "AMD";
		case B_CPU_CYRIX_x86:
			return "Cyrix";
		case B_CPU_IDT_x86:
			/* IDT was bought by VIA */
			if (((type >> 8) & 0xf) >= 6)
				return "VIA";
			return "IDT";
		case B_CPU_RISE_x86:
				return "Rise";
		case B_CPU_TRANSMETA_x86:
			return "Transmeta";

		default:
			return NULL;
	}
#endif
}


#ifdef __INTEL__
/*! Parameter 'name' needs to point to an allocated array of 49 characters. */
void
get_cpuid_model_string(char *name)
{
	/* References:
	 *
	 * http://grafi.ii.pw.edu.pl/gbm/x86/cpuid.html
	 * http://www.sandpile.org/ia32/cpuid.htm
	 * http://www.amd.com/us-en/assets/content_type/
	 *	white_papers_and_tech_docs/TN13.pdf (Duron erratum)
	 */

	cpuid_info baseInfo;
	cpuid_info cpuInfo;
	int32 maxStandardFunction, maxExtendedFunction = 0;

	memset(name, 0, 49 * sizeof(char));

	if (get_cpuid(&baseInfo, 0, 0) != B_OK) {
		/* This CPU doesn't support cpuid. */
		return;
	}

	maxStandardFunction = baseInfo.eax_0.max_eax;
	if (maxStandardFunction >= 500) {
		maxStandardFunction = 0;
			/* Old Pentium sample chips have the CPU signature here. */
	}

	/* Extended cpuid */

	get_cpuid(&cpuInfo, 0x80000000, 0);
		/* hardcoded to CPU 0 */

	/* Extended cpuid is only supported if max_eax is greater than the */
	/* service id. */
	if (cpuInfo.eax_0.max_eax > 0x80000000)
		maxExtendedFunction = cpuInfo.eax_0.max_eax & 0xff;

	if (maxExtendedFunction >= 4) {
		int32 i;

		for (i = 0; i < 3; i++) {
			cpuid_info nameInfo;
			get_cpuid(&nameInfo, 0x80000002 + i, 0);

			memcpy(name, &nameInfo.regs.eax, 4);
			memcpy(name + 4, &nameInfo.regs.ebx, 4);
			memcpy(name + 8, &nameInfo.regs.ecx, 4);
			memcpy(name + 12, &nameInfo.regs.edx, 4);
			name += 16;
		}
	}
}
#endif	/* __INTEL__ */


const char *
get_cpu_model_string(system_info *info)
{
#if __INTEL__
	char cpuidName[49];
		/* for use with get_cpuid_model_string() */
#endif	/* __INTEL__ */

	/* Determine CPU type */
	switch (info->cpu_type) {
#if __POWERPC__
		case B_CPU_PPC_603:
			return "603";
		case B_CPU_PPC_603e:
			return "603e";
		case B_CPU_PPC_750:
			return "750";
		case B_CPU_PPC_604:
			return "604";
		case B_CPU_PPC_604e:
			return "604e";
#endif	/* __POWERPC__ */
#if __INTEL__
		case B_CPU_x86:
			return "Unknown x86";

		/* Intel */
		case B_CPU_INTEL_PENTIUM:
		case B_CPU_INTEL_PENTIUM75:
			return "Pentium";
		case B_CPU_INTEL_PENTIUM_486_OVERDRIVE:
		case B_CPU_INTEL_PENTIUM75_486_OVERDRIVE:
			return "Pentium OD";
		case B_CPU_INTEL_PENTIUM_MMX:
		case B_CPU_INTEL_PENTIUM_MMX_MODEL_8:
			return "Pentium MMX";
		case B_CPU_INTEL_PENTIUM_PRO:
			return "Pentium Pro";
		case B_CPU_INTEL_PENTIUM_II_MODEL_3:
		case B_CPU_INTEL_PENTIUM_II_MODEL_5:
			return "Pentium II";
		case B_CPU_INTEL_CELERON:
		case B_CPU_INTEL_CELERON_MODEL_22:
			return "Celeron";
		case B_CPU_INTEL_PENTIUM_III:
		case B_CPU_INTEL_PENTIUM_III_MODEL_8:
		case B_CPU_INTEL_PENTIUM_III_MODEL_11:
		case B_CPU_INTEL_PENTIUM_III_XEON:
			return "Pentium III";
		case B_CPU_INTEL_PENTIUM_M:
		case B_CPU_INTEL_PENTIUM_M_MODEL_13:
			get_cpuid_model_string(cpuidName);
			if (strcasestr(cpuidName, "Celeron") != NULL)
				return "Pentium M Celeron";
			return "Pentium M";
		case B_CPU_INTEL_ATOM:
			return "Atom";
		case B_CPU_INTEL_PENTIUM_CORE:
			get_cpuid_model_string(cpuidName);
			if (strcasestr(cpuidName, "Celeron") != NULL)
				return "Core Celeron";
			return "Core";
		case B_CPU_INTEL_PENTIUM_CORE_2:
			get_cpuid_model_string(cpuidName);
			if (strcasestr(cpuidName, "Celeron") != NULL)
				return "Core 2 Celeron";
			if (strcasestr(cpuidName, "Xeon") != NULL)
				return "Core 2 Xeon";
			return "Core 2";
		case B_CPU_INTEL_PENTIUM_CORE_2_45_NM:
			get_cpuid_model_string(cpuidName);
			if (strcasestr(cpuidName, "Celeron") != NULL)
				return "Core 2 Celeron";
			if (strcasestr(cpuidName, "Xeon") != NULL)
				return "Core 2 Xeon";
			if (strcasestr(cpuidName, "Pentium") != NULL)
				return "Pentium";
			if (strcasestr(cpuidName, "Extreme") != NULL)
				return "Core 2 Extreme";
			return	"Core 2";
		case B_CPU_INTEL_PENTIUM_CORE_I5_M430:
			return "Core i5";
		case B_CPU_INTEL_PENTIUM_CORE_I7:
		case B_CPU_INTEL_PENTIUM_CORE_I7_Q720:
			get_cpuid_model_string(cpuidName);
			if (strcasestr(cpuidName, "Xeon") != NULL)
				return "Core i7 Xeon";
			return "Core i7";
		case B_CPU_INTEL_PENTIUM_IV:
		case B_CPU_INTEL_PENTIUM_IV_MODEL_1:
		case B_CPU_INTEL_PENTIUM_IV_MODEL_2:
		case B_CPU_INTEL_PENTIUM_IV_MODEL_3:
		case B_CPU_INTEL_PENTIUM_IV_MODEL_4:
			get_cpuid_model_string(cpuidName);
			if (strcasestr(cpuidName, "Celeron") != NULL)
				return "Pentium 4 Celeron";
			if (strcasestr(cpuidName, "Xeon") != NULL)
				return "Pentium 4 Xeon";
			return "Pentium 4";

		/* AMD */
		case B_CPU_AMD_K5_MODEL_0:
		case B_CPU_AMD_K5_MODEL_1:
		case B_CPU_AMD_K5_MODEL_2:
		case B_CPU_AMD_K5_MODEL_3:
			return "K5";
		case B_CPU_AMD_K6_MODEL_6:
		case B_CPU_AMD_K6_MODEL_7:
			return "K6";
		case B_CPU_AMD_K6_2:
			return "K6-2";
		case B_CPU_AMD_K6_III:
		case B_CPU_AMD_K6_III_MODEL_13:
			return "K6-III";
		case B_CPU_AMD_GEODE_LX:
			return "Geode LX";
		case B_CPU_AMD_ATHLON_MODEL_1:
		case B_CPU_AMD_ATHLON_MODEL_2:
		case B_CPU_AMD_ATHLON_THUNDERBIRD:
			return "Athlon";
		case B_CPU_AMD_ATHLON_XP_MODEL_6:
		case B_CPU_AMD_ATHLON_XP_MODEL_7:
		case B_CPU_AMD_ATHLON_XP_MODEL_8:
		case B_CPU_AMD_ATHLON_XP_MODEL_10:
			return "Athlon XP";
		case B_CPU_AMD_DURON:
			return "Duron";
		case B_CPU_AMD_ATHLON_64_MODEL_3:
		case B_CPU_AMD_ATHLON_64_MODEL_4:
		case B_CPU_AMD_ATHLON_64_MODEL_7:
		case B_CPU_AMD_ATHLON_64_MODEL_8:
		case B_CPU_AMD_ATHLON_64_MODEL_11:
		case B_CPU_AMD_ATHLON_64_MODEL_12:
		case B_CPU_AMD_ATHLON_64_MODEL_14:
		case B_CPU_AMD_ATHLON_64_MODEL_15:
		case B_CPU_AMD_ATHLON_64_MODEL_20:
		case B_CPU_AMD_ATHLON_64_MODEL_23:
		case B_CPU_AMD_ATHLON_64_MODEL_24:
		case B_CPU_AMD_ATHLON_64_MODEL_27:
		case B_CPU_AMD_ATHLON_64_MODEL_28:
		case B_CPU_AMD_ATHLON_64_MODEL_31:
		case B_CPU_AMD_ATHLON_64_MODEL_35:
		case B_CPU_AMD_ATHLON_64_MODEL_43:
		case B_CPU_AMD_ATHLON_64_MODEL_44:
		case B_CPU_AMD_ATHLON_64_MODEL_47:
		case B_CPU_AMD_ATHLON_64_MODEL_63:
		case B_CPU_AMD_ATHLON_64_MODEL_79:
		case B_CPU_AMD_ATHLON_64_MODEL_95:
		case B_CPU_AMD_ATHLON_64_MODEL_127:
			return "Athlon 64";
		case B_CPU_AMD_OPTERON_MODEL_5:
		case B_CPU_AMD_OPTERON_MODEL_21:
		case B_CPU_AMD_OPTERON_MODEL_33:
		case B_CPU_AMD_OPTERON_MODEL_37:
		case B_CPU_AMD_OPTERON_MODEL_39:
			return "Opteron";
		case B_CPU_AMD_TURION_64_MODEL_36:
		case B_CPU_AMD_TURION_64_MODEL_76:
		case B_CPU_AMD_TURION_64_MODEL_104:
			return "Turion 64";
		case B_CPU_AMD_PHENOM_MODEL_2:
			return "Phenom";
		case B_CPU_AMD_PHENOM_II_MODEL_4:
		case B_CPU_AMD_PHENOM_II_MODEL_5:
		case B_CPU_AMD_PHENOM_II_MODEL_6:
		case B_CPU_AMD_PHENOM_II_MODEL_10:
			get_cpuid_model_string(cpuidName);
			if (strcasestr(cpuidName, "Athlon") != NULL)
				return "Athlon II";
			return "Phenom II";
		case B_CPU_AMD_A_SERIES:
			return "A-Series";
		case B_CPU_AMD_C_SERIES:
			return "C-Series";
		case B_CPU_AMD_E_SERIES:
			return "E-Series";
		case B_CPU_AMD_FX_SERIES:
			return "FX-Series";

		/* Transmeta */
		case B_CPU_TRANSMETA_CRUSOE:
			return "Crusoe";
		case B_CPU_TRANSMETA_EFFICEON:
		case B_CPU_TRANSMETA_EFFICEON_2:
			return "Efficeon";

		/* IDT/VIA */
		case B_CPU_IDT_WINCHIP_C6:
			return "WinChip C6";
		case B_CPU_IDT_WINCHIP_2:
			return "WinChip 2";
		case B_CPU_VIA_C3_SAMUEL:
			return "C3 Samuel";
		case B_CPU_VIA_C3_SAMUEL_2:
			/* stepping identified the model */
			if ((info->cpu_revision & 0xf) < 8)
				return "C3 Eden/Samuel 2";
			return "C3 Ezra";
		case B_CPU_VIA_C3_EZRA_T:
			return "C3 Ezra-T";
		case B_CPU_VIA_C3_NEHEMIAH:
			/* stepping identified the model */
			if ((info->cpu_revision & 0xf) < 8)
				return "C3 Nehemiah";
			return "C3 Eden-N";
		case B_CPU_VIA_C7_ESTHER:
		case B_CPU_VIA_C7_ESTHER_2:
			return "C7";
		case B_CPU_VIA_NANO_ISAIAH:
			return "Nano";

		/* Cyrix/VIA */
		case B_CPU_CYRIX_GXm:
			return "GXm";
		case B_CPU_CYRIX_6x86MX:
			return "6x86MX";

		/* Rise */
		case B_CPU_RISE_mP6:
			return "mP6";

		/* National Semiconductor */
		case B_CPU_NATIONAL_GEODE_GX1:
			return "Geode GX1";
#endif	/* __INTEL__ */

		default:
			if ((info->cpu_type & B_CPU_x86_VENDOR_MASK) == B_CPU_INTEL_x86) {
				// Fallback to manual parsing of the model string
				get_cpuid_model_string(cpuidName);
				return parse_intel(cpuidName);
			}
			return NULL;
	}
}


void
get_cpu_type(char *vendorBuffer, size_t vendorSize, char *modelBuffer,
	size_t modelSize)
{
	const char *vendor, *model;
	system_info info;

	get_system_info(&info);

	vendor = get_cpu_vendor_string(info.cpu_type);
	if (vendor == NULL)
		vendor = "Unknown";

	model = get_cpu_model_string(&info);
	if (model == NULL)
		model = "Unknown";

#ifdef R5_COMPATIBLE
	strncpy(vendorBuffer, vendor, vendorSize - 1);
	vendorBuffer[vendorSize - 1] = '\0';
	strncpy(modelBuffer, model, modelSize - 1);
	modelBuffer[modelSize - 1] = '\0';
#else
	strlcpy(vendorBuffer, vendor, vendorSize);
	strlcpy(modelBuffer, model, modelSize);
#endif
}


int32
get_rounded_cpu_speed(void)
{
	system_info info;

	int target, frac, delta;
	int freqs[] = { 100, 50, 25, 75, 33, 67, 20, 40, 60, 80, 10, 30, 70, 90 };
	uint x;

	get_system_info(&info);
	target = info.cpu_clock_speed / 1000000;
	frac = target % 100;
	delta = -frac;

	for (x = 0; x < sizeof(freqs) / sizeof(freqs[0]); x++) {
		int ndelta = freqs[x] - frac;
		if (abs(ndelta) < abs(delta))
			delta = ndelta;
	}
	return target + delta;
}

