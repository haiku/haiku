/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _STAGE2_PRIV_H
#define _STAGE2_PRIV_H

#include <stage2.h>

extern void _start(unsigned int mem, int in_vesa, unsigned int vesa_ptr);
extern void clearscreen(void);
extern void kputs(const char *str);
extern int dprintf(const char *fmt, ...);
extern void sleep(long long time);
extern long long system_time(void);
extern void execute_n_instructions(int count);
void system_time_setup(long a);
long long rdtsc();
unsigned int get_eflags(void);
void set_eflags(unsigned int val);
void cpuid(unsigned int selector, unsigned int *data);

//void put_uint_dec(unsigned int a);
//void put_uint_hex(unsigned int a);
//void nmessage(const char *str1, unsigned int a, const char *str2);
//void nmessage2(const char *str1, unsigned int a, const char *str2, unsigned int b, const char *str3);

#define ROUNDUP(a, b) (((a) + ((b) - 1)) & (~((b) - 1)))
#define ROUNDOWN(a, b) (((a) / (b)) * (b))

#define PAGE_SIZE 0x1000
#define KERNEL_BASE 0x80000000
#define KERNEL_ENTRY 0x80000080
#define STACK_SIZE 2
#define DEFAULT_PAGE_FLAGS (1 | 2) // present/rw
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 24
#define ADDR_MASK 0xfffff000

#define NULL ((void *)0)

#define _PACKED __attribute__((packed))

#define IDT_LIMIT 0x800
#define GDT_LIMIT 0x800

struct gdt_idt_descr {
	unsigned short a;
	unsigned int *b;
} _PACKED;

// SMP stuff
extern int smp_boot(kernel_args *ka, unsigned int kernel_entry);
extern void smp_trampoline(void);
extern void smp_trampoline_end(void);

#define MP_FLT_SIGNATURE '_PM_'
#define MP_CTH_SIGNATURE 'PCMP'

#define APIC_DM_INIT       (5 << 8)
#define APIC_DM_STARTUP    (6 << 8)
#define APIC_LEVEL_TRIG    (1 << 14)
#define APIC_ASSERT        (1 << 13)

#define APIC_ENABLE        0x100
#define APIC_FOCUS         (~(1 << 9))
#define APIC_SIV           (0xff)

#define APIC_ID            ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x020))
#define APIC_VERSION       ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x030))
#define APIC_TPRI          ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x080))
#define APIC_EOI           ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x0b0))
#define APIC_LDR           ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x0d0))
#define APIC_SIVR          ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x0f0))
#define APIC_ESR           ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x280))
#define APIC_ICR1          ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x300))
#define APIC_ICR2          ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x310))
#define APIC_LVTT          ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x320))
#define APIC_LINT0         ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x350))
#define APIC_LINT1         ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x360))
#define APIC_LVT3          ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x370))
#define APIC_ICRT          ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x380))
#define APIC_CCRT          ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x390))
#define APIC_TDCR          ((unsigned int *) ((unsigned int) ka->arch_args.apic + 0x3e0))

#define APIC_TDCR_2        0x00
#define APIC_TDCR_4        0x01
#define APIC_TDCR_8        0x02
#define APIC_TDCR_16       0x03
#define APIC_TDCR_32       0x08
#define APIC_TDCR_64       0x09
#define APIC_TDCR_128      0x0a
#define APIC_TDCR_1        0x0b

#define APIC_LVTT_MASK     0x000310ff
#define APIC_LVTT_VECTOR   0x000000ff
#define APIC_LVTT_DS       0x00001000
#define APIC_LVTT_M        0x00010000
#define APIC_LVTT_TM       0x00020000

#define APIC_LVT_DM_ExtINT 0x00000700
#define APIC_LVT_DM_NMI    0x00000400
#define APIC_LVT_IIPP      0x00002000
#define APIC_LVT_TM        0x00008000
#define APIC_LVT_M         0x00010000
#define APIC_LVT_OS        0x00020000
#define APIC_TPR_PRIO      0x000000ff
#define APIC_TPR_INT       0x000000f0
#define APIC_TPR_SUB       0x0000000f

#define APIC_SVR_SWEN      0x00000100
#define APIC_SVR_FOCUS     0x00000200

#define APIC_DEST_STARTUP  0x00600

#define LOPRIO_LEVEL       0x00000010

#define APIC_DEST_FIELD          (0)
#define APIC_DEST_SELF           (1 << 18)
#define APIC_DEST_ALL            (2 << 18)
#define APIC_DEST_ALL_BUT_SELF   (3 << 18)

#define IOAPIC_ID          0x0
#define IOAPIC_VERSION     0x1
#define IOAPIC_ARB         0x2
#define IOAPIC_REDIR_TABLE 0x10

#define IPI_CACHE_FLUSH    0x40
#define IPI_INV_TLB        0x41
#define IPI_INV_PTE        0x42
#define IPI_INV_RESCHED    0x43
#define IPI_STOP           0x44

#define MP_EXT_PE          0
#define MP_EXT_BUS         1
#define MP_EXT_IO_APIC     2
#define MP_EXT_IO_INT      3
#define MP_EXT_LOCAL_INT   4

#define MP_EXT_PE_LEN          20
#define MP_EXT_BUS_LEN         8
#define MP_EXT_IO_APIC_LEN     8
#define MP_EXT_IO_INT_LEN      8
#define MP_EXT_LOCAL_INT_LEN   8

struct mp_config_table {
	unsigned int signature;       /* "PCMP" */
	unsigned short table_len;     /* length of this structure */
	unsigned char mp_rev;         /* spec supported, 1 for 1.1 or 4 for 1.4 */
	unsigned char checksum;       /* checksum, all bytes add up to zero */
	char oem[8];                  /* oem identification, not null-terminated */
	char product[12];             /* product name, not null-terminated */
	void *oem_table_ptr;          /* addr of oem-defined table, zero if none */
	unsigned short oem_len;       /* length of oem table */
	unsigned short num_entries;   /* number of entries in base table */
	unsigned int apic;            /* address of apic */
	unsigned short ext_len;       /* length of extended section */
	unsigned char ext_checksum;   /* checksum of extended table entries */
};

struct mp_flt_struct {
	unsigned int signature;       /* "_MP_" */
	struct mp_config_table *mpc;  /* address of mp configuration table */
	unsigned char mpc_len;        /* length of this structure in 16-byte units */
	unsigned char mp_rev;         /* spec supported, 1 for 1.1 or 4 for 1.4 */
	unsigned char checksum;       /* checksum, all bytes add up to zero */
	unsigned char mp_feature_1;   /* mp system configuration type if no mpc */
	unsigned char mp_feature_2;   /* imcrp */
	unsigned char mp_feature_3, mp_feature_4, mp_feature_5; /* reserved */
};

struct mp_ext_pe
{
	unsigned char type;
	unsigned char apic_id;
	unsigned char apic_version;
	unsigned char cpu_flags;
	unsigned int signature;       /* stepping, model, family, each four bits */
	unsigned int feature_flags;
	unsigned int res1, res2;
};

struct mp_ext_ioapic
{
	unsigned char type;
	unsigned char ioapic_id;
	unsigned char ioapic_version;
	unsigned char ioapic_flags;
	unsigned int *addr;
};

struct mp_ext_bus
{
	unsigned char type;
	unsigned char bus_id;
	char          name[6];
};

#endif

