/*
 * Copyright 2018-2019 Haiku, Inc. All rights reserved.
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


class Command {
	public:
		uint16_t Bits() { return fBits; }

		void SendCommand(uint8_t command, uint8_t type)
		{
			fBits = (command << 8) | type;
		}

		static const uint8_t kNoReplyType = 0;
		static const uint8_t kR1Type = 0x1C;
		static const uint8_t kR2Type = 0x09;
		static const uint8_t kR3Type = 0x02;
		static const uint8_t kR6Type = 0x1C;
		static const uint8_t kR7Type = 0x3C;

	private:
		volatile uint16_t fBits;
} __attribute__((packed));
#define SDHCI_RESPONSE_R1                               2
#define SDHCI_CMD_CRC_EN                                1 << 3
#define SDHCI_CMD_INDEX_EN                              1 << 4


class PresentState {
	public:
		uint32_t Bits() { return fBits; }

		bool IsCardInserted() { return fBits & (1 << 16); }
		bool CommandInhibit() { return fBits & (1 << 0); }

	private:
		volatile uint32_t fBits;
} __attribute__((packed));


class PowerControl {
	public:
		uint8_t Bits() { return fBits; }

		void SetVoltage(int voltage) {
			fBits |= voltage | kBusPowerOn;
		}
		void PowerOff() { fBits &= ~kBusPowerOn; }

		static const uint8_t k3v3 = 7 << 1;
		static const uint8_t k3v0 = 6 << 1;
		static const uint8_t k1v8 = 5 << 1;
	private:
		volatile uint8_t fBits;

		static const uint8_t kBusPowerOn = 1;
} __attribute__((packed));


class ClockControl
{
	public:
		uint16_t Bits() { return fBits; }

		uint16_t SetDivider(uint16_t divider) {
			if (divider == 1)
				divider = 0;
			else
				divider /= 2;
			uint16_t bits = fBits & ~0xffc0;
			bits |= divider << 8;
			bits |= (divider >> 8) & 0xc0;
			fBits = bits;

			return divider == 0 ? 1 : divider * 2;
		}

		void EnableInternal() { fBits |= 1 << 0; }
		bool InternalStable() { return fBits & (1 << 1); }
		void EnableSD() { fBits |= 1 << 2; }
		void DisableSD() { fBits &= ~(1 << 2); }
		void EnablePLL() { fBits |= 1 << 3; }
	private:
		volatile  uint16_t fBits;
} __attribute__((packed));


class SoftwareReset {
	public:
		uint8_t Bits() { return fBits; }

		void ResetAll() {
			fBits |= 1;
			while(fBits & 1);
		}

		void ResetCommandLine() {
			fBits |= 2;
			while(fBits & 2);
		}

	private:
		volatile uint8_t fBits;
} __attribute__((packed));


/* Interrupt registers */
#define SDHCI_INT_CMD_CMP		0x00000001 		// command complete enable
#define SDHCI_INT_TRANS_CMP		0x00000002		// transfer complete enable
#define SDHCI_INT_CARD_INS 		0x00000040 		// card insertion enable
#define SDHCI_INT_CARD_REM 		0x00000080 		// card removal enable
#define SDHCI_INT_ERROR         0x00008000      // error
#define SDHCI_INT_TIMEOUT		0x00010000 		// Timeout error
#define SDHCI_INT_CRC			0x00020000 		// CRC error
#define SDHCI_INT_END_BIT		0x00040000 		// end bit error
#define SDHCI_INT_INDEX 		0x00080000		// index error
#define SDHCI_INT_BUS_POWER		0x00800000 		// power fail

#define	 SDHCI_INT_CMD_ERROR_MASK	(SDHCI_INT_TIMEOUT | \
		SDHCI_INT_CRC | SDHCI_INT_END_BIT | SDHCI_INT_INDEX)

#define SDHCI_INT_CMD_MASK 	(SDHCI_INT_CMD_CMP | SDHCI_INT_CMD_ERROR_MASK)


class Capabilities
{
	public:
		uint64_t Bits() { return fBits; }

		uint8_t SupportedVoltages() { return (fBits >> 24) & 7; }
		uint8_t BaseClockFrequency() { return (fBits >> 8) & 0xFF; }

		static const uint8_t k3v3 = 1;
		static const uint8_t k3v0 = 2;
		static const uint8_t k1v8 = 4;

	private:
		const uint64_t fBits;
} __attribute__((packed));


class HostControllerVersion {
	public:
		const uint8_t specVersion;
		const uint8_t vendorVersion;
} __attribute__((packed));


struct registers {
	// SD command generation
	volatile uint32_t system_address;
	volatile uint16_t block_size;
	volatile uint16_t block_count;
	volatile uint32_t argument;
	volatile uint16_t transfer_mode;
	Command command;

	// Response
	volatile uint32_t response[4];

	// Buffer Data Port
	volatile uint32_t buffer_data_port;

	// Host control 1
	PresentState present_state;
	volatile uint8_t host_control;
	PowerControl power_control;
	volatile uint8_t block_gap_control;
	volatile uint8_t wakeup_control;
	ClockControl clock_control;
	volatile uint8_t timeout_control;
	SoftwareReset software_reset;

	// Interrupt control
	volatile uint32_t interrupt_status;
	volatile uint32_t interrupt_status_enable;
	volatile uint32_t interrupt_signal_enable;
	volatile uint16_t auto_cmd12_error_status;

	// Host control 2
	volatile uint16_t host_control_2;

	// Capabilities
	Capabilities capabilities;
	volatile uint64_t max_current_capabilities;

	// Force event
	volatile uint16_t force_event_acmd_status;
	volatile uint16_t force_event_error_status;

	// ADMA2
	volatile uint8_t adma_error_status;
	volatile uint8_t padding[3];
	volatile uint64_t adma_system_address;

	// Preset values
	volatile uint64_t preset_value[2];
	volatile uint32_t :32;
	volatile uint16_t uhs2_preset_value;
	volatile uint16_t :16;

	// ADMA3
	volatile uint64_t adma3_id_address;

	// UHS-II
	volatile uint16_t uhs2_block_size;
	volatile uint16_t :16;
	volatile uint32_t uhs2_block_count;
	volatile uint8_t uhs2_command_packet[20];
	volatile uint16_t uhs2_transfer_mode;
	volatile uint16_t uhs2_command;
	volatile uint8_t uhs2_response[20];
	volatile uint8_t uhs2_msg_select;
	volatile uint8_t padding2[3];
	volatile uint32_t uhs2_msg;
	volatile uint16_t uhs2_device_interrupt_status;
	volatile uint8_t uhs2_device_select;
	volatile uint8_t uhs2_device_int_code;
	volatile uint16_t uhs2_software_reset;
	volatile uint16_t uhs2_timer_control;
	volatile uint32_t uhs2_error_interrupt_status;
	volatile uint32_t uhs2_error_interrupt_status_enable;
	volatile uint32_t uhs2_error_interrupt_signal_enable;
	volatile uint8_t padding3[16];

	// Pointers
	volatile uint16_t uhs2_settings_pointer;
	volatile uint16_t uhs2_host_capabilities_pointer;
	volatile uint16_t uhs2_test_pointer;
	volatile uint16_t embedded_control_pointer;
	volatile uint16_t vendor_specific_pointer;
	volatile uint16_t reserved_specific_pointer;
	volatile uint8_t padding4[16];

	// Common area
	volatile uint16_t slot_interrupt_status;
	HostControllerVersion host_controller_version;
} __attribute__((packed));

typedef void* sdhci_mmc_bus;



#endif /*_SDHCI_PCI_H*/
