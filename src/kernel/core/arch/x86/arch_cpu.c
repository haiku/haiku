/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <arch/cpu.h>
#include <vm.h>
#include <smp.h>
#include <arch/x86/selector.h>
#include <tls.h>
#include <boot/kernel_args.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


static struct tss **sTSS;
static int *sIsTSSLoaded;

segment_descriptor *gGDT = NULL;


status_t
arch_cpu_preboot_init(kernel_args *args)
{
	write_dr3(0);
	return B_OK;
}


status_t
arch_cpu_init(kernel_args *args)
{
	setup_system_time(args->arch_args.system_time_cv_factor);

	return B_OK;
}


status_t
arch_cpu_init_post_vm(kernel_args *args)
{
	uint32 i;

	// account for the segment descriptors

	gGDT = (segment_descriptor *)args->arch_args.vir_gdt;
	create_area("gdt", (void **)&gGDT, B_EXACT_ADDRESS, B_PAGE_SIZE, B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	// currently taken out of the build, because it's not yet used (and assumes
	// (a fixed number of used GDT entries)
	//i386_selector_init(gGDT);  // pass the new gdt

	// setup task-state segments

	sTSS = malloc(sizeof(struct tss *) * args->num_cpus);
	if (sTSS == NULL) {
		panic("arch_cpu_init_post_vm: could not allocate buffer for tss pointers\n");
		return B_NO_MEMORY;
	}

	sIsTSSLoaded = malloc(sizeof(int) * args->num_cpus);
	if (sIsTSSLoaded == NULL) {
		panic("arch_cpu_init_post_vm: could not allocate buffer for tss booleans\n");
		return B_NO_MEMORY;
	}
	memset(sIsTSSLoaded, 0, sizeof(int) * args->num_cpus);

	for (i = 0; i < args->num_cpus; i++) {
		char tssName[16];
		area_id area;

		sprintf(tssName, "tss%lu", i);
		area = create_area(tssName, (void **)&sTSS[i], B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE,
			B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (area < 0) {
			panic("arch_cpu_init2: unable to create region for tss\n");
			return B_NO_MEMORY;
		}

		// initialize TSS
		memset(sTSS[i], 0, sizeof(struct tss));
		sTSS[i]->ss0 = KERNEL_DATA_SEG;

		// add TSS descriptor for this new TSS
		set_tss_descriptor(&gGDT[TSS_BASE_SEGMENT + i], (addr_t)sTSS[i], sizeof(struct tss));
	}

	// setup TLS descriptors (one for every CPU)

	for (i = 0; i < args->num_cpus; i++) {
		set_segment_descriptor(&gGDT[TLS_BASE_SEGMENT + i], 0, TLS_SIZE,
			DT_DATA_WRITEABLE, DPL_USER);
	}

	return 0;
}


void
i386_set_tss_and_kstack(addr_t kstack)
{
	int currentCPU = smp_get_current_cpu();

//	dprintf("i386_set_kstack: kstack 0x%x, cpu %d\n", kstack, currentCPU);
	if (!sIsTSSLoaded[currentCPU]) {
		short seg = ((TSS_BASE_SEGMENT + currentCPU) << 3) | DPL_KERNEL;
		asm("movw  %0, %%ax;"
			"ltr %%ax;" : : "r" (seg) : "eax");
		sIsTSSLoaded[currentCPU] = true;
	}

	sTSS[currentCPU]->sp0 = kstack;
//	dprintf("done\n");
}


void
arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
	int32 num_pages = end / B_PAGE_SIZE - start / B_PAGE_SIZE;
	while (num_pages-- >= 0) {
		invalidate_TLB(start);
		start += B_PAGE_SIZE;
	}
}


void
arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
	int i;
	for (i = 0; i < num_pages; i++) {
		invalidate_TLB(pages[i]);
	}
}


status_t
arch_cpu_user_memcpy(void *to, const void *from, size_t size, addr_t *fault_handler)
{
	char *tmp = (char *)to;
	char *s = (char *)from;

	*fault_handler = (addr_t)&&error;

	while (size--)
		*tmp++ = *s++;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
	return B_BAD_ADDRESS;
}


ssize_t
arch_cpu_user_strlcpy(char *to, const char *from, size_t size, addr_t *faultHandler)
{
	int from_length = 0;

	*faultHandler = (addr_t)&&error;

	if (size > 0) {
		to[--size] = '\0';
		// copy 
		for ( ; size; size--, from_length++, to++, from++) {
			if ((*to = *from) == '\0')
				break;
		}
	}
	// count any leftover from chars
	while (*from++ != '\0')
		from_length++;

	*faultHandler = 0;
	return from_length;

error:
	*faultHandler = 0;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_user_memset(void *s, char c, size_t count, addr_t *fault_handler)
{
	char *xs = (char *)s;

	*fault_handler = (addr_t)&&error;

	while (count--)
		*xs++ = c;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
	return B_BAD_ADDRESS;
}


void
arch_cpu_idle(void)
{
	switch (smp_get_num_cpus()) {
		case 0:
			panic("You need at least 1 CPU to run Haiku\n");
		case 1:
			asm("hlt");
		default:
			break;
	}
}


void
arch_cpu_sync_icache(void *address, size_t length)
{
	// instruction cache is always consistent on x86
}

