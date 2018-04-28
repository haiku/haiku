/*
 * Copyright 2018 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 */
#ifndef _SDHCI_PCI_H
#define _SDHCI_PCI_H


#include <device_manager.h>
#include <KernelExport.h>


#define SDHCI_PCI_SLOT_INFO 							0x40
#define SDHCI_PCI_SLOTS(x) 								((( x >> 4) & 7))
#define SDHCI_PCI_SLOT_INFO_FIRST_BASE_INDEX(x)			(( x ) & 7)

#define SDHCI_DEVICE_TYPE_ITEM 							"sdhci/type"
#define SDHCI_BUS_TYPE_NAME 							"bus/sdhci/v1"

#define SDHCI_CARD_DETECT								1 << 16

#define SDHCI_SOFTWARE_RESET_ALL						1 << 0

#define SDHCI_BASE_CLOCK_FREQ(x)						((x >> 8) & 63)
#define SDHCI_VOLTAGE_SUPPORTED(x)                      ((x >> 24) & 7)
#define SDHCI_VOLTAGE_SUPPORTED_33(x)                   ((x >> 24) & 1)
#define SDHCI_VOLTAGE_SUPPORTED_30(x)                  	((x >> 24) & 3)
#define SDHCI_VOLTAGE_SUPPORT_33						7 << 1
#define SDHCI_VOLTAGE_SUPPORT_30						6 << 1
#define SDHCI_VOLTAGE_SUPPORT_18						5 << 1
#define SDHCI_CARD_INSERTED(x)							((x >> 16) & 1)
#define SDHCI_BASE_CLOCK_DIV_1							0 << 8
#define SDHCI_BASE_CLOCK_DIV_2							1 << 8
#define SDHCI_BASE_CLOCK_DIV_4							2 << 8
#define SDHCI_BASE_CLOCK_DIV_8							4 << 8
#define SDHCI_BASE_CLOCK_DIV_16							8 << 8
#define SDHCI_BASE_CLOCK_DIV_32							16 << 8
#define SDHCI_BASE_CLOCK_DIV_64							32 << 8
#define SDHCI_BASE_CLOCK_DIV_128						64 << 8
#define SDHCI_BASE_CLOCK_DIV_256						128 << 8
#define SDHCI_INTERNAL_CLOCK_ENABLE						1 << 0
#define SDHCI_INTERNAL_CLOCK_STABLE						1 << 1
#define SDHCI_SD_CLOCK_ENABLE							1 << 2
#define SDHCI_SD_CLOCK_DISABLE							~(1 << 2)
#define SDHCI_CLR_FREQ_SEL 							    ~(255 << 8)
#define SDHCI_BUS_POWER_ON							    1
#define SDHCI_RESPONSE_R1                               2
#define SDHCI_CMD_CRC_EN                                1 << 3
#define SDHCI_CMD_INDEX_EN                              1 << 4
#define SDHCI_CMD_0                                     ~(63 << 8)
/* Interrupt registers */
#define SDHCI_INT_CMD_CMP		0x00000001 		// command complete enable
#define SDHCI_INT_TRANS_CMP		0x00000002		// transfer complete enable
#define SDHCI_INT_CARD_INS 		0x00000040 		// card insertion enable
#define SDHCI_INT_CARD_REM 		0x00000080 		// card removal enable
#define SDHCI_INT_TIMEOUT		0x00010000 		// Timeout error
#define SDHCI_INT_CRC			0x00020000 		// CRC error
#define SDHCI_INT_END_BIT		0x00040000 		// end bit error
#define SDHCI_INT_INDEX 		0x00080000		// index error
#define SDHCI_INT_BUS_POWER		0x00800000 		// power fail

#define	 SDHCI_INT_CMD_ERROR_MASK	(SDHCI_INT_TIMEOUT | \
		SDHCI_INT_CRC | SDHCI_INT_END_BIT | SDHCI_INT_INDEX)

#define SDHCI_INT_CMD_MASK 	(SDHCI_INT_CMD_CMP | SDHCI_INT_CMD_ERROR_MASK)


struct registers {
	volatile uint32_t system_address;
	volatile uint16_t block_size;
	volatile uint16_t block_count;
	volatile uint32_t argument;
	volatile uint16_t transfer_mode;
	volatile uint16_t command;
	volatile uint32_t response0;
	volatile uint32_t response2;
	volatile uint32_t response4;
	volatile uint32_t response6;
	volatile uint32_t buffer_data_port;
	volatile uint32_t present_state;
	volatile uint8_t host_control;
	volatile uint8_t power_control;
	volatile uint8_t block_gap_control;
	volatile uint8_t wakeup_control;
	volatile uint16_t clock_control;
	volatile uint8_t timeout_control;
	volatile uint8_t software_reset;
	volatile uint32_t interrupt_status;
	volatile uint32_t interrupt_status_enable;
	volatile uint32_t interrupt_signal_enable;
	volatile uint16_t auto_cmd12_error_status;
	volatile uint16_t : 16;
	volatile uint32_t capabilities;
	volatile uint32_t capabilities_rsvd;
	volatile uint32_t max_current_capabilities;
	volatile uint32_t max_current_capabilities_rsvd;
	volatile uint64_t padding[21];
	volatile uint32_t : 32;
	volatile uint16_t slot_interrupt_status;
	volatile uint16_t host_control_version;
} __attribute__((packed));

typedef void* sdhci_mmc_bus;

#define DELAY(n)	snooze(n)

#define SDHCI_BUS_CONTROLLER_MODULE_NAME "bus_managers/mmc_bus/driver_v1"

#define	MMC_BUS_MODULE_NAME "bus_managers/mmc_bus/device/v1"

static void sdhci_register_dump(uint8_t, struct registers*);
static void sdhci_reset(struct registers*);
static void sdhci_set_clock(struct registers*, uint16_t);
static void sdhci_set_power(struct registers*);
static void sdhci_stop_clock(struct registers*);
void sdhci_error_interrupt_recovery(struct registers*);

status_t sdhci_generic_interrupt(void*);

typedef struct {
	driver_module_info info;

} sdhci_mmc_bus_interface;


#endif /*_SDHCI_PCI_H*/
