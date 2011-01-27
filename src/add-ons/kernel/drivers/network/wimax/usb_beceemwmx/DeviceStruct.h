/*
 *	Beceem WiMax USB Driver.
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the MIT license.
 *
 *	Based on GPL code developed by: Beceem Communications Pvt. Ltd
 */

#ifndef _USB_BECEEM_DEVICE_STRUCT_
#define _USB_BECEEM_DEVICE_STRUCT_

#include "Driver.h"

typedef struct FirmwareInfo
{
	void*			pvMappedFirmwareAddress;
	unsigned long	u32FirmwareLength;
	unsigned long	u32StartingAddress;
}__attribute__((packed)) FIRMWARE_INFO, *PFIRMWARE_INFO;

/*	WARNING: DO NOT edit this struct
 *	This struct is size / location dependant to vendor conf
 */
typedef struct _VENDORCFG
{
	/*
	Vendor Configuration Versions -- Beceem Internal
		Clear drivers use version 26
	# Version 1 - Creation
	# Version 2 - Major revision - Created the current structure
	# Version 3 - Renaming to HARQ structure to 'General'. HARQ will be
	#             reintroduced later.
	# Version 4 - Added enabling of Tx Power Report from config file
	# Version 5 - Added enabling of Random FA selection
	# Version 7 - Added the firmware config option
	# Version 8 - Added the config option for 8.75 or 10 MHz bandwidth
	# Version 9 - Added the config option for ShutDown Timer
	#           - Removed changes in Version 8
	# Version 10 - Merged versions 8 and 9
	# Version 11 - EncrSupport & NumOfSAId added
	# Version 12 - Radio and PHY Parameters added
	# Version 13 - options for testing cqich, rang etc
	# Version 14 - NA
	# Version 15 - Added Max MAC Data per Frame to be sent in REG-REQ
	# Version 16 - Added flags to enable Corrigendum 2
	# Version 17 - IDLE Reserved1 field is changed to idle mode option.
	#              0 means normal, 1 for CPE special Idle mode
	# Version 18 - Changes to Radio Parameter, high band support added
	# Version 19 - Changed Idlemode Enable to powersaving modes enable for Sleep
	#            - Changed Idlemode options to powersaving mode enable for Sleep
	# Version 20 - Changed Rtps Enable to Minimum Grant size, nRtps Enable to
	#              Maximum Grant size, PC reserved 4 to MimoEnabled
	# Version 21 - Changed eRtps Enable to PHS Enabled
	# Version 22 - Removed PC related cfg options (made them unused).
	#            - Added a test option for connect-disconnect testing
	# Version 23 - Removed the unused parameter from the config file
	# version 24 - change config file as the latest config file not consistent
	# version 25 - Added Segmented PUSC config option
	# version 26 - Updated customize field for Wibro Nw
	# version 27 - Added Shutdown Related Params
	# version 28 - Added Band AMC Related Params
	# version 29 - Updated HostDrvrConfig6 for DDR Memory Clock Frequency
	#              selection & Power Save Mode Selection.
	# version 30 - Changed the "MAX MAC Data per frame" from 15:4 to 75:0
	#              (to reflect T3 capability - for T2 its still 15:4 this is
	#              handled in MS firmware)
	# Version 31 - Added CFG option for HARQ channel mapping
	# Version 32 - Added CFG options for ERTPS
	#            - Removed CFG option for HARQ channel mapping
	# Version 33 - Sync 4.x and 5.x CFG file
	# Version 34 - Updated the comments for HostDrvrConfig6.
	#            - Added Cfg options for Enabled Action Trigger and TTWF
	#              values for Sleepmode
	# Version 35 - Added support for IP retention (bit 1 of customize field)
	# Version 36 - Renamed the PHS Enable field to CS options. This has now been
	#              made a bit field
	# Version 37 - Updated HostDrvrConfig4(31:30) and HostDrvrConfig5(31:0) for
	#              WiMAX trigger type and threshold
	# Version 39 - Using the handoff enable flags to enable/disable HO on signal
	#              fade (Active State Retention feature).
	# Version 40 - Clean up and add support for MIMO B enable/disable, and
	#              Encryption enable/disable
	# Version 41 - Added bitwise support for Eth CS
	# Version 42 - Added support for CFG based purge timer setting for real time
	#              HARQ connections
	# Version 43 - Added config for out of HARQ order delivery feature
	# Version 44 - Added config for enabling RPD 124 changes related to Tx power
	#              report
	# Version 45 - Added flag in Wimax options to enable/disable GPIO 2
	#              signaling of Out of Wimax Coverage.
	# Version 46 - Added flag to enable inclusion of Idlemode MAC Hash Skip
	#              Threshold TLV
	# Version 47 - Added DP options
	# Version 48 - Added flag in Wimax options to enable/disable GPIO 14 based
	#              low power mode (LPM) entry
	# Version 49 - Added flag under ARQ ENable for disabling ARQ Cut Thru for
	#              Ack generation
	# Version 50 - Added flag for handling partial nacking of PDUs, and retxing
	#              only nacked blocks  */
	unsigned int	m_u32CfgVersion;

	/*
	Scanning Parameters
		Make center frequency as 0 to enable scanning
		Or use: 2336, 2345, 2354, 2367.5, 2385.5
		Choosing the center frequency non-zero will
		disable scanning! */
	unsigned int	m_u32CenterFrequency;
	unsigned int	m_u32BandAScan;
	unsigned int	m_u32BandBScan;
	unsigned int	m_u32BandCScan;

	// QoS Params
	unsigned int	m_u32minGrantsize;       // size of minimum grant is 0 or 6
	unsigned int	m_u32PHSEnable;

	// HO Params
	unsigned int	m_u32HoEnable;
	unsigned int	m_u32HoReserved1;
	unsigned int	m_u32HoReserved2;

	/*
	MIMO Enable ==> 0xddccbbaa
		aa = 0x01 Enables DL MIMO,
		bb = 0x01 Enabled UL CSM ;
		cc = 0x01 to disable MIMO B support
		0xdd Reserved
		0x0101 => Enables DL MIMO and UL CSM
		0x010101 =? Enable DL MIMO, UL CSM, and disable MIMO B in DL */
	unsigned int	m_u32MimoEnable;

	/*
	Security Parameters
		bit 0 = Enable PKMV2 support (Auth support)
		bit 1 = Enable index based SFID mapping for MS init SFs
		bit 2 = Enable domain restriction for security
		bit 3 = Disable encryption support
		bit[31:4] = Unused. */
	unsigned int	m_u32SecurityEnable;

	/*
	PowerSaving enable
		bit 1 = 1 Idlemode enable
		bit 2 = 1 Sleepmode Enable */
	unsigned int	m_u32PowerSavingModesEnable;

	/*
	PowerSaving Mode Options
		bit 0 = 1: CPE mode - to keep pcmcia if alive;
		bit 1 = 1: CINR reporing in Idlemode Msg
		bit 2 = 1: Default PSC Enable in sleepmode */
	unsigned int	m_u32PowerSavingModeOptions;

	/*
	ARQ Enable ==> 0xddccbbaa
		aa => 0x01 Enables ARQ
		bb => 0x01 Disables the ARQ feature where MS combines ARQ FB and tranport
		CID Bw Requests:
		cc => 0x01 Enable ARQ FB BW req enh
		dd => 0th bit Disable ARQ Cut thru for Ack Generation
		dd => 1st bit enables handling of partial nacking of PDUs, and retxing */
	unsigned int	m_u32ArqEnable;

	/*
	HARQ Enable ==> 0xddccbbaa
		aa => 0x01 Enables HARQ on Transport;
		bb => 0 bit Enables HARQ on Management
		bb => 1 bit Enables Out of order delivery on non-ARQ Rx ERTPS HARQ connections
		bb => 2 bit Enables Out of order delivery on all non-ARQ RX HARQ connections
		dd => If set, used as the PDU purge timer on the non-ARQ Rx HARQ RT connections
		ERTPS, RTPS, UGS
		HARQ on Management only is NOT Permitted.
		Eg. 0x0101 => Enables HARQ on Management and HARQ on Transport Connections
		Eg. 0x0301 => Enables HARQ on Management and HARQ on Transport Connections
					  Enables out of deliver on the non-ARQ rx ERTPS HARQ connections
		Eg. 0x0501 => Enables HARQ on Management and HARQ on Transport Connections
					  Enables out of deliver on the non-ARQ RX HARQ connections */
	unsigned int	m_u32HarqEnable;

	// EEPROM Param Location
	unsigned int	m_u32EEPROMFlag;

	/*
	Customize
		Normal Mode should be set to (0x00000100)
		Set bit 0 for using D5 CQICH IE (WiBro NW)
		Bit 26 should be set to disable the BEU */
	unsigned int	m_u32Customize;

	/*
	Config Bandwidth
		Should be in Hz i.e. 8750000, 10000000 */
	unsigned int	m_u32ConfigBW;

	/*
	Shutdown Timer
		number of frames (5 ms)
		ShutDown Timer Value =  0x7fffffff */
	unsigned int	m_u32ShutDownTimer;

	/*
	Radio Parameter
		e.g. 0x000dccba
		d  = [19:16]	Second/High Band Select
			{ 2->2.3 to 2.4G; 3->2.5 to 2.7G; 4->3.4 to 3.6G,
				F-> Select from EEPROM if info available }
			Do not fill for single band unit
		cc = [15:8]		Board Type, set to 0
		b  = [7:4]		Primary/Low Band Select
			{ 2->2.3 to 2.4G; 3->2.5 to 2.7G; 4->3.4 to 3.6G,
				F-> Select from EEPROM if info available }
		a  = [3:0] Set to 2

		Examples:
		# 2.3GHz MS120 single band unit needs 0x22
		# 2.5GHz MS120 single band unit needs 0x32
		# 3.5GHz MS120 single band unit needs 0x42
		# 2.3G and/or 3.5G BCS200 unit needs 0x40022
		# 2.5G and/or 3.5G BCS200 unit needs 0x40032 */
	unsigned int	m_u32RadioParameter;

	/*
	PhyParameter1 e.g. 0xccccbbaa
		cccc	= [31:16]	80 * Value of TTG in us
		bb		= [15:8]	Number of DL symbols
		aa		= [7:0]		Number of UL symbols
		Special value of 0xFFFFFFFF indicates use of embedded logic
		PhyParameter1 = 0xFFFFFFFF */
	unsigned int	m_u32PhyParameter1;

	/*
	PhyParameter2
		[16]	Set to 1 for accurate CINR measurement on noise limited scenarios
				If set to 1, note the BTS Link Adaptation tables may need change
		[15:8]	Backoff in 0.25dBm steps in Transmit power to be
				applied for an uncalibrated unit */
	unsigned int	m_u32PhyParameter2;

	/*
	PhyParameter3 = 0x0 */
	unsigned int	m_u32PhyParameter3;

	/*
	TestOptions
		in eval mode only;
		-lower 16bits = basic cid for testing;
		-then bit 16 is test cqich,
		-bit 17  test init rang;
		-bit 18 test periodic rang
		-bit 19 is test harq ack/nack

		in normal mode;
		-#define ENABLE_RNG_REQ_RPD_R4								0x1
		-#define ENABLE_RNG_REQ_RPD_R3								0x2
		-#define TEST_HOCODE_IN_IM_REENTRY_RPD						0x4
		-#define ENABLE_PHY_PARAMETERS_RPD							0x8
		-#define ENABLE_INDEP_CONTROL_DATA_POW_SCALLING				0x10
		-#define ENABLE_PTXIRMAX_FOR_ALL_CDMA_CODES					0x20
		-#define ENABLE_EXT_BS_INITIATED_IDLE_MODE					0x40
		-#define DISABLE_RNG_REQ_ANOMALY							0x100
		-#define ENABLE_HARQ_TLVS_IN_DSA_RSP_IOPR					0x200
		-#define ENABLE_TX_PWR_RPT_RPD_124							0x400
		-#define INCLUDE_IM_MAC_HASH_SKIP_THRESH_TLV				0x800
		-#define ENABLE_POLL_ME_BIT_USING_UGS						0x8000
		-#define NW_ENTRY_DREG										0x100000
		-#define TEST_DISABLE_DSX_FLOW_CONTROL_TLV					0x200000
		-#define TEST_SBC_OPTIMIZATION								0x400000
		-#define TEST_DISABLE_EARLY_TX_POWER_GEN					0x1000000
		-#define LA_DISABLE_ECINR_SUPPORT							0x2000000
		-#define ENABLE_NSP_SBC_DISCOVERY_METHOD					0x8000000
		-#define ENABLE__BF_WA									 	0x20000000
		-#define ENABLE_BR_PWR_INCR_AND_IGNORE_PC_IE_OUT_DYNAMIC_RNG 0x40000000
		-#define ENABLE_BR_CODE_TIMEOUT_PWR_ADJ_PDUS				0x80000000*/
	unsigned int	m_u32TestOptions;

	/*
	Max MAC Data per Frame to be sent in REG-REQ */
	unsigned int	m_u32MaxMACDataperDLFrame;
	unsigned int	m_u32MaxMACDataperULFrame;

	unsigned int	m_u32Corr2MacFlags;

	/*
	HostDrvrConfig1/HostDrvrConfig2/HostDrvrConfig3
		12 Bytes are required for setting the LED Configurations.
		LED Type (This can be either RED/YELLOW/GREEN/FLUORESCENT)
		ON State (This makes sure that the LED is ON till a state is achieved)
		Blink State (This makes sure that the LED Keeps Blinking till a state is
		achieved e.g. firmware download state)*/
	unsigned int	HostDrvrConfig1;
	unsigned int	HostDrvrConfig2;
	unsigned int	HostDrvrConfig3;

	/*
	HostDrvrConfig4/HostDrvrConfig5
		Used for "WiMAX Trigger" for out of WiMAX Coverage.
		HostDrvrConfig4 [31:30]
			To set WiMAX Trigger Type:
				00 -"Not Defined", 01 -"RSSI", 10 -"CINR", 11 -"Reserved"
		HostDrvrConfig5 [31:0]
			To set WiMAX Trigger Threshold for the type
			selected in HostDrvrConfig4 */
	unsigned int	HostDrvrConfig4;
	unsigned int	HostDrvrConfig5;

	/*
	HostDrvrConfig6:
		Important device configuration data
		Bit[3..0]	Control Flags
			Bit 0: Automatic Firmware Download 1: Enable, 0: Disable
			Bit 1: Auto Linkup 1: Enable, 0: Disable
			Bit 2: Reserved.
			Bit 3: Reserved.
		Bit[7..4]	RESERVED
		Bit[11..8]	DDR Memory Clock Configuration
			MEM_080_MHZ	= 0x0
			MEM_133_MHZ	= 0x3
			MEM_160_MHZ	= 0x5
		Bit[14..12]	PowerSaveMethod
			DEVICE_POWERSAVE_MODE_AS_MANUAL_CLOCK_GATING	= 0x0
			DEVICE_POWERSAVE_MODE_AS_PMU_CLOCK_GATING		= 0x1
			DEVICE_POWERSAVE_MODE_AS_PMU_SHUTDOWN			= 0x2
		Bit[15]		Idlemode Auto correct mode.
			0-Enable, 1- Disable. */
	unsigned int	HostDrvrConfig6;

	/*
	Segmented PSUC
		Option to enable support of Segmented PUSC.
		If set to 0, only full reuse profiles are supported */
	unsigned int	m_u32SegmentedPUSCenable;

	/*
	BAMC Related Parameters
	4.x does not support this feature
	This is added just to sync 4.x and 5.x CFGs
		Bit[0..15]	Band AMC signaling configuration:
			Bit 0 = 1 Enable Band AMC signaling.
		Bit[16..31]	Band AMC Data configuration:
			Bit 16 = 1 Band AMC 2x3 support.
		Bit 0 is effective only if bit 16 is set. */
	unsigned int	 m_u32BandAMCEnable;

} VENDORCFG, *PSVENDORCFG;

