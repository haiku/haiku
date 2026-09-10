/*
 * Copyright (c) 1990 The Regents of the University of California.
 * Copyright (c) 2014-2016 The FreeBSD Foundation
 * Copyright 2026, Haiku, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Portions of this software were developed by Andrew Turner
 * under sponsorship from the FreeBSD Foundation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *	from: FreeBSD: src/sys/i386/include/cpu.h,v 1.62 2001/06/29
 */
#ifndef _SYSTEM_ARCH_CPU_TYPE_H
#define _SYSTEM_ARCH_CPU_TYPE_H


/* Implementers from FreeBSD, sys/arm64/include/cpu.h */
#define	CPU_IMPL_ARM		0x41
#define	CPU_IMPL_BROADCOM	0x42
#define	CPU_IMPL_CAVIUM		0x43
#define	CPU_IMPL_DEC		0x44
#define	CPU_IMPL_FUJITSU	0x46
#define	CPU_IMPL_HISILICON	0x48
#define	CPU_IMPL_INFINEON	0x49
#define	CPU_IMPL_FREESCALE	0x4D
#define	CPU_IMPL_NVIDIA		0x4E
#define	CPU_IMPL_APM		0x50
#define	CPU_IMPL_QUALCOMM	0x51
#define	CPU_IMPL_MARVELL	0x56
#define	CPU_IMPL_APPLE		0x61
#define	CPU_IMPL_INTEL		0x69
#define	CPU_IMPL_AMPERE		0xC0
#define	CPU_IMPL_MICROSOFT	0x6D

/* Part numbers from FreeBSD, sys/arm64/arm64/identcpu.c */

/* ARM Part numbers */
#define	CPU_PART_FOUNDATION		0xD00
#define	CPU_PART_CORTEX_A34		0xD02
#define	CPU_PART_CORTEX_A53		0xD03
#define	CPU_PART_CORTEX_A35		0xD04
#define	CPU_PART_CORTEX_A55		0xD05
#define	CPU_PART_CORTEX_A65		0xD06
#define	CPU_PART_CORTEX_A57		0xD07
#define	CPU_PART_CORTEX_A72		0xD08
#define	CPU_PART_CORTEX_A73		0xD09
#define	CPU_PART_CORTEX_A75		0xD0A
#define	CPU_PART_CORTEX_A76		0xD0B
#define	CPU_PART_NEOVERSE_N1	0xD0C
#define	CPU_PART_CORTEX_A77		0xD0D
#define	CPU_PART_CORTEX_A76AE	0xD0E
#define	CPU_PART_AEM_V8			0xD0F
#define	CPU_PART_NEOVERSE_V1	0xD40
#define	CPU_PART_CORTEX_A78		0xD41
#define	CPU_PART_CORTEX_A78AE	0xD42
#define	CPU_PART_CORTEX_A65AE	0xD43
#define	CPU_PART_CORTEX_X1		0xD44
#define	CPU_PART_CORTEX_A510	0xD46
#define	CPU_PART_CORTEX_A710	0xD47
#define	CPU_PART_CORTEX_X2		0xD48
#define	CPU_PART_NEOVERSE_N2	0xD49
#define	CPU_PART_NEOVERSE_E1	0xD4A
#define	CPU_PART_CORTEX_A78C	0xD4B
#define	CPU_PART_CORTEX_X1C		0xD4C
#define	CPU_PART_CORTEX_A715	0xD4D
#define	CPU_PART_CORTEX_X3		0xD4E
#define	CPU_PART_NEOVERSE_V2	0xD4F
#define	CPU_PART_CORTEX_A520	0xD80
#define	CPU_PART_CORTEX_A720	0xD81
#define	CPU_PART_CORTEX_X4		0xD82
#define	CPU_PART_NEOVERSE_V3AE	0xD83
#define	CPU_PART_NEOVERSE_V3	0xD84
#define	CPU_PART_CORTEX_X925	0xD85
#define	CPU_PART_CORTEX_A725	0xD87
#define	CPU_PART_C1_NANO		0xD8A
#define	CPU_PART_C1_PRO			0xD8B
#define	CPU_PART_C1_ULTRA		0xD8C
#define	CPU_PART_NEOVERSE_N3	0xD8E
#define	CPU_PART_C1_PREMIUM		0xD90

