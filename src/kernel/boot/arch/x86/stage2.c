/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <bootdir.h>
#include <stage2.h>
#include "arch/x86/stage2_priv.h"
#include "vesa.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <elf32.h>

const unsigned kBSSSize = 0x9000;


// we're running out of the first 'file' contained in the bootdir, which is
// a set of binaries and data packed back to back, described by an array
// of boot_entry structures at the beginning. The load address is fixed.
#define BOOTDIR_ADDR 0x100000
static const boot_entry *bootdir = (boot_entry*)BOOTDIR_ADDR;

// stick the kernel arguments in a pseudo-random page that will be mapped
// at least during the call into the kernel. The kernel should copy the
// data out and unmap the page.
static kernel_args *ka = (kernel_args *)0x20000;

// needed for message
static unsigned short *kScreenBase = (unsigned short*) 0xb8000;
static unsigned screenOffset = 0;
static unsigned int line = 0;

unsigned int cv_factor = 0;

// size of bootdir in pages
static unsigned int bootdir_pages = 0;

// working pagedir and pagetable
static unsigned int *pgdir = 0;
static unsigned int *pgtable = 0;

// function decls for this module
static void calculate_cpu_conversion_factor(void);
static void load_elf_image(void *data, unsigned int *next_paddr,
	addr_range *ar0, addr_range *ar1, unsigned int *start_addr, addr_range *dynamic_section);
static int mmu_init(kernel_args *ka, unsigned int *next_paddr);
static void mmu_map_page(unsigned int vaddr, unsigned int paddr);
static int check_cpu(void);