/*Structure which stores the information of different LED types
 *  and corresponding LED state information of driver states*/
typedef struct _GPIO_LED_STATE
{
	uint8_t 	LED_Type;
		// specify GPIO number - use 0xFF if not used
	uint8_t 	LED_On_State;
		// Bits set or reset for different states
	uint8_t 	LED_Blink_State;
		// Bits set or reset for blinking LEDs for different states
	uint8_t		GPIO_Num;
		// GPIO number (this is the bit offset and is 1<<n'ed)
	uint8_t 	BitPolarity;
		// Is hardware normal polarity or reverse polarity

} GPIO_LED_STATE, *PSGPIO_LED_STATE;

typedef struct _GPIO_LED_MAP
{
	GPIO_LED_STATE		LEDState[NUM_OF_LEDS];
		// struct stores the state of the LED's
	bool				bIdleMode_tx_from_host;
		// Store notification if driver came out of idle mode
		// due to Host or target
	bool				bIdle_led_off;

//  TODO : figure out how to handle these linux wait typedefs
//	wait_queue_head_t	notify_led_event;
//	wait_queue_head_t	idleModeSyncEvent;
	bool					notify_led_event;
	bool					idleModeSyncEvent;
//  TODO END : figure out how to handle these linux wait typedefs

	struct task_struct	*led_cntrl_threadid;
	int					led_thread_running;
	bool				bLedInitDone;

}GPIO_LED_MAP, *PSGPIO_LED_MAP;

