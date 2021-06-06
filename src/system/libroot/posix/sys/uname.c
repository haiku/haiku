/*
 * Copyright 2004-2007, Jérôme Duval jerome.duval@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/utsname.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <OS.h>
#include <ByteOrder.h>

#include <errno_private.h>
#include <system_revision.h>


int
uname(struct utsname *info)
{
	cpu_topology_node_info root;
	system_info systemInfo;
	const char *platform;
	const char *haikuRevision;
	uint32 count = 1;
	status_t error;

	if (!info) {
		__set_errno(B_BAD_VALUE);
		return -1;
	}

	get_system_info(&systemInfo);

	strlcpy(info->sysname, "Haiku", sizeof(info->sysname));

	haikuRevision = __get_haiku_revision();
	if (haikuRevision[0] != '\0')
		snprintf(info->version, sizeof(info->version), "%s ", haikuRevision);
	else
		info->version[0] = '\0';
	strlcat(info->version, systemInfo.kernel_build_date, sizeof(info->version));
	strlcat(info->version, " ", sizeof(info->version));
	strlcat(info->version, systemInfo.kernel_build_time, sizeof(info->version));
	snprintf(info->release, sizeof(info->release), "%" B_PRId64,
		systemInfo.kernel_version);

	error = get_cpu_topology_info(&root, &count);
	if (error != B_OK || count < 1)
		platform = "unknown";
	else {
		switch (root.data.root.platform) {
			case B_CPU_x86:
				platform = "BePC";
				break;
			case B_CPU_x86_64:
				platform = "x86_64";
				break;
			case B_CPU_PPC:
				platform = "ppc";
				break;
			case B_CPU_PPC_64:
				platform = "ppc64";
				break;
			case B_CPU_M68K:
				platform = "m68k";
				break;
			case B_CPU_ARM:
				/* The minimal ARM version emulated by QEMU
				 * XXX: use armv6 (raspberry Pi)?
				 * XXX: should we really use B_HOST_IS_LENDIAN here?
				 * XXX: use real cpu version as on Linux?
				 *	cf. http://git.qemu.org/?p=qemu.git;a=blob;f=linux-user/uname.c
				 */
#if B_HOST_IS_LENDIAN
				platform = "armv5tel";
#else
				platform = "armv5teb";
#endif
				break;
			case B_CPU_ARM_64:
				platform = "aarch64";
				break;
			case B_CPU_ALPHA:
				platform = "alpha";
				break;
			case B_CPU_MIPS:
				platform = "mips";
				break;
			case B_CPU_SH:
				platform = "sh4";
				break;
			case B_CPU_RISC_V:
				platform = "riscv64";
				break;
			case B_CPU_UNKNOWN:
			default:
				platform = "unknown";
				break;
		}
	}

	strlcpy(info->machine, platform, sizeof(info->machine));

	if (gethostname(info->nodename, sizeof(info->nodename)) != 0)
		strlcpy(info->nodename, "unknown", sizeof(info->nodename));

	return 0;
}