// called by the stage1 bootloader.
// State:
//   32-bit
//   mmu disabled
//   stack somewhere below 1 MB
//   supervisor mode
void _start(unsigned int mem, int in_vesa, unsigned int vesa_ptr)
{
	unsigned int *idt;
	unsigned int *gdt;
	unsigned int next_vaddr;
	unsigned int next_paddr;
	unsigned int i;
	unsigned int kernel_entry;

	asm("cld");			// Ain't nothing but a GCC thang.
	asm("fninit");		// initialize floating point unit

	clearscreen();
	dprintf("stage2 bootloader entry.\n");
	dprintf("memsize = 0x%x, in_vesa %d, vesa_ptr 0x%x\n", mem, in_vesa, vesa_ptr);

	// verify we can run on this cpu
	if(check_cpu() < 0) {
		dprintf("\nSorry, this computer appears to be lacking some of the features\n");
		dprintf("needed by OpenBeOS. It is currently only able to run on\n");
		dprintf("Pentium class cpus and above, with a few exceptions to\n");
		dprintf("that rule.\n");
		dprintf("\nPlease reset your computer to continue.");

		for(;;);
	}

	// calculate the conversion factor that translates rdtsc time to real microseconds
	calculate_cpu_conversion_factor();

	// calculate how big the bootdir is so we know where we can start grabbing pages
	{
		int entry;
		for (entry = 0; entry < 64; entry++) {
			if (bootdir[entry].be_type == BE_TYPE_NONE)
				break;

			bootdir_pages += bootdir[entry].be_size;
		}

//		nmessage("bootdir is ", bootdir_pages, " pages long\n");
	}

	ka->bootdir_addr.start = (unsigned long)bootdir;
	ka->bootdir_addr.size = bootdir_pages * PAGE_SIZE;

	next_paddr = BOOTDIR_ADDR + bootdir_pages * PAGE_SIZE;

	if(in_vesa) {
		struct VBEModeInfoBlock *mode_info = (struct VBEModeInfoBlock *)(vesa_ptr + 0x200);

		ka->fb.enabled = 1;
		ka->fb.x_size = mode_info->x_resolution;
		ka->fb.y_size = mode_info->y_resolution;
		ka->fb.bit_depth = mode_info->bits_per_pixel;
		ka->fb.mapping.start = mode_info->phys_base_ptr;
		ka->fb.mapping.size = ka->fb.x_size * ka->fb.y_size * (ka->fb.bit_depth/8);
		ka->fb.already_mapped = 0;
	} else {
		ka->fb.enabled = 0;
	}

	mmu_init(ka, &next_paddr);

	// load the kernel (3rd entry in the bootdir)
	load_elf_image((void *)(bootdir[2].be_offset * PAGE_SIZE + BOOTDIR_ADDR), &next_paddr,
			&ka->kernel_seg0_addr, &ka->kernel_seg1_addr, &kernel_entry, &ka->kernel_dynamic_section_addr);

	if(ka->kernel_seg1_addr.size > 0)
		next_vaddr = ROUNDUP(ka->kernel_seg1_addr.start + ka->kernel_seg1_addr.size, PAGE_SIZE);
	else
		next_vaddr = ROUNDUP(ka->kernel_seg0_addr.start + ka->kernel_seg0_addr.size, PAGE_SIZE);

	// map in a kernel stack
	ka->cpu_kstack[0].start = next_vaddr;
	for(i=0; i<STACK_SIZE; i++) {
		mmu_map_page(next_vaddr, next_paddr);
		next_vaddr += PAGE_SIZE;
		next_paddr += PAGE_SIZE;
	}
	ka->cpu_kstack[0].size = next_vaddr - ka->cpu_kstack[0].start;

//	dprintf("new stack at 0x%x to 0x%x\n", ka->cpu_kstack[0].start, ka->cpu_kstack[0].start + ka->cpu_kstack[0].size);

	// set up a new idt
	{
		struct gdt_idt_descr idt_descr;

		// find a new idt
		idt = (unsigned int *)next_paddr;
		ka->arch_args.phys_idt = (unsigned int)idt;
		next_paddr += PAGE_SIZE;

//		nmessage("idt at ", (unsigned int)idt, "\n");

		// clear it out
		for(i=0; i<IDT_LIMIT/4; i++) {
			idt[i] = 0;
		}

		// map the idt into virtual space
		mmu_map_page(next_vaddr, (unsigned int)idt);
		ka->arch_args.vir_idt = (unsigned int)next_vaddr;
		next_vaddr += PAGE_SIZE;

		// load the idt
		idt_descr.a = IDT_LIMIT - 1;
		idt_descr.b = (unsigned int *)ka->arch_args.vir_idt;

		asm("lidt	%0;"
			: : "m" (idt_descr));

//		nmessage("idt at virtual address ", next_vpage, "\n");
	}

	// set up a new gdt
	{
		struct gdt_idt_descr gdt_descr;

		// find a new gdt
		gdt = (unsigned int *)next_paddr;
		ka->arch_args.phys_gdt = (unsigned int)gdt;
		next_paddr += PAGE_SIZE;

//		nmessage("gdt at ", (unsigned int)gdt, "\n");

		// put segment descriptors in it
		gdt[0] = 0;
		gdt[1] = 0;
		gdt[2] = 0x0000ffff; // seg 0x8  -- kernel 4GB code
		gdt[3] = 0x00cf9a00;
		gdt[4] = 0x0000ffff; // seg 0x10 -- kernel 4GB data
		gdt[5] = 0x00cf9200;
		gdt[6] = 0x0000ffff; // seg 0x1b -- ring 3 4GB code
		gdt[7] = 0x00cffa00;
		gdt[8] = 0x0000ffff; // seg 0x23 -- ring 3 4GB data
		gdt[9] = 0x00cff200;
		// gdt[10] & gdt[11] will be filled later by the kernel

		// map the gdt into virtual space
		mmu_map_page(next_vaddr, (unsigned int)gdt);
		ka->arch_args.vir_gdt = (unsigned int)next_vaddr;
		next_vaddr += PAGE_SIZE;

		// load the GDT
		gdt_descr.a = GDT_LIMIT - 1;
		gdt_descr.b = (unsigned int *)ka->arch_args.vir_gdt;

		asm("lgdt	%0;"
			: : "m" (gdt_descr));

//		nmessage("gdt at virtual address ", next_vpage, "\n");
	}

	// Map the pg_dir into kernel space at 0xffc00000-0xffffffff
	// this enables a mmu trick where the 4 MB region that this pgdir entry
	// represents now maps the 4MB of potential pagetables that the pgdir
	// points to. Thrown away later in VM bringup, but useful for now.
	pgdir[1023] = (unsigned int)pgdir | DEFAULT_PAGE_FLAGS;

	// also map it on the next vpage
	mmu_map_page(next_vaddr, (unsigned int)pgdir);
	ka->arch_args.vir_pgdir = next_vaddr;
	next_vaddr += PAGE_SIZE;

	// save the kernel args
	ka->arch_args.system_time_cv_factor = cv_factor;
	ka->phys_mem_range[0].start = 0;
	ka->phys_mem_range[0].size = mem;
	ka->num_phys_mem_ranges = 1;
	ka->str = NULL;
	ka->phys_alloc_range[0].start = BOOTDIR_ADDR;
	ka->phys_alloc_range[0].size = next_paddr - BOOTDIR_ADDR;
	ka->num_phys_alloc_ranges = 1;
	ka->virt_alloc_range[0].start = KERNEL_BASE;
	ka->virt_alloc_range[0].size = next_vaddr - KERNEL_BASE;
	ka->num_virt_alloc_ranges = 1;
	ka->arch_args.page_hole = 0xffc00000;
	ka->num_cpus = 1;
#if 0
	dprintf("kernel args at 0x%x\n", ka);
	dprintf("pgdir = 0x%x\n", ka->pgdir);
	dprintf("pgtables[0] = 0x%x\n", ka->pgtables[0]);
	dprintf("phys_idt = 0x%x\n", ka->phys_idt);
	dprintf("vir_idt = 0x%x\n", ka->vir_idt);
	dprintf("phys_gdt = 0x%x\n", ka->phys_gdt);
	dprintf("vir_gdt = 0x%x\n", ka->vir_gdt);
	dprintf("mem_size = 0x%x\n", ka->mem_size);
	dprintf("str = 0x%x\n", ka->str);
	dprintf("bootdir = 0x%x\n", ka->bootdir);
	dprintf("bootdir_size = 0x%x\n", ka->bootdir_size);
	dprintf("phys_alloc_range_low = 0x%x\n", ka->phys_alloc_range_low);
	dprintf("phys_alloc_range_high = 0x%x\n", ka->phys_alloc_range_high);
	dprintf("virt_alloc_range_low = 0x%x\n", ka->virt_alloc_range_low);
	dprintf("virt_alloc_range_high = 0x%x\n", ka->virt_alloc_range_high);
	dprintf("page_hole = 0x%x\n", ka->page_hole);
#endif
//	dprintf("finding and booting other cpus...\n");
	smp_boot(ka, kernel_entry);

	dprintf("jumping into kernel at 0x%x\n", kernel_entry);

	ka->cons_line = line;

	asm("movl	%0, %%eax;	"			// move stack out of way
		"movl	%%eax, %%esp; "
		: : "m" (ka->cpu_kstack[0].start + ka->cpu_kstack[0].size));
	asm("pushl  $0x0; "					// we're the BSP cpu (0)
		"pushl 	%0;	"					// kernel args
		"pushl 	$0x0;"					// dummy retval for call to main
		"pushl 	%1;	"					// this is the start address
		"ret;		"					// jump.
		: : "g" (ka), "g" (kernel_entry));
}