typedef struct _FLASH_CS_INFO
{
	uint32_t	MagicNumber;
		// 0xBECE - F1A5 for FLAS(H)

	uint32_t	FlashLayoutVersion;
		// Flash layout version

	uint32_t	ISOImageVersion;
		// ISO Image / Format / Eng version

	uint32_t	SCSIFirmwareVersion;
		// SCSI Firmware Version

	uint32_t	OffsetFromZeroForPart1ISOImage;
		// Normally 0

	uint32_t	OffsetFromZeroForScsiFirmware;
		// Normally 12MB

	uint32_t	SizeOfScsiFirmware;
		// Size of firmware, varies

	uint32_t	OffsetFromZeroForPart2ISOImage;
		// 1st word offset 12MB + sizeofScsiFirmware

	uint32_t	OffsetFromZeroForCalibrationStart;
	uint32_t	OffsetFromZeroForCalibrationEnd;

	uint32_t	OffsetFromZeroForVSAStart;
	uint32_t	OffsetFromZeroForVSAEnd;
		// VSA0 offsets

	uint32_t	OffsetFromZeroForControlSectionStart;
	uint32_t	OffsetFromZeroForControlSectionData;
		// Control Section offsets

	uint32_t	CDLessInactivityTimeout;
		// NO Data Activity timeout to switch from MSC to NW Mode

	uint32_t	NewImageSignature;
		// New ISO Image Signature

	uint32_t	FlashSectorSizeSig;
		// Signature to validate the sector size.

	uint32_t	FlashSectorSize;
		// Sector Size

	uint32_t	FlashWriteSupportSize;
		// Write Size Support

	uint32_t	TotalFlashSize;
		// Total Flash Size

	uint32_t	FlashBaseAddr;
		// Flash Base Address for offset specified

	uint32_t	FlashPartMaxSize;
		// Flash Part Max Size

	uint32_t	IsCDLessDeviceBootSig;
		// Is CDLess or Flash Bootloader

	uint32_t	MassStorageTimeout;
		// MSC Timeout after reset to switch from MSC to NW Mode

} FLASH_CS_INFO, *PFLASH_CS_INFO;

