/*
 * Copyright 2004-2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */

/* Taken from the Pulse application, and extended.
 * It's used by Pulse, AboutHaiku, and sysinfo.
 */

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include <OS.h>


#ifdef __cplusplus
extern "C" {
#endif

static const char* get_cpu_vendor_string(enum cpu_vendor cpuVendor);
static const char* get_cpu_model_string(enum cpu_platform platform,
	enum cpu_vendor cpuVendor, uint32 cpuModel);
void get_cpu_type(char *vendorBuffer, size_t vendorSize,
		char *modelBuffer, size_t modelSize);
int32 get_rounded_cpu_speed(void);

#ifdef __cplusplus
}
#endif


#if defined(__INTEL__) || defined(__x86_64__)
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


static const char*
parse_amd(const char* name)
{
	static char buffer[49];

	// ignore initial spaces
	int index = 0;
	for (; name[index] != '\0'; index++) {
		if (name[index] != ' ')
			break;
	}

	// Keep an initial "mobile"
	int outIndex = 0;
	bool spaceWritten = false;
	if (!strncasecmp(&name[index], "Mobile ", 7)) {
		strcpy(buffer, "Mobile ");
		spaceWritten = true;
		outIndex += 7;
		index += 7;
	}

	// parse model
	for (; name[index] != '\0'; index++) {
		if (!strncasecmp(&name[index], "(r)", 3)) {
			outIndex += strlcpy(&buffer[outIndex], "®",
				sizeof(buffer) - outIndex);
			index += 2;
		} else if (!strncasecmp(&name[index], "(tm)", 4)) {
			outIndex += strlcpy(&buffer[outIndex], "™",
				sizeof(buffer) - outIndex);
			index += 3;
		} else if (!strncmp(&name[index], "with ", 5)
			|| !strncmp(&name[index], "/w", 2)) {
			// Cut off the rest
			break;
		} else if (name[index] == '-') {
			if (!spaceWritten)
				buffer[outIndex++] = ' ';
			spaceWritten = true;
		} else {
			const char* kWords[] = {
				"Eight-core", "6-core", "Six-core", "Quad-core", "Dual-core",
				"Dual core", "Processor", "APU", "AMD", "Intel", "Integrated",
				"CyrixInstead", "Advanced Micro Devices", "Comb", "DualCore",
				"Technology", "Mobile", "Triple-Core"
			};
			bool removed = false;
			for (size_t i = 0; i < sizeof(kWords) / sizeof(kWords[0]); i++) {
				size_t length = strlen(kWords[i]);
				if (!strncasecmp(&name[index], kWords[i], length)) {
					index += length - 1;
					removed = true;
					break;
				}
			}
			if (removed)
				continue;

			if (name[index] == ' ') {
				if (spaceWritten)
					continue;
				spaceWritten = true;
			} else
				spaceWritten = false;
			buffer[outIndex++] = name[index];
		}
	}

	// cut off trailing spaces
	while (outIndex > 1 && buffer[outIndex - 1] == ' ')
		outIndex--;

	buffer[outIndex] = '\0';

	// skip new initial spaces
	for (outIndex = 0; buffer[outIndex] != '\0'; outIndex++) {
		if (buffer[outIndex] != ' ')
			break;
	}
	return buffer + outIndex;
}
#endif


static const char*
get_cpu_vendor_string(enum cpu_vendor cpuVendor)
{
	// Should match vendors in OS.h
	static const char* vendorStrings[] = {
		NULL, "AMD", "Cyrix", "IDT", "Intel", "National Semiconductor", "Rise",
		"Transmeta", "VIA", "IBM", "Motorola", "NEC"
	};

	if ((size_t)cpuVendor >= sizeof(vendorStrings) / sizeof(const char*))
		return NULL;
	return vendorStrings[cpuVendor];
}


#if defined(__INTEL__) || defined(__x86_64__)
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
#endif	/* __INTEL__ || __x86_64__ */


