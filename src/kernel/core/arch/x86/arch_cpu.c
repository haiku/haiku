/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <arch/cpu.h>
#include <malloc.h>
#include <vm.h>
#include <debug.h>
#include <smp.h>
#include <arch/x86/selector.h>
#include <Errors.h>
#include <kerrors.h>
#include <tls.h>

#include <boot/stage2.h>

#include <string.h>
#include <stdio.h>

static struct tss **tss;
static int *tss_loaded;

segment_descriptor *gGDT = NULL;


int
arch_cpu_preboot_init(kernel_args *ka)
{
	write_dr3(0);
	return 0;
}


int
arch_cpu_init(kernel_args *ka)
{
	setup_system_time(ka->arch_args.system_time_cv_factor);

	return 0;
}


int
arch_cpu_init2(kernel_args *ka)
{
	unsigned int i;

	// account for the segment descriptors

	gGDT = (segment_descriptor *)ka->arch_args.vir_gdt;
	vm_create_anonymous_region(vm_get_kernel_aspace_id(), "gdt", (void **)&gGDT,
		REGION_ADDR_EXACT_ADDRESS, PAGE_SIZE, REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);

	// currently taken out of the build, because it's not yet used (and assumes
	// (a fixed number of used GDT entries)
	//i386_selector_init(gGDT);  // pass the new gdt

	// setup task-state segments

	tss = malloc(sizeof(struct tss *) * ka->num_cpus);
	if (tss == NULL) {
		panic("arch_cpu_init2: could not allocate buffer for tss pointers\n");
		return B_NO_MEMORY;
	}

	tss_loaded = malloc(sizeof(int) * ka->num_cpus);
	if (tss == NULL) {
		panic("arch_cpu_init2: could not allocate buffer for tss booleans\n");
		return B_NO_MEMORY;
	}
	memset(tss_loaded, 0, sizeof(int) * ka->num_cpus);

	for (i = 0; i < ka->num_cpus; i++) {
		char tss_name[16];
		region_id rid;

		sprintf(tss_name, "tss%d", i);
		rid = vm_create_anonymous_region(vm_get_kernel_aspace_id(), tss_name, (void **)&tss[i],
			REGION_ADDR_ANY_ADDRESS, PAGE_SIZE, REGION_WIRING_WIRED, LOCK_RW|LOCK_KERNEL);
		if (rid < 0) {
			panic("arch_cpu_init2: unable to create region for tss\n");
			return B_NO_MEMORY;
		}

		// initialize TSS
		memset(tss[i], 0, sizeof(struct tss));
		tss[i]->ss0 = KERNEL_DATA_SEG;

		// add TSS descriptor for this new TSS
		set_tss_descriptor(&gGDT[TSS_BASE_SEGMENT + i], (addr)tss[i], sizeof(struct tss));
	}

	// setup TLS descriptors (one for every CPU)

	for (i = 0; i < ka->num_cpus; i++) {
		set_segment_descriptor(&gGDT[TLS_BASE_SEGMENT + i], 0, TLS_SIZE,
			DT_DATA_WRITEABLE, DPL_USER);
	}

	return 0;
}


void
i386_set_tss_and_kstack(addr kstack)
{
	int currentCPU = smp_get_current_cpu();

//	dprintf("i386_set_kstack: kstack 0x%x, cpu %d\n", kstack, currentCPU);
	if (!tss_loaded[currentCPU]) {
		short seg = ((TSS_BASE_SEGMENT + currentCPU) << 3) | DPL_KERNEL;
		asm("movw  %0, %%ax;"
			"ltr %%ax;" : : "r" (seg) : "eax");
		tss_loaded[currentCPU] = true;
	}

	tss[currentCPU]->sp0 = kstack;
//	dprintf("done\n");
}


void
arch_cpu_invalidate_TLB_range(addr start, addr end)
{
	int num_pages = end/PAGE_SIZE - start/PAGE_SIZE;
	while (num_pages-- >= 0) {
		invalidate_TLB(start);
		start += PAGE_SIZE;
	}
}


void
arch_cpu_invalidate_TLB_list(addr pages[], int num_pages)
{
	int i;
	for (i = 0; i < num_pages; i++) {
		invalidate_TLB(pages[i]);
	}
}


int
arch_cpu_user_memcpy(void *to, const void *from, size_t size, addr *fault_handler)
{
	char *tmp = (char *)to;
	char *s = (char *)from;

	*fault_handler = (addr)&&error;

	while (size--)
		*tmp++ = *s++;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
	return B_BAD_ADDRESS;
}


int
arch_cpu_user_strcpy(char *to, const char *from, addr *fault_handler)
{
	*fault_handler = (addr)&&error;

	while ((*to++ = *from++) != '\0')
		;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
	return B_BAD_ADDRESS;
}


int
arch_cpu_user_strncpy(char *to, const char *from, size_t size, addr *fault_handler)
{
	*fault_handler = (addr)&&error;

	while(size-- && (*to++ = *from++) != '\0')
		;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
	return B_BAD_ADDRESS;
}

/*!	\brief Copies at most (\a size - 1) characters from the string in \a from to
	the string in \a to, NULL-terminating the result.

	\param to Pointer to the destination C-string.
	\param from Pointer to the source C-string.
	\param size Size in bytes of the string buffer pointed to by \a to.
	
	\return strlen(\a from).
*/
int
arch_cpu_user_strlcpy(char *to, const char *from, size_t size, addr *faultHandler)
{
	int from_length = 0;

	*faultHandler = (addr)&&error;

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


int
arch_cpu_user_memset(void *s, char c, size_t count, addr *fault_handler)
{
	char *xs = (char *)s;

	*fault_handler = (addr)&&error;

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
			panic("You need at least 1 CPU to run OpenBeOS\n");
		case 1:
			asm("hlt");
		default:
			break;
	}
}

