#ifndef FREQ_TABLE_H
#define FREQ_TABLE_H

#include <OS.h>
#include <KernelExport.h>
#include <Errors.h>

#include "device/power_managment.h"

#define TRACE_EST
#ifdef TRACE_EST
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif

// this code is taken from freeBSD

#define MSR_GET_FREQ_STATE		0x198 
#define MSR_SET_FREQ_STATE   	0x199

/* Estimate in microseconds of latency for performing a transition. */
#define EST_TRANS_LAT           1000

/* Identifying characteristics of a processor and supported frequencies. */
typedef struct {
	const uint32	vendor_id;
	uint32			id32;
	freq_info		*freqtab;
} ss_cpu_info;


status_t est_get_info(freq_info **freqsInfo);
status_t est_table_info(uint64 msr, freq_info **freqs);
status_t est_msr_info(uint64 msr, freq_info **freqs);

freq_info* est_get_current(freq_info *freq_list);
void est_get_id16(uint16 *id16_p);
status_t est_set_id16(uint16 id16, bool need_check = false);

/* Convert MHz and mV into IDs for passing to the MSR. */
#define ID16(MHz, mV, bus_clk)                          \
        (((MHz / bus_clk) << 8) | ((mV ? mV - 700 : 0) >> 4))
#define ID32(MHz_hi, mV_hi, MHz_lo, mV_lo, bus_clk)     \
        ((ID16(MHz_lo, mV_lo, bus_clk) << 16) | (ID16(MHz_hi, mV_hi, bus_clk)))

/* Format for storing IDs in our table. */
#define CPUFREQ_VAL_UNKNOWN (-1)
#define CPU_VENDOR_INTEL	B_CPU_INTEL_x86
#define CPU_VENDOR_CENTAUR	B_CPU_VIA_IDT_x86

#define FREQ_INFO_PWR(MHz, mV, bus_clk, mW)             \
        { MHz, mV, ID16(MHz, mV, bus_clk), mW }
#define FREQ_INFO(MHz, mV, bus_clk)                     \
        FREQ_INFO_PWR(MHz, mV, bus_clk, CPUFREQ_VAL_UNKNOWN)
#define INTEL(tab, zhi, vhi, zlo, vlo, bus_clk)         \
        { CPU_VENDOR_INTEL, ID32(zhi, vhi, zlo, vlo, bus_clk), tab }
#define CENTAUR(tab, zhi, vhi, zlo, vlo, bus_clk)       \
        { CPU_VENDOR_CENTAUR, ID32(zhi, vhi, zlo, vlo, bus_clk), tab }

/* Default bus clock value for Centrino processors. */
#define INTEL_BUS_CLK           100

/*
 * Frequency (MHz) and voltage (mV) settings.  Data from the
 * Intel Pentium M Processor Datasheet (Order Number 252612), Table 5.
 *
 * Dothan processors have multiple VID#s with different settings for
 * each VID#.  Since we can't uniquely identify this info
 * without undisclosed methods from Intel, we can't support newer
 * processors with this table method.  If ACPI Px states are supported,
 * we get info from them.
 */