static const char*
get_cpu_model_string(enum cpu_platform platform, enum cpu_vendor cpuVendor,
	uint32 cpuModel)
{
#if defined(__INTEL__) || defined(__x86_64__)
	char cpuidName[49];
#endif

	(void)cpuVendor;
	(void)cpuModel;

#if defined(__INTEL__) || defined(__x86_64__)
	if (platform != B_CPU_x86 && platform != B_CPU_x86_64)
		return NULL;

	// XXX: This *really* isn't accurate. There is differing math
	// based on the CPU vendor.. Don't use these numbers anywhere
	// except "fast and dumb" identification of processor names.
	//
	// see cpuidtool.c to decode cpuid signatures (sysinfo) into a
	// value for this function.
	//
	// sysinfo has code in it which obtains the proper fam/mod/step ids

	uint16 family = ((cpuModel >> 8) & 0xf) | ((cpuModel >> 16) & 0xff0);
	uint16 model = ((cpuModel >> 4) & 0xf) | ((cpuModel >> 12) & 0xf0);
	uint8 stepping = cpuModel & 0xf;

	if (cpuVendor == B_CPU_VENDOR_AMD) {
		if (family == 5) {
			if (model <= 3)
				return "K5";
			if (model <= 7)
				return "K6";
			if (model == 8)
				return "K6-2";
			if (model == 9 || model == 0xd)
				return "K6-III";
			if (model == 0xa)
				return "Geode LX";
		} else if (family == 6) {
			if (model <= 2 || model == 4)
				return "Athlon";
			if (model == 3)
				return "Duron";
			if (model <= 8 || model == 0xa)
				return "Athlon XP";
		} else if (family == 0xf) {
			if (model <= 4 || model == 7 || model == 8
				|| (model >= 0xb && model <= 0xf) || model == 0x14
				|| model == 0x18 || model == 0x1b || model == 0x1f
				|| model == 0x23 || model == 0x2b
				|| ((model & 0xf) == 0xf && model >= 0x2f && model <= 0x7e)) {
				return "Athlon 64";
			}
			if (model == 5 || model == 0x15 || model == 0x21 || model == 0x25
				|| model == 0x27) {
				return "Opteron";
			}
			if (model == 0x1c || model == 0x2c || model == 0x7f)
				return "Sempron 64";
			if (model == 0x24 || model == 0x4c || model == 0x68)
				return "Turion 64";
		} else if (family == 0x1f) {
			if (model == 2)
				return "Phenom";
			if ((model >= 4 && model <= 6) || model == 0xa) {
				get_cpuid_model_string(cpuidName);
				if (strcasestr(cpuidName, "Athlon") != NULL)
					return "Athlon II";
				return "Phenom II";
			}
		} else if (family == 0x3f)
			return "A-Series";
		else if (family == 0x5f) {
			if (model == 1)
				return "C-Series";
			if (model == 2)
				return "E-Series";
		} else if (family == 0x6f) {
			if (model == 1 || model == 2)
				return "FX-Series";
			if (model == 0x10 || model == 0x13)
				return "A-Series";
		} else if (family == 0x8f)
			return "Ryzen 7";

		// Fallback to manual parsing of the model string
		get_cpuid_model_string(cpuidName);
		return parse_amd(cpuidName);
	}

	if (cpuVendor == B_CPU_VENDOR_CYRIX) {
		if (family == 5 && model == 4)
			return "GXm";
		if (family == 6)
			return "6x86MX";
		return NULL;
	}

	if (cpuVendor == B_CPU_VENDOR_INTEL) {
		if (family == 5) {
			if (model == 1 || model == 2)
				return "Pentium";
			if (model == 3 || model == 9)
				return "Pentium OD";
			if (model == 4 || model == 8)
				return "Pentium MMX";
		} else if (family == 6) {
			if (model == 1)
				return "Pentium Pro";
			if (model == 3 || model == 5)
				return "Pentium II";
			if (model == 6)
				return "Celeron";
			if (model == 7 || model == 8 || model == 0xa || model == 0xb)
				return "Pentium III";
			if (model == 9 || model == 0xd) {
				get_cpuid_model_string(cpuidName);
				if (strcasestr(cpuidName, "Celeron") != NULL)
					return "Pentium M Celeron";
				return "Pentium M";
			}
			if (model == 0x1c || model == 0x26 || model == 0x36)
				return "Atom";
			if (model == 0xe) {
				get_cpuid_model_string(cpuidName);
				if (strcasestr(cpuidName, "Celeron") != NULL)
					return "Core Celeron";
				return "Core";
			}
			if (model == 0xf || model == 0x17) {
                get_cpuid_model_string(cpuidName);
				if (strcasestr(cpuidName, "Celeron") != NULL)
					return "Core 2 Celeron";
				if (strcasestr(cpuidName, "Xeon") != NULL)
					return "Core 2 Xeon";
				if (strcasestr(cpuidName, "Pentium") != NULL)
					return "Pentium";
				if (strcasestr(cpuidName, "Extreme") != NULL)
					return "Core 2 Extreme";
				return "Core 2";
			}
			if (model == 0x25)
				return "Core i5";
			if (model == 0x1a || model == 0x1e) {
				get_cpuid_model_string(cpuidName);
				if (strcasestr(cpuidName, "Xeon") != NULL)
					return "Core i7 Xeon";
				return "Core i7";
			}
		} else if (family == 0xf) {
			if (model <= 4) {
				get_cpuid_model_string(cpuidName);
				if (strcasestr(cpuidName, "Celeron") != NULL)
					return "Pentium 4 Celeron";
				if (strcasestr(cpuidName, "Xeon") != NULL)
					return "Pentium 4 Xeon";
				return "Pentium 4";
			}
		}

		// Fallback to manual parsing of the model string
		get_cpuid_model_string(cpuidName);
		return parse_intel(cpuidName);
	}

	if (cpuVendor == B_CPU_VENDOR_NATIONAL_SEMICONDUCTOR) {
		if (family == 5) {
			if (model == 4)
				return "Geode GX1";
			if (model == 5)
				return "Geode GX2";
			return NULL;
		}
	}

	if (cpuVendor == B_CPU_VENDOR_RISE) {
		if (family == 5)
			return "mP6";
		return NULL;
	}

	if (cpuVendor == B_CPU_VENDOR_TRANSMETA) {
		if (family == 5 && model == 4)
			return "Crusoe";
		if (family == 0xf && (model == 2 || model == 3))
			return "Efficeon";
		return NULL;
	}

	if (cpuVendor == B_CPU_VENDOR_VIA) {
		if (family == 5) {
			if (model == 4)
				return "WinChip C6";
			if (model == 8)
				return "WinChip 2";
			if (model == 9)
				return "WinChip 3";
			return NULL;
		} else if (family == 6) {
			if (model == 6)
				return "C3 Samuel";
			if (model == 7) {
				if (stepping < 8)
					return "C3 Eden/Samuel 2";
				return "C3 Ezra";
			}
			if (model == 8)
				return "C3 Ezra-T";
			if (model == 9) {
				if (stepping < 8)
					return "C3 Nehemiah";
				return "C3 Ezra-N";
			}
			if (model == 0xa || model == 0xd)
				return "C7";
			if (model == 0xf)
				return "Nano";
			return NULL;
		}
	}

#endif

	return NULL;
}


