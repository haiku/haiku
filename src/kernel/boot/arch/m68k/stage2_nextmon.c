/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include "stage2_priv.h"

#define MG_simm 0
#define MG_flags 4
#define MG_sid 6
#define MG_pagesize 10
#define MG_mon_stack 14
#define MG_vbr 18
#define MG_nvram 22
#define MG_inetntoa 54
#define MG_inputline 72
#define MG_region 200
#define MG_alloc_base 232
#define MG_alloc_brk 236
#define MG_boot_dev 240
#define MG_boot_arg 244
#define MG_boot_info 248
#define MG_boot_file 252
#define MG_bootfile 256
#define MG_boot_how 320
#define MG_km 324
#define MG_km_flags 368
#define MG_mon_init 370
#define MG_si 374
#define MG_time 378
#define MG_sddp 382
#define MG_dgp 386
#define MG_s5cp 390
#define MG_odc 394
#define MG_odd 398
#define MG_radix 402
#define MG_dmachip 404
#define MG_diskchip 408
#define MG_intrstat 412
#define MG_intrmask 416
#define MG_nofault 420
#define MG_fmt 424
#define MG_addr 426
#define MG_na 458
#define MG_mx 462
#define MG_my 466
#define MG_cursor_save 470
#define MG_getc 726
#define MG_try_getc 730
#define MG_putc 734
#define MG_alert 738
#define MG_alert_confirm 742
#define MG_alloc 746
#define MG_boot_slider 750
#define MG_eventc 754
#define MG_event_high 758
#define MG_animate 762
#define MG_anim_time 766
#define MG_scsi_intr 770
#define MG_scsi_intrarg 774
#define MG_minor 778
#define MG_seq 780
#define MG_anim_run 782
#define MG_major 786
#define MG_con_slot 844
#define MG_con_fbnum 845
#define MG_con_map_vaddr0 860
#define MG_con_map_vaddr1 872
#define MG_con_map_vaddr2 884
#define MG_con_map_vaddr3 896
#define MG_con_map_vaddr4 908
#define MG_con_map_vaddr5 920

#define MG_clientetheraddr 788
#define MG_machine_type    936
#define MG_board_rev       937

#define N_SIMM          4               /* number of SIMMs in machine */

/* SIMM types */
#define SIMM_SIZE       0x03
#define SIMM_SIZE_EMPTY 0x00
#define SIMM_SIZE_16MB  0x01
#define SIMM_SIZE_4MB   0x02
#define SIMM_SIZE_1MB   0x03
#define SIMM_PAGE_MODE  0x04
#define SIMM_PARITY     0x08 /* ?? */

#define NEXT_RAMBASE 0x4000000

/* Machine types */
#define NeXT_CUBE       0
#define NeXT_WARP9      1
#define NeXT_X15        2
#define NeXT_WARP9C     3
#define NeXT_TURBO 4
#define NeXT_TURBO_COLOR 5

typedef int (*getcptr)(void);
typedef int (*putcptr)(int);

#define MON(type, off) (*(type *)((unsigned int) (mg) + off))

static char *mg = 0;

int init_nextmon(char *monitor)
{
	mg = monitor;

	return 0;
}

int probe_memory(kernel_args *ka)
{
	int i, r;
	char machine_type = MON(char,MG_machine_type);
	int msize1, msize4, msize16;

	/* depending on the machine, the bank layout is different */
	dprintf("machine type: 0x%x\n", machine_type);
	switch(machine_type) {
		case NeXT_WARP9:
		case NeXT_X15:
			msize16 = 0x10000000;
			msize4 = 0x400000;
			msize1 = 0x100000;
			break;
		case NeXT_WARP9C:
			msize16 = 0x800000;
			msize4 = 0x200000;
			msize1 = 0x80000;
			break;
		case NeXT_TURBO:
		case NeXT_TURBO_COLOR:
			msize16 = 0x2000000;
			msize4 = 0x800000;
			msize1 = 0x200000;
			break;
		default:
			msize16 = 0x100000;
			msize4 = 0x100000;
			msize1 = 0x100000;
	}

	/* start probing ram */
	dprintf("memory probe:\n");
	ka->num_phys_mem_ranges = 0;
	for(i=0; i<N_SIMM; i++) {
		char probe = MON(char,MG_simm+i);

		if((probe & SIMM_SIZE) != SIMM_SIZE_EMPTY) {
			ka->phys_mem_range[ka->num_phys_mem_ranges].start = NEXT_RAMBASE + (msize16 * i);
		}
		switch(probe & SIMM_SIZE) {
			case SIMM_SIZE_16MB:
				ka->phys_mem_range[ka->num_phys_mem_ranges].size = msize16;
				break;
			case SIMM_SIZE_4MB:
				ka->phys_mem_range[ka->num_phys_mem_ranges].size = msize4;
				break;
			case SIMM_SIZE_1MB:
				ka->phys_mem_range[ka->num_phys_mem_ranges].size = msize1;
				break;
		}
		dprintf("bank %i: start 0x%x, size 0x%x\n", i,
			ka->phys_mem_range[ka->num_phys_mem_ranges].start, ka->phys_mem_range[ka->num_phys_mem_ranges].size);
		ka->num_phys_mem_ranges++;
	}

	dprintf("alloc_base: 0x%x\n", MON(int, MG_alloc_base));
	dprintf("alloc_brk: 0x%x\n", MON(int, MG_alloc_brk));
	dprintf("mon_stack: 0x%x\n", MON(int, MG_mon_stack));

	return 0;
}

void putc(int c)
{
	MON(putcptr,MG_putc)(c);
}

int getc(void)
{
	return(MON(getcptr,MG_getc)());
}
