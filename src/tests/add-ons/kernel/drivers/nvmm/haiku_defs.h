#ifndef LIBNVMM_TESTS_HAIKU_DEFS
#define LIBNVMM_TESTS_HAIKU_DEFS

#include <OS.h>
#define PAGE_SIZE B_PAGE_SIZE

#define _ASSEMBLER
#include <arch/x86/descriptors.h>
#undef _ASSEMBLER

// taken from nvmm_os.h / DragonFlyBSD's segments.h
#define GSEL(s,r)       (((s) << 3) | r) /* a global selector */
#define GCODE_SEL       KERNEL_CODE_SEGMENT /* Kernel Code Descriptor */
#define GDATA_SEL       KERNEL_DATA_SEGMENT /* Kernel Data Descriptor */
#define SEL_KPL         DPL_KERNEL /* kernel privilege level */

// taken from libnvmm_x86.c
#define PTE_P           0x0000000000000001      /* Present */
#define PTE_W           0x0000000000000002      /* Write */

// taken from NetBSD
#define PSL_MBO		0x00000002
#define SDT_MEMRWA	19
#define SDT_MEMERA	27
#define SDT_SYSLDT	2
#define SDT_SYS386BSY	11

typedef uint64_t pt_entry_t;

static void err(int error_code, char *str)
{
	printf(str);
	printf("\n");
	exit(error_code);
}

#define errx(error_code, str) err(error_code, str)

#endif /* LIBNVMM_TESTS_HAIKU_DEFS */