/* Cavium Part numbers */
#define	CPU_PART_THUNDERX		0x0A1
#define	CPU_PART_THUNDERX_81XX	0x0A2
#define	CPU_PART_THUNDERX_83XX	0x0A3
#define	CPU_PART_THUNDERX2		0x0AF

#define	CPU_REV_THUNDERX_1_0	0x00
#define	CPU_REV_THUNDERX_1_1	0x01

#define	CPU_REV_THUNDERX2_0	0x00

/* APM (now Ampere) Part number */
#define CPU_PART_EMAG8180	0x000

/* Ampere Part numbers */
#define	CPU_PART_AMPERE1	0xAC3
#define	CPU_PART_AMPERE1A	0xAC4

/* Microsoft Part numbers */
#define	CPU_PART_AZURE_COBALT_100	0xD49

/* Qualcomm */
#define	CPU_PART_KRYO400_GOLD	0x804
#define	CPU_PART_KRYO400_SILVER	0x805

/* Apple part numbers */
#define CPU_PART_M1_ICESTORM      0x022
#define CPU_PART_M1_FIRESTORM     0x023
#define CPU_PART_M1_ICESTORM_PRO  0x024
#define CPU_PART_M1_FIRESTORM_PRO 0x025
#define CPU_PART_M1_ICESTORM_MAX  0x028
#define CPU_PART_M1_FIRESTORM_MAX 0x029
#define CPU_PART_M2_BLIZZARD      0x032
#define CPU_PART_M2_AVALANCHE     0x033
#define CPU_PART_M2_BLIZZARD_PRO  0x034
#define CPU_PART_M2_AVALANCHE_PRO 0x035
#define CPU_PART_M2_BLIZZARD_MAX  0x038
#define CPU_PART_M2_AVALANCHE_MAX 0x039

/* MIDR_EL1 field extraction */
#define	CPU_IMPL(midr)	(((midr) >> 24) & 0xff)
#define	CPU_PART(midr)	(((midr) >> 4) & 0xfff)
#define	CPU_VAR(midr)	(((midr) >> 20) & 0xf)
#define	CPU_REV(midr)	(((midr) >> 0) & 0xf)

#define	CPU_IMPL_TO_MIDR(val)	(((val) & 0xff) << 24)
#define	CPU_PART_TO_MIDR(val)	(((val) & 0xfff) << 4)
#define	CPU_VAR_TO_MIDR(val)	(((val) & 0xf) << 20)
#define	CPU_REV_TO_MIDR(val)	(((val) & 0xf) << 0)

#define	CPU_IMPL_MASK	(0xff << 24)
#define	CPU_PART_MASK	(0xfff << 4)
#define	CPU_VAR_MASK	(0xf << 20)
#define	CPU_REV_MASK	(0xf << 0)

#define	CPU_ID_RAW(impl, part, var, rev)		\
    (CPU_IMPL_TO_MIDR((impl)) |				\
    CPU_PART_TO_MIDR((part)) | CPU_VAR_TO_MIDR((var)) |	\
    CPU_REV_TO_MIDR((rev)))

struct cpu_parts {
	uint32		part_id;
	const char	*part_name;
};
#define CPU_PART_NONE {0, NULL}

struct cpu_implementers {
	uint32			impl_id;
	/*
	 * Part number is implementation defined
	 * so each vendor will have its own set of values and names.
	 */
	const struct cpu_parts	*cpu_parts;
};
#define CPU_IMPLEMENTER_NONE {0, NULL}

/*
 * Per-implementer table of (PartNum, CPU Name) pairs.
 */