struct WIMAX_DEVICE
{
			VENDORCFG			vendorcfg;
				// we memcpy the vendor cfg here
			unsigned int        syscfgBefFw;
				// SYS_CFG before firmware
			bool                CPUFlashBoot;
				// Reverse MIPS
			unsigned int        deviceChipID;
				// Beceem interface chip model
volatile	bool				driverHalt;
				// gets set to true when driver is being shutdown
volatile	unsigned int		driverState;
				// Driver state, defined in Driver.h
volatile	bool				driverDDRinit;
				// has the DDR memory been initialized?
volatile	bool				driverFwPushed;
				// has the firmware been pushed to the device?


			GPIO_LED_MAP		LEDMap;
				// Map of LED's in GPIO
			thread_id			LEDThreadID;
				// Thread ID of LED state handler

			unsigned int		nvmType;
				// NVM Type (FLASH|EEPROM|UNKNOWN)
			unsigned int		nvmDSDSize;
				// NVM DSD Size
			unsigned int		nvmVerMajor;
				// NVM Major
			unsigned int		nvmVerMinor;
				// NVM Minor

volatile	uint32_t			nvmFlashBaseAddr;
				// NVM Flash base address
			unsigned long		nvmFlashCalStart;
				// NVM Flash calibrated start
			unsigned long		nvmFlashCSStart;
				// NVM Flash CS start
			PFLASH_CS_INFO		nvmFlashCSInfo;
				// NVM Flash CS info
			unsigned long int	nvmFlashID;
				// ID of Flash chip
volatile	bool				nvmFlashCSDone;
				// Has CS been read yet?
volatile	bool				nvmFlashRaw;
				// Do we need raw access at the moment?

			unsigned int		nvmFlashMajor;
				// NVM Flash layout major version
			unsigned int		nvmFlashMinor;
				// NVM Flash layout major version

			unsigned int		hwParamPtr;
				// Pointer to the NVM Param section address
			unsigned char		gpioInfo[32];
				// Stored GPIO information
			unsigned int		gpioBitMap;
				// GPIO LED bitmap
};

#endif // _USB_BECEEM_DEVICE_STRUCT_