static void load_elf_image(void *data, unsigned int *next_paddr, addr_range *ar0, addr_range *ar1, unsigned int *start_addr, addr_range *dynamic_section)
{
	struct Elf32_Ehdr *imageHeader = (struct Elf32_Ehdr*) data;
	struct Elf32_Phdr *segments = (struct Elf32_Phdr*)(imageHeader->e_phoff + (unsigned) imageHeader);
	int segmentIndex;
	int foundSegmentIndex = 0;

	ar0->size = 0;
	ar1->size = 0;
	dynamic_section->size = 0;

	for (segmentIndex = 0; segmentIndex < imageHeader->e_phnum; segmentIndex++) {
		struct Elf32_Phdr *segment = &segments[segmentIndex];
		unsigned segmentOffset;

		switch(segment->p_type) {
			case PT_LOAD:
				break;
			case PT_DYNAMIC:
				dynamic_section->start = segment->p_vaddr;
				dynamic_section->size = segment->p_memsz;
			default:
				continue;
		}

//		dprintf("segment %d\n", segmentIndex);
//		dprintf("p_vaddr 0x%x p_paddr 0x%x p_filesz 0x%x p_memsz 0x%x\n",
//			segment->p_vaddr, segment->p_paddr, segment->p_filesz, segment->p_memsz);

		/* Map initialized portion */
		for (segmentOffset = 0;
			segmentOffset < ROUNDUP(segment->p_filesz, PAGE_SIZE);
			segmentOffset += PAGE_SIZE) {

			mmu_map_page(segment->p_vaddr + segmentOffset, *next_paddr);
			memcpy((void *)ROUNDOWN(segment->p_vaddr + segmentOffset, PAGE_SIZE),
				(void *)ROUNDOWN((unsigned)data + segment->p_offset + segmentOffset, PAGE_SIZE), PAGE_SIZE);
			(*next_paddr) += PAGE_SIZE;
		}

		/* Clean out the leftover part of the last page */
		if(segment->p_filesz % PAGE_SIZE > 0) {
//			dprintf("memsetting 0 to va 0x%x, size %d\n", (void*)((unsigned)segment->p_vaddr + segment->p_filesz), PAGE_SIZE  - (segment->p_filesz % PAGE_SIZE));
			memset((void*)((unsigned)segment->p_vaddr + segment->p_filesz), 0, PAGE_SIZE
				- (segment->p_filesz % PAGE_SIZE));
		}

		/* Map uninitialized portion */
		for (; segmentOffset < ROUNDUP(segment->p_memsz, PAGE_SIZE); segmentOffset += PAGE_SIZE) {
//			dprintf("mapping zero page at va 0x%x\n", segment->p_vaddr + segmentOffset);
			mmu_map_page(segment->p_vaddr + segmentOffset, *next_paddr);
			memset((void *)(segment->p_vaddr + segmentOffset), 0, PAGE_SIZE);
			(*next_paddr) += PAGE_SIZE;
		}
		switch(foundSegmentIndex) {
			case 0:
				ar0->start = segment->p_vaddr;
				ar0->size = segment->p_memsz;
				break;
			case 1:
				ar1->start = segment->p_vaddr;
				ar1->size = segment->p_memsz;
				break;
			default:
				;
		}
		foundSegmentIndex++;
	}
	*start_addr = imageHeader->e_entry;
}