/* ARM Ltd. */
static const struct cpu_parts cpu_parts_arm[] = {
	{CPU_PART_AEM_V8, "AEMv8"},
	{CPU_PART_FOUNDATION, "Foundation-Model"},
	{CPU_PART_CORTEX_A34, "Cortex-A34"},
	{CPU_PART_CORTEX_A35, "Cortex-A35"},
	{CPU_PART_CORTEX_A53, "Cortex-A53"},
	{CPU_PART_CORTEX_A55, "Cortex-A55"},
	{CPU_PART_CORTEX_A57, "Cortex-A57"},
	{CPU_PART_CORTEX_A65, "Cortex-A65"},
	{CPU_PART_CORTEX_A65AE, "Cortex-A65AE"},
	{CPU_PART_CORTEX_A72, "Cortex-A72"},
	{CPU_PART_CORTEX_A73, "Cortex-A73"},
	{CPU_PART_CORTEX_A75, "Cortex-A75"},
	{CPU_PART_CORTEX_A76, "Cortex-A76"},
	{CPU_PART_CORTEX_A76AE, "Cortex-A76AE"},
	{CPU_PART_CORTEX_A77, "Cortex-A77"},
	{CPU_PART_CORTEX_A78, "Cortex-A78"},
	{CPU_PART_CORTEX_A78AE, "Cortex-A78AE"},
	{CPU_PART_CORTEX_A78C, "Cortex-A78C"},
	{CPU_PART_CORTEX_A510, "Cortex-A510"},
	{CPU_PART_CORTEX_A520, "Cortex-A520"},
	{CPU_PART_CORTEX_A710, "Cortex-A710"},
	{CPU_PART_CORTEX_A715, "Cortex-A715"},
	{CPU_PART_CORTEX_A720, "Cortex-A720"},
	{CPU_PART_CORTEX_A725, "Cortex-A725"},
	{CPU_PART_CORTEX_X925, "Cortex-X925"},
	{CPU_PART_CORTEX_X1, "Cortex-X1"},
	{CPU_PART_CORTEX_X1C, "Cortex-X1C"},
	{CPU_PART_CORTEX_X2, "Cortex-X2"},
	{CPU_PART_CORTEX_X3, "Cortex-X3"},
	{CPU_PART_CORTEX_X4, "Cortex-X4"},
	{CPU_PART_C1_NANO, "C1-Nano"},
	{CPU_PART_C1_PRO, "C1-Pro"},
	{CPU_PART_C1_PREMIUM, "C1-Premium"},
	{CPU_PART_C1_ULTRA, "C1-Ultra"},
	{CPU_PART_NEOVERSE_E1, "Neoverse-E1"},
	{CPU_PART_NEOVERSE_N1, "Neoverse-N1"},
	{CPU_PART_NEOVERSE_N2, "Neoverse-N2"},
	{CPU_PART_NEOVERSE_N3, "Neoverse-N3"},
	{CPU_PART_NEOVERSE_V1, "Neoverse-V1"},
	{CPU_PART_NEOVERSE_V2, "Neoverse-V2"},
	{CPU_PART_NEOVERSE_V3, "Neoverse-V3"},
	{CPU_PART_NEOVERSE_V3AE, "Neoverse-V3AE"},
	CPU_PART_NONE,
};

/* Cavium */
static const struct cpu_parts cpu_parts_cavium[] = {
	{CPU_PART_THUNDERX, "ThunderX"},
	{CPU_PART_THUNDERX2, "ThunderX2"},
	CPU_PART_NONE,
};

/* APM (now Ampere) */
static const struct cpu_parts cpu_parts_apm[] = {
	{CPU_PART_EMAG8180, "eMAG 8180"},
	CPU_PART_NONE,
};

/* Ampere */
static const struct cpu_parts cpu_parts_ampere[] = {
	{CPU_PART_AMPERE1, "AmpereOne AC03"},
	{CPU_PART_AMPERE1A, "AmpereOne AC04"},
	CPU_PART_NONE,
};