static freq_info PM17_130[] = {
        /* 130nm 1.70GHz Pentium M */
        FREQ_INFO(1700, 1484, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1308, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1228, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1004, INTEL_BUS_CLK),
        FREQ_INFO( 600,  956, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM16_130[] = {
        /* 130nm 1.60GHz Pentium M */
        FREQ_INFO(1600, 1484, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1420, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1276, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1164, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
        FREQ_INFO( 600,  956, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM15_130[] = {
        /* 130nm 1.50GHz Pentium M */
        FREQ_INFO(1500, 1484, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1452, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1356, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1228, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1116, INTEL_BUS_CLK),
        FREQ_INFO( 600,  956, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM14_130[] = {
        /* 130nm 1.40GHz Pentium M */
        FREQ_INFO(1400, 1484, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1436, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1308, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1180, INTEL_BUS_CLK),
        FREQ_INFO( 600,  956, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM13_130[] = {
        /* 130nm 1.30GHz Pentium M */
        FREQ_INFO(1300, 1388, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1356, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1292, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1260, INTEL_BUS_CLK),
        FREQ_INFO( 600,  956, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM13_LV_130[] = {
        /* 130nm 1.30GHz Low Voltage Pentium M */
        FREQ_INFO(1300, 1180, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1164, INTEL_BUS_CLK),
        FREQ_INFO(1100, 1100, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1020, INTEL_BUS_CLK),
        FREQ_INFO( 900, 1004, INTEL_BUS_CLK),
        FREQ_INFO( 800,  988, INTEL_BUS_CLK),
        FREQ_INFO( 600,  956, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM12_LV_130[] = {
        /* 130 nm 1.20GHz Low Voltage Pentium M */
        FREQ_INFO(1200, 1180, INTEL_BUS_CLK),
        FREQ_INFO(1100, 1164, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1100, INTEL_BUS_CLK),
        FREQ_INFO( 900, 1020, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1004, INTEL_BUS_CLK),
        FREQ_INFO( 600,  956, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM11_LV_130[] = {
        /* 130 nm 1.10GHz Low Voltage Pentium M */
        FREQ_INFO(1100, 1180, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1164, INTEL_BUS_CLK),
        FREQ_INFO( 900, 1100, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1020, INTEL_BUS_CLK),
        FREQ_INFO( 600,  956, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM11_ULV_130[] = {
        /* 130 nm 1.10GHz Ultra Low Voltage Pentium M */
        FREQ_INFO(1100, 1004, INTEL_BUS_CLK),
        FREQ_INFO(1000,  988, INTEL_BUS_CLK),
        FREQ_INFO( 900,  972, INTEL_BUS_CLK),
        FREQ_INFO( 800,  956, INTEL_BUS_CLK),
        FREQ_INFO( 600,  844, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM10_ULV_130[] = {
        /* 130 nm 1.00GHz Ultra Low Voltage Pentium M */
        FREQ_INFO(1000, 1004, INTEL_BUS_CLK),
        FREQ_INFO( 900,  988, INTEL_BUS_CLK),
        FREQ_INFO( 800,  972, INTEL_BUS_CLK),
        FREQ_INFO( 600,  844, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};

/*
 * Data from "Intel Pentium M Processor on 90nm Process with
 * 2-MB L2 Cache Datasheet", Order Number 302189, Table 5.
 */
static freq_info PM_765A_90[] = {
        /* 90 nm 2.10GHz Pentium M, VID #A */
        FREQ_INFO(2100, 1340, INTEL_BUS_CLK),
        FREQ_INFO(1800, 1276, INTEL_BUS_CLK),
        FREQ_INFO(1600, 1228, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1180, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1132, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_765B_90[] = {
        /* 90 nm 2.10GHz Pentium M, VID #B */
        FREQ_INFO(2100, 1324, INTEL_BUS_CLK),
        FREQ_INFO(1800, 1260, INTEL_BUS_CLK),
        FREQ_INFO(1600, 1212, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1180, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1132, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_765C_90[] = {
        /* 90 nm 2.10GHz Pentium M, VID #C */
        FREQ_INFO(2100, 1308, INTEL_BUS_CLK),
        FREQ_INFO(1800, 1244, INTEL_BUS_CLK),
        FREQ_INFO(1600, 1212, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1164, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1116, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_765E_90[] = {
        /* 90 nm 2.10GHz Pentium M, VID #E */
        FREQ_INFO(2100, 1356, INTEL_BUS_CLK),
        FREQ_INFO(1800, 1292, INTEL_BUS_CLK),
        FREQ_INFO(1600, 1244, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1196, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1148, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1100, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_755A_90[] = {
        /* 90 nm 2.00GHz Pentium M, VID #A */
        FREQ_INFO(2000, 1340, INTEL_BUS_CLK),
        FREQ_INFO(1800, 1292, INTEL_BUS_CLK),
        FREQ_INFO(1600, 1244, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1196, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1148, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1100, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_755B_90[] = {
        /* 90 nm 2.00GHz Pentium M, VID #B */
        FREQ_INFO(2000, 1324, INTEL_BUS_CLK),
        FREQ_INFO(1800, 1276, INTEL_BUS_CLK),
        FREQ_INFO(1600, 1228, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1180, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1132, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_755C_90[] = {
        /* 90 nm 2.00GHz Pentium M, VID #C */
        FREQ_INFO(2000, 1308, INTEL_BUS_CLK),
        FREQ_INFO(1800, 1276, INTEL_BUS_CLK),
        FREQ_INFO(1600, 1228, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1180, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1132, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_755D_90[] = {
        /* 90 nm 2.00GHz Pentium M, VID #D */
        FREQ_INFO(2000, 1276, INTEL_BUS_CLK),
        FREQ_INFO(1800, 1244, INTEL_BUS_CLK),
        FREQ_INFO(1600, 1196, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1164, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1116, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_745A_90[] = {
        /* 90 nm 1.80GHz Pentium M, VID #A */
        FREQ_INFO(1800, 1340, INTEL_BUS_CLK),
        FREQ_INFO(1600, 1292, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1228, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1164, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_745B_90[] = {
        /* 90 nm 1.80GHz Pentium M, VID #B */
        FREQ_INFO(1800, 1324, INTEL_BUS_CLK),
        FREQ_INFO(1600, 1276, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1212, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1164, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_745C_90[] = {
        /* 90 nm 1.80GHz Pentium M, VID #C */
        FREQ_INFO(1800, 1308, INTEL_BUS_CLK),
        FREQ_INFO(1600, 1260, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1212, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1148, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1100, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_745D_90[] = {
        /* 90 nm 1.80GHz Pentium M, VID #D */
        FREQ_INFO(1800, 1276, INTEL_BUS_CLK),
        FREQ_INFO(1600, 1228, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1180, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1132, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1084, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1036, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_735A_90[] = {
        /* 90 nm 1.70GHz Pentium M, VID #A */
        FREQ_INFO(1700, 1340, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1244, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1180, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_735B_90[] = {
        /* 90 nm 1.70GHz Pentium M, VID #B */
        FREQ_INFO(1700, 1324, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1244, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1180, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_735C_90[] = {
        /* 90 nm 1.70GHz Pentium M, VID #C */
        FREQ_INFO(1700, 1308, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1228, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1164, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_735D_90[] = {
        /* 90 nm 1.70GHz Pentium M, VID #D */
        FREQ_INFO(1700, 1276, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1212, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1148, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1100, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_725A_90[] = {
        /* 90 nm 1.60GHz Pentium M, VID #A */
        FREQ_INFO(1600, 1340, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1276, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1212, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1132, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1068, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_725B_90[] = {
        /* 90 nm 1.60GHz Pentium M, VID #B */
        FREQ_INFO(1600, 1324, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1260, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1196, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1132, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1068, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_725C_90[] = {
        /* 90 nm 1.60GHz Pentium M, VID #C */
        FREQ_INFO(1600, 1308, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1244, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1180, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_725D_90[] = {
        /* 90 nm 1.60GHz Pentium M, VID #D */
        FREQ_INFO(1600, 1276, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1228, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1164, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_715A_90[] = {
        /* 90 nm 1.50GHz Pentium M, VID #A */
        FREQ_INFO(1500, 1340, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1228, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1148, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1068, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_715B_90[] = {
        /* 90 nm 1.50GHz Pentium M, VID #B */
        FREQ_INFO(1500, 1324, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1212, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1148, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1068, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_715C_90[] = {
        /* 90 nm 1.50GHz Pentium M, VID #C */
        FREQ_INFO(1500, 1308, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1212, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1132, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1068, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_715D_90[] = {
        /* 90 nm 1.50GHz Pentium M, VID #D */
        FREQ_INFO(1500, 1276, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1180, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1116, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_778_90[] = {
        /* 90 nm 1.60GHz Low Voltage Pentium M */
        FREQ_INFO(1600, 1116, INTEL_BUS_CLK),
        FREQ_INFO(1500, 1116, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1100, INTEL_BUS_CLK),
        FREQ_INFO(1300, 1084, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1068, INTEL_BUS_CLK),
        FREQ_INFO(1100, 1052, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 900, 1036, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1020, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_758_90[] = {
        /* 90 nm 1.50GHz Low Voltage Pentium M */
        FREQ_INFO(1500, 1116, INTEL_BUS_CLK),
        FREQ_INFO(1400, 1116, INTEL_BUS_CLK),
        FREQ_INFO(1300, 1100, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1084, INTEL_BUS_CLK),
        FREQ_INFO(1100, 1068, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 900, 1036, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1020, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_738_90[] = {
        /* 90 nm 1.40GHz Low Voltage Pentium M */
        FREQ_INFO(1400, 1116, INTEL_BUS_CLK),
        FREQ_INFO(1300, 1116, INTEL_BUS_CLK),
        FREQ_INFO(1200, 1100, INTEL_BUS_CLK),
        FREQ_INFO(1100, 1068, INTEL_BUS_CLK),
        FREQ_INFO(1000, 1052, INTEL_BUS_CLK),
        FREQ_INFO( 900, 1036, INTEL_BUS_CLK),
        FREQ_INFO( 800, 1020, INTEL_BUS_CLK),
        FREQ_INFO( 600,  988, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_773G_90[] = {
        /* 90 nm 1.30GHz Ultra Low Voltage Pentium M, VID #G */
        FREQ_INFO(1300,  956, INTEL_BUS_CLK),
        FREQ_INFO(1200,  940, INTEL_BUS_CLK),
        FREQ_INFO(1100,  924, INTEL_BUS_CLK),
        FREQ_INFO(1000,  908, INTEL_BUS_CLK),
        FREQ_INFO( 900,  876, INTEL_BUS_CLK),
        FREQ_INFO( 800,  860, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_773H_90[] = {
        /* 90 nm 1.30GHz Ultra Low Voltage Pentium M, VID #H */
        FREQ_INFO(1300,  940, INTEL_BUS_CLK),
        FREQ_INFO(1200,  924, INTEL_BUS_CLK),
        FREQ_INFO(1100,  908, INTEL_BUS_CLK),
        FREQ_INFO(1000,  892, INTEL_BUS_CLK),
        FREQ_INFO( 900,  876, INTEL_BUS_CLK),
        FREQ_INFO( 800,  860, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_773I_90[] = {
        /* 90 nm 1.30GHz Ultra Low Voltage Pentium M, VID #I */
        FREQ_INFO(1300,  924, INTEL_BUS_CLK),
        FREQ_INFO(1200,  908, INTEL_BUS_CLK),
        FREQ_INFO(1100,  892, INTEL_BUS_CLK),
        FREQ_INFO(1000,  876, INTEL_BUS_CLK),
        FREQ_INFO( 900,  860, INTEL_BUS_CLK),
        FREQ_INFO( 800,  844, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_773J_90[] = {
        /* 90 nm 1.30GHz Ultra Low Voltage Pentium M, VID #J */
        FREQ_INFO(1300,  908, INTEL_BUS_CLK),
        FREQ_INFO(1200,  908, INTEL_BUS_CLK),
        FREQ_INFO(1100,  892, INTEL_BUS_CLK),
        FREQ_INFO(1000,  876, INTEL_BUS_CLK),
        FREQ_INFO( 900,  860, INTEL_BUS_CLK),
        FREQ_INFO( 800,  844, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_773K_90[] = {
        /* 90 nm 1.30GHz Ultra Low Voltage Pentium M, VID #K */
        FREQ_INFO(1300,  892, INTEL_BUS_CLK),
        FREQ_INFO(1200,  892, INTEL_BUS_CLK),
        FREQ_INFO(1100,  876, INTEL_BUS_CLK),
        FREQ_INFO(1000,  860, INTEL_BUS_CLK),
        FREQ_INFO( 900,  860, INTEL_BUS_CLK),
        FREQ_INFO( 800,  844, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_773L_90[] = {
        /* 90 nm 1.30GHz Ultra Low Voltage Pentium M, VID #L */
        FREQ_INFO(1300,  876, INTEL_BUS_CLK),
        FREQ_INFO(1200,  876, INTEL_BUS_CLK),
        FREQ_INFO(1100,  860, INTEL_BUS_CLK),
        FREQ_INFO(1000,  860, INTEL_BUS_CLK),
        FREQ_INFO( 900,  844, INTEL_BUS_CLK),
        FREQ_INFO( 800,  844, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_753G_90[] = {
        /* 90 nm 1.20GHz Ultra Low Voltage Pentium M, VID #G */
        FREQ_INFO(1200,  956, INTEL_BUS_CLK),
        FREQ_INFO(1100,  940, INTEL_BUS_CLK),
        FREQ_INFO(1000,  908, INTEL_BUS_CLK),
        FREQ_INFO( 900,  892, INTEL_BUS_CLK),
        FREQ_INFO( 800,  860, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_753H_90[] = {
        /* 90 nm 1.20GHz Ultra Low Voltage Pentium M, VID #H */
        FREQ_INFO(1200,  940, INTEL_BUS_CLK),
        FREQ_INFO(1100,  924, INTEL_BUS_CLK),
        FREQ_INFO(1000,  908, INTEL_BUS_CLK),
        FREQ_INFO( 900,  876, INTEL_BUS_CLK),
        FREQ_INFO( 800,  860, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_753I_90[] = {
        /* 90 nm 1.20GHz Ultra Low Voltage Pentium M, VID #I */
        FREQ_INFO(1200,  924, INTEL_BUS_CLK),
        FREQ_INFO(1100,  908, INTEL_BUS_CLK),
        FREQ_INFO(1000,  892, INTEL_BUS_CLK),
        FREQ_INFO( 900,  876, INTEL_BUS_CLK),
        FREQ_INFO( 800,  860, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_753J_90[] = {
        /* 90 nm 1.20GHz Ultra Low Voltage Pentium M, VID #J */
        FREQ_INFO(1200,  908, INTEL_BUS_CLK),
        FREQ_INFO(1100,  892, INTEL_BUS_CLK),
        FREQ_INFO(1000,  876, INTEL_BUS_CLK),
        FREQ_INFO( 900,  860, INTEL_BUS_CLK),
        FREQ_INFO( 800,  844, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_753K_90[] = {
        /* 90 nm 1.20GHz Ultra Low Voltage Pentium M, VID #K */
        FREQ_INFO(1200,  892, INTEL_BUS_CLK),
        FREQ_INFO(1100,  892, INTEL_BUS_CLK),
        FREQ_INFO(1000,  876, INTEL_BUS_CLK),
        FREQ_INFO( 900,  860, INTEL_BUS_CLK),
        FREQ_INFO( 800,  844, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_753L_90[] = {
        /* 90 nm 1.20GHz Ultra Low Voltage Pentium M, VID #L */
        FREQ_INFO(1200,  876, INTEL_BUS_CLK),
        FREQ_INFO(1100,  876, INTEL_BUS_CLK),
        FREQ_INFO(1000,  860, INTEL_BUS_CLK),
        FREQ_INFO( 900,  844, INTEL_BUS_CLK),
        FREQ_INFO( 800,  844, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};

static freq_info PM_733JG_90[] = {
        /* 90 nm 1.10GHz Ultra Low Voltage Pentium M, VID #G */
        FREQ_INFO(1100,  956, INTEL_BUS_CLK),
        FREQ_INFO(1000,  940, INTEL_BUS_CLK),
        FREQ_INFO( 900,  908, INTEL_BUS_CLK),
        FREQ_INFO( 800,  876, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_733JH_90[] = {
        /* 90 nm 1.10GHz Ultra Low Voltage Pentium M, VID #H */
        FREQ_INFO(1100,  940, INTEL_BUS_CLK),
        FREQ_INFO(1000,  924, INTEL_BUS_CLK),
        FREQ_INFO( 900,  892, INTEL_BUS_CLK),
        FREQ_INFO( 800,  876, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_733JI_90[] = {
        /* 90 nm 1.10GHz Ultra Low Voltage Pentium M, VID #I */
        FREQ_INFO(1100,  924, INTEL_BUS_CLK),
        FREQ_INFO(1000,  908, INTEL_BUS_CLK),
        FREQ_INFO( 900,  892, INTEL_BUS_CLK),
        FREQ_INFO( 800,  860, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_733JJ_90[] = {
        /* 90 nm 1.10GHz Ultra Low Voltage Pentium M, VID #J */
        FREQ_INFO(1100,  908, INTEL_BUS_CLK),
        FREQ_INFO(1000,  892, INTEL_BUS_CLK),
        FREQ_INFO( 900,  876, INTEL_BUS_CLK),
        FREQ_INFO( 800,  860, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_733JK_90[] = {
        /* 90 nm 1.10GHz Ultra Low Voltage Pentium M, VID #K */
        FREQ_INFO(1100,  892, INTEL_BUS_CLK),
        FREQ_INFO(1000,  876, INTEL_BUS_CLK),
        FREQ_INFO( 900,  860, INTEL_BUS_CLK),
        FREQ_INFO( 800,  844, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_733JL_90[] = {
        /* 90 nm 1.10GHz Ultra Low Voltage Pentium M, VID #L */
        FREQ_INFO(1100,  876, INTEL_BUS_CLK),
        FREQ_INFO(1000,  876, INTEL_BUS_CLK),
        FREQ_INFO( 900,  860, INTEL_BUS_CLK),
        FREQ_INFO( 800,  844, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
};
static freq_info PM_733_90[] = {
        /* 90 nm 1.10GHz Ultra Low Voltage Pentium M */
        FREQ_INFO(1100,  940, INTEL_BUS_CLK),
        FREQ_INFO(1000,  924, INTEL_BUS_CLK),
        FREQ_INFO( 900,  892, INTEL_BUS_CLK),
        FREQ_INFO( 800,  876, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};
static freq_info PM_723_90[] = {
        /* 90 nm 1.00GHz Ultra Low Voltage Pentium M */
        FREQ_INFO(1000,  940, INTEL_BUS_CLK),
        FREQ_INFO( 900,  908, INTEL_BUS_CLK),
        FREQ_INFO( 800,  876, INTEL_BUS_CLK),
        FREQ_INFO( 600,  812, INTEL_BUS_CLK),
        FREQ_INFO(   0,    0, 1),
};

/*
 * VIA C7-M 500 MHz FSB, 400 MHz FSB, and ULV variants.
 * Data from the "VIA C7-M Processor BIOS Writer's Guide (v2.17)" datasheet.
 */
static freq_info C7M_795[] = {
        /* 2.00GHz Centaur C7-M 533 Mhz FSB */
        FREQ_INFO_PWR(2000, 1148, 133, 20000),
        FREQ_INFO_PWR(1867, 1132, 133, 18000),
        FREQ_INFO_PWR(1600, 1100, 133, 15000),
        FREQ_INFO_PWR(1467, 1052, 133, 13000),
        FREQ_INFO_PWR(1200, 1004, 133, 10000),
        FREQ_INFO_PWR( 800,  844, 133,  7000),
        FREQ_INFO_PWR( 667,  844, 133,  6000),
        FREQ_INFO_PWR( 533,  844, 133,  5000),
        FREQ_INFO(0, 0, 1),
};
static freq_info C7M_785[] = {
        /* 1.80GHz Centaur C7-M 533 Mhz FSB */
        FREQ_INFO_PWR(1867, 1148, 133, 18000),
        FREQ_INFO_PWR(1600, 1100, 133, 15000),
        FREQ_INFO_PWR(1467, 1052, 133, 13000),
        FREQ_INFO_PWR(1200, 1004, 133, 10000),
        FREQ_INFO_PWR( 800,  844, 133,  7000),
        FREQ_INFO_PWR( 667,  844, 133,  6000),
        FREQ_INFO_PWR( 533,  844, 133,  5000),
        FREQ_INFO(0, 0, 1),
};
static freq_info C7M_765[] = {
        /* 1.60GHz Centaur C7-M 533 Mhz FSB */
        FREQ_INFO_PWR(1600, 1084, 133, 15000),
        FREQ_INFO_PWR(1467, 1052, 133, 13000),
        FREQ_INFO_PWR(1200, 1004, 133, 10000),
        FREQ_INFO_PWR( 800,  844, 133,  7000),
        FREQ_INFO_PWR( 667,  844, 133,  6000),
        FREQ_INFO_PWR( 533,  844, 133,  5000),
        FREQ_INFO(0, 0, 1),
};

static freq_info C7M_794[] = {
        /* 2.00GHz Centaur C7-M 400 Mhz FSB */
        FREQ_INFO_PWR(2000, 1148, 100, 20000),
        FREQ_INFO_PWR(1800, 1132, 100, 18000),
        FREQ_INFO_PWR(1600, 1100, 100, 15000),
        FREQ_INFO_PWR(1400, 1052, 100, 13000),
        FREQ_INFO_PWR(1000, 1004, 100, 10000),
        FREQ_INFO_PWR( 800,  844, 100,  7000),
        FREQ_INFO_PWR( 600,  844, 100,  6000),
        FREQ_INFO_PWR( 400,  844, 100,  5000),
        FREQ_INFO(0, 0, 1),
};
static freq_info C7M_784[] = {
        /* 1.80GHz Centaur C7-M 400 Mhz FSB */
        FREQ_INFO_PWR(1800, 1148, 100, 18000),
        FREQ_INFO_PWR(1600, 1100, 100, 15000),
        FREQ_INFO_PWR(1400, 1052, 100, 13000),
        FREQ_INFO_PWR(1000, 1004, 100, 10000),
        FREQ_INFO_PWR( 800,  844, 100,  7000),
        FREQ_INFO_PWR( 600,  844, 100,  6000),
        FREQ_INFO_PWR( 400,  844, 100,  5000),
        FREQ_INFO(0, 0, 1),
};
static freq_info C7M_764[] = {
        /* 1.60GHz Centaur C7-M 400 Mhz FSB */
        FREQ_INFO_PWR(1600, 1084, 100, 15000),
        FREQ_INFO_PWR(1400, 1052, 100, 13000),
        FREQ_INFO_PWR(1000, 1004, 100, 10000),
        FREQ_INFO_PWR( 800,  844, 100,  7000),
        FREQ_INFO_PWR( 600,  844, 100,  6000),
        FREQ_INFO_PWR( 400,  844, 100,  5000),
        FREQ_INFO(0, 0, 1),
};
static freq_info C7M_754[] = {
        /* 1.50GHz Centaur C7-M 400 Mhz FSB */
        FREQ_INFO_PWR(1500, 1004, 100, 12000),
        FREQ_INFO_PWR(1400,  988, 100, 11000),
        FREQ_INFO_PWR(1000,  940, 100,  9000),
        FREQ_INFO_PWR( 800,  844, 100,  7000),
        FREQ_INFO_PWR( 600,  844, 100,  6000),
        FREQ_INFO_PWR( 400,  844, 100,  5000),
        FREQ_INFO(0, 0, 1),
};
static freq_info C7M_771[] = {
        /* 1.20GHz Centaur C7-M 400 Mhz FSB */
        FREQ_INFO_PWR(1200,  860, 100,  7000),
        FREQ_INFO_PWR(1000,  860, 100,  6000),
        FREQ_INFO_PWR( 800,  844, 100,  5500),
        FREQ_INFO_PWR( 600,  844, 100,  5000),
        FREQ_INFO_PWR( 400,  844, 100,  4000),
        FREQ_INFO(0, 0, 1),
};

static freq_info C7M_775_ULV[] = {
        /* 1.50GHz Centaur C7-M ULV */
        FREQ_INFO_PWR(1500,  956, 100,  7500),
        FREQ_INFO_PWR(1400,  940, 100,  6000),
        FREQ_INFO_PWR(1000,  860, 100,  5000),
        FREQ_INFO_PWR( 800,  828, 100,  2800),
        FREQ_INFO_PWR( 600,  796, 100,  2500),
        FREQ_INFO_PWR( 400,  796, 100,  2000),
        FREQ_INFO(0, 0, 1),
};
static freq_info C7M_772_ULV[] = {
        /* 1.20GHz Centaur C7-M ULV */
        FREQ_INFO_PWR(1200,  844, 100,  5000),
        FREQ_INFO_PWR(1000,  844, 100,  4000),
        FREQ_INFO_PWR( 800,  828, 100,  2800),
        FREQ_INFO_PWR( 600,  796, 100,  2500),
        FREQ_INFO_PWR( 400,  796, 100,  2000),
        FREQ_INFO(0, 0, 1),
};
static freq_info C7M_779_ULV[] = {
        /* 1.00GHz Centaur C7-M ULV */
        FREQ_INFO_PWR(1000,  796, 100,  3500),
        FREQ_INFO_PWR( 800,  796, 100,  2800),
        FREQ_INFO_PWR( 600,  796, 100,  2500),
        FREQ_INFO_PWR( 400,  796, 100,  2000),
        FREQ_INFO(0, 0, 1),
};
static freq_info C7M_770_ULV[] = {
        /* 1.00GHz Centaur C7-M ULV */
        FREQ_INFO_PWR(1000,  844, 100,  5000),
        FREQ_INFO_PWR( 800,  796, 100,  2800),
        FREQ_INFO_PWR( 600,  796, 100,  2500),
        FREQ_INFO_PWR( 400,  796, 100,  2000),
        FREQ_INFO(0, 0, 1),
};

static ss_cpu_info ESTprocs[] = {
        INTEL(PM17_130,         1700, 1484, 600, 956, INTEL_BUS_CLK),
        INTEL(PM16_130,         1600, 1484, 600, 956, INTEL_BUS_CLK),
        INTEL(PM15_130,         1500, 1484, 600, 956, INTEL_BUS_CLK),
        INTEL(PM14_130,         1400, 1484, 600, 956, INTEL_BUS_CLK),
        INTEL(PM13_130,         1300, 1388, 600, 956, INTEL_BUS_CLK),
        INTEL(PM13_LV_130,      1300, 1180, 600, 956, INTEL_BUS_CLK),
        INTEL(PM12_LV_130,      1200, 1180, 600, 956, INTEL_BUS_CLK),
        INTEL(PM11_LV_130,      1100, 1180, 600, 956, INTEL_BUS_CLK),
        INTEL(PM11_ULV_130,     1100, 1004, 600, 844, INTEL_BUS_CLK),
        INTEL(PM10_ULV_130,     1000, 1004, 600, 844, INTEL_BUS_CLK),
        INTEL(PM_765A_90,       2100, 1340, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_765B_90,       2100, 1324, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_765C_90,       2100, 1308, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_765E_90,       2100, 1356, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_755A_90,       2000, 1340, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_755B_90,       2000, 1324, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_755C_90,       2000, 1308, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_755D_90,       2000, 1276, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_745A_90,       1800, 1340, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_745B_90,       1800, 1324, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_745C_90,       1800, 1308, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_745D_90,       1800, 1276, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_735A_90,       1700, 1340, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_735B_90,       1700, 1324, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_735C_90,       1700, 1308, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_735D_90,       1700, 1276, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_725A_90,       1600, 1340, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_725B_90,       1600, 1324, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_725C_90,       1600, 1308, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_725D_90,       1600, 1276, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_715A_90,       1500, 1340, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_715B_90,       1500, 1324, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_715C_90,       1500, 1308, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_715D_90,       1500, 1276, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_778_90,        1600, 1116, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_758_90,        1500, 1116, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_738_90,        1400, 1116, 600, 988, INTEL_BUS_CLK),
        INTEL(PM_773G_90,       1300,  956, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_773H_90,       1300,  940, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_773I_90,       1300,  924, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_773J_90,       1300,  908, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_773K_90,       1300,  892, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_773L_90,       1300,  876, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_753G_90,       1200,  956, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_753H_90,       1200,  940, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_753I_90,       1200,  924, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_753J_90,       1200,  908, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_753K_90,       1200,  892, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_753L_90,       1200,  876, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_733JG_90,      1100,  956, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_733JH_90,      1100,  940, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_733JI_90,      1100,  924, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_733JJ_90,      1100,  908, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_733JK_90,      1100,  892, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_733JL_90,      1100,  876, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_733_90,        1100,  940, 600, 812, INTEL_BUS_CLK),
        INTEL(PM_723_90,        1000,  940, 600, 812, INTEL_BUS_CLK),

        CENTAUR(C7M_795,        2000, 1148, 533, 844, 133),
        CENTAUR(C7M_794,        2000, 1148, 400, 844, 100),
        CENTAUR(C7M_785,        1867, 1148, 533, 844, 133),
        CENTAUR(C7M_784,        1800, 1148, 400, 844, 100),
        CENTAUR(C7M_765,        1600, 1084, 533, 844, 133),
        CENTAUR(C7M_764,        1600, 1084, 400, 844, 100),
        CENTAUR(C7M_754,        1500, 1004, 400, 844, 100),
        CENTAUR(C7M_775_ULV,    1500,  956, 400, 796, 100),
        CENTAUR(C7M_771,        1200,  860, 400, 844, 100),
        CENTAUR(C7M_772_ULV,    1200,  844, 400, 796, 100),
        CENTAUR(C7M_779_ULV,    1000,  796, 400, 796, 100),
        CENTAUR(C7M_770_ULV,    1000,  844, 400, 796, 100),
        { 0, 0, NULL },
};

#endif