// allocate a page directory and page table to facilitate mapping
// pages to the 0x80000000 - 0x80400000 region.
// also identity maps the first 4MB of memory
static int mmu_init(kernel_args *ka, unsigned int *next_paddr)
{
	int i;

	// allocate a new pgdir
	pgdir = (unsigned int *)*next_paddr;
	(*next_paddr) += PAGE_SIZE;
	ka->arch_args.phys_pgdir = (unsigned int)pgdir;

	// clear out the pgdir
	for(i = 0; i < 1024; i++)
		pgdir[i] = 0;

	// make a pagetable at this random spot
	pgtable = (unsigned int *)0x11000;

	for (i = 0; i < 1024; i++) {
		pgtable[i] = (i * 0x1000) | DEFAULT_PAGE_FLAGS;
	}	// pkx: create first 4 MB one-to-one mapping

	pgdir[0] = (unsigned int)pgtable | DEFAULT_PAGE_FLAGS;
		// pkx: put the one-to-one mapping into the page dir.

	// Get new page table and clear it out
	pgtable = (unsigned int *)*next_paddr;
	ka->arch_args.pgtables[0] = (unsigned int)pgtable;
	ka->arch_args.num_pgtables = 1;

	(*next_paddr) += PAGE_SIZE;
	for (i = 0; i < 1024; i++)
		pgtable[i] = 0;

	// put the new page table into the page directory
	// this maps the kernel at KERNEL_BASE
	pgdir[KERNEL_BASE/(4*1024*1024)] = (unsigned int)pgtable | DEFAULT_PAGE_FLAGS;

	// switch to the new pgdir
	asm("movl %0, %%eax;"
		"movl %%eax, %%cr3;" :: "m" (pgdir) : "eax");
	// Important.  Make sure supervisor threads can fault on read only pages...
	asm("movl %%eax, %%cr0" : : "a" ((1 << 31) | (1 << 16) | (1 << 5) | 1));
		// pkx: moved the paging turn-on to here.

	return 0;
}

// can only map the 4 meg region right after KERNEL_BASE, may fix this later
// if need arises.
static void mmu_map_page(unsigned int vaddr, unsigned int paddr)
{
//	dprintf("mmu_map_page: vaddr 0x%x, paddr 0x%x\n", vaddr, paddr);
	if(vaddr < KERNEL_BASE || vaddr >= (KERNEL_BASE + 4096*1024)) {
		dprintf("mmu_map_page: asked to map invalid page!\n");
		for(;;);
	}
	paddr &= ~(PAGE_SIZE-1);
//	dprintf("paddr 0x%x @ index %d\n", paddr, (vaddr % (PAGE_SIZE * 1024)) / PAGE_SIZE);
	pgtable[(vaddr % (PAGE_SIZE * 1024)) / PAGE_SIZE] = paddr | DEFAULT_PAGE_FLAGS;
}