void
get_cpu_type(char *vendorBuffer, size_t vendorSize, char *modelBuffer,
	size_t modelSize)
{
	const char *vendor, *model;

	uint32 topologyNodeCount = 0;
	cpu_topology_node_info* topology = NULL;
	get_cpu_topology_info(NULL, &topologyNodeCount);
	if (topologyNodeCount != 0)
		topology = new cpu_topology_node_info[topologyNodeCount];
	get_cpu_topology_info(topology, &topologyNodeCount);

	enum cpu_platform platform = B_CPU_UNKNOWN;
	enum cpu_vendor cpuVendor = B_CPU_VENDOR_UNKNOWN;
	uint32 cpuModel = 0;
	for (uint32 i = 0; i < topologyNodeCount; i++) {
		switch (topology[i].type) {
			case B_TOPOLOGY_ROOT:
				platform = topology[i].data.root.platform;
				break;

			case B_TOPOLOGY_PACKAGE:
				cpuVendor = topology[i].data.package.vendor;
				break;

			case B_TOPOLOGY_CORE:
				cpuModel = topology[i].data.core.model;
				break;

			default:
				break;
		}
	}
	delete[] topology;

	vendor = get_cpu_vendor_string(cpuVendor);
	if (vendor == NULL)
		vendor = "Unknown";

	model = get_cpu_model_string(platform, cpuVendor, cpuModel);
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
	uint32 topologyNodeCount = 0;
	cpu_topology_node_info* topology = NULL;
	get_cpu_topology_info(NULL, &topologyNodeCount);
	if (topologyNodeCount != 0)
		topology = new cpu_topology_node_info[topologyNodeCount];
	get_cpu_topology_info(topology, &topologyNodeCount);

	uint64 cpuFrequency = 0;
	for (uint32 i = 0; i < topologyNodeCount; i++) {
		if (topology[i].type == B_TOPOLOGY_CORE) {
				cpuFrequency = topology[i].data.core.default_frequency;
				break;
		}
	}
	delete[] topology;

	int target, frac, delta;
	int freqs[] = { 100, 50, 25, 75, 33, 67, 20, 40, 60, 80, 10, 30, 70, 90 };
	uint x;

	target = cpuFrequency / 1000000;
	frac = target % 100;
	delta = -frac;

	for (x = 0; x < sizeof(freqs) / sizeof(freqs[0]); x++) {
		int ndelta = freqs[x] - frac;
		if (abs(ndelta) < abs(delta))
			delta = ndelta;
	}
	return target + delta;
}