/* Microsoft */
static const struct cpu_parts cpu_parts_microsoft[] = {
	{CPU_PART_AZURE_COBALT_100, "Azure Cobalt 100"},
	CPU_PART_NONE,
};

/* Qualcomm */
static const struct cpu_parts cpu_parts_qcom[] = {
	{CPU_PART_KRYO400_GOLD, "Kryo 400 Gold"},
	{CPU_PART_KRYO400_SILVER, "Kryo 400 Silver"},
	CPU_PART_NONE,
};

/* Apple */
static const struct cpu_parts cpu_parts_apple[] = {
	{CPU_PART_M1_ICESTORM, "M1 Icestorm"},
	{CPU_PART_M1_FIRESTORM, "M1 Firestorm"},
	{CPU_PART_M1_ICESTORM_PRO, "M1 Pro Icestorm"},
	{CPU_PART_M1_FIRESTORM_PRO, "M1 Pro Firestorm"},
	{CPU_PART_M1_ICESTORM_MAX, "M1 Max Icestorm"},
	{CPU_PART_M1_FIRESTORM_MAX, "M1 Max Firestorm"},
	{CPU_PART_M2_BLIZZARD, "M2 Blizzard"},
	{CPU_PART_M2_AVALANCHE, "M2 Avalanche"},
	{CPU_PART_M2_BLIZZARD_PRO, "M2 Pro Blizzard"},
	{CPU_PART_M2_AVALANCHE_PRO, "M2 Pro Avalanche"},
	{CPU_PART_M2_BLIZZARD_MAX, "M2 Max Blizzard"},
	{CPU_PART_M2_AVALANCHE_MAX, "M2 Max Avalanche"},
	CPU_PART_NONE,
};

/* Unknown */
static const struct cpu_parts cpu_parts_none[] = {
	CPU_PART_NONE,
};

/*
 * Implementers table.
 */
static const struct cpu_implementers cpu_implementers[] = {
	{CPU_IMPL_AMPERE, cpu_parts_ampere},
	{CPU_IMPL_APPLE, cpu_parts_apple},
	{CPU_IMPL_APM, cpu_parts_apm},
	{CPU_IMPL_ARM, cpu_parts_arm},
	{CPU_IMPL_BROADCOM, cpu_parts_none},
	{CPU_IMPL_CAVIUM, cpu_parts_cavium},
	{CPU_IMPL_DEC, cpu_parts_none},
	{CPU_IMPL_FREESCALE, cpu_parts_none},
	{CPU_IMPL_FUJITSU, cpu_parts_none},
	{CPU_IMPL_HISILICON, cpu_parts_none},
	{CPU_IMPL_INFINEON, cpu_parts_none},
	{CPU_IMPL_INTEL, cpu_parts_none},
	{CPU_IMPL_MARVELL, cpu_parts_none},
	{CPU_IMPL_MICROSOFT, cpu_parts_microsoft},
	{CPU_IMPL_NVIDIA, cpu_parts_none},
	{CPU_IMPL_QUALCOMM, cpu_parts_qcom},
	CPU_IMPLEMENTER_NONE,
};


static inline const char*
get_cpu_model_string(enum cpu_platform platform, enum cpu_vendor cpuVendor, uint32 cpuModel)
{
	(void)cpuVendor;

	if (platform != B_CPU_ARM_64)
		return NULL;

	uint32 impl = CPU_IMPL(cpuModel);
	uint32 part = CPU_PART(cpuModel);

	for (int i = 0; cpu_implementers[i].impl_id != 0; i++) {
		if (cpu_implementers[i].impl_id != impl)
			continue;
		const struct cpu_parts* parts = cpu_implementers[i].cpu_parts;
		for (int j = 0; parts[j].part_name != NULL; j++) {
			if (parts[j].part_id == part)
				return parts[j].part_name;
		}
		break;
	}

	return NULL;
}

#endif	/* _SYSTEM_ARCH_CPU_TYPE_H */