static int check_cpu(void)
{
//	unsigned int i;
	unsigned int data[4];
	char str[17];

	// check the eflags register to see if the cpuid instruction exists
	if((get_eflags() & 1<<21) == 0) {
		set_eflags(get_eflags() | 1<<21);
		if((get_eflags() & 1<<21) == 0) {
			// we couldn't set the ID bit of the eflags register, this cpu is old
			return -1;
		}
	}

	// we can safely call cpuid

	// print some fun data
	cpuid(0, data);

	// build the vendor string
	memset(str, 0, sizeof(str));
	*(unsigned int *)&str[0] = data[1];
	*(unsigned int *)&str[4] = data[3];
	*(unsigned int *)&str[8] = data[2];

	// get the family, model, stepping
	cpuid(1, data);
	dprintf("CPU: family %d model %d stepping %d, string '%s'\n",
		(data[0] >> 8) & 0xf, (data[0] >> 4) & 0xf, data[0] & 0xf, str);

	// check for bits we need
	cpuid(1, data);
	if(!(data[3] & 1<<4)) return -1; // check for rdtsc

	return 0;
}

void sleep(long long time)
{
	long long start = system_time();

	while(system_time() - start <= time)
		;
}

#define outb(value,port) \
	asm("outb %%al,%%dx"::"a" (value),"d" (port))


#define inb(port) ({ \
	unsigned char _v; \
	asm volatile("inb %%dx,%%al":"=a" (_v):"d" (port)); \
	_v; \
	})

#define TIMER_CLKNUM_HZ 1193167

static void calculate_cpu_conversion_factor(void)
{
	unsigned char	low, high;
	unsigned long	expired;
	long long		t1, t2;
	long long       time_base_ticks;
	double			timer_usecs;

	/* program the timer to count down mode */
	outb(0x34, 0x43);

	outb(0xff, 0x40);		/* low and then high */
	outb(0xff, 0x40);

	t1 = rdtsc();

	execute_n_instructions(32*20000);

	t2 = rdtsc();

	outb(0x00, 0x43); /* latch counter value */
	low = inb(0x40);
	high = inb(0x40);

	expired = (unsigned long)0xffff - ((((unsigned long)high) << 8) + low);

	timer_usecs = (expired * 1.0) / (TIMER_CLKNUM_HZ/1000000.0);
	time_base_ticks = t2 -t1;

	dprintf("CPU at %d Hz\n", (int)((time_base_ticks / timer_usecs) * 1000000));

	system_time_setup((int)((time_base_ticks / timer_usecs) * 1000000));
}

void clearscreen()
{
	int i;

	for(i=0; i< SCREEN_WIDTH*SCREEN_HEIGHT*2; i++) {
		kScreenBase[i] = 0xf20;
	}
}

static void scrup()
{
	int i;
	memcpy(kScreenBase, kScreenBase + SCREEN_WIDTH,
		SCREEN_WIDTH * SCREEN_HEIGHT * 2 - SCREEN_WIDTH * 2);
	screenOffset = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH;
	for(i=0; i<SCREEN_WIDTH; i++)
		kScreenBase[screenOffset + i] = 0x0720;
	line = SCREEN_HEIGHT - 1;
}

void kputs(const char *str)
{
	while (*str) {
		if (*str == '\n') {
			line++;
			if(line > SCREEN_HEIGHT - 1)
				scrup();
			else
				screenOffset += SCREEN_WIDTH - (screenOffset % 80);
		} else {
			kScreenBase[screenOffset++] = 0xf00 | *str;
		}
		if (screenOffset > SCREEN_WIDTH * SCREEN_HEIGHT)
			scrup();

		str++;
	}
}

int dprintf(const char *fmt, ...)
{
	int ret;
	va_list args;
	char temp[256];

	va_start(args, fmt);
	ret = vsprintf(temp,fmt,args);
	va_end(args);

	kputs(temp);
	return ret;
}

