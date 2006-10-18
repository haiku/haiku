/*
	Driver for Intel(R) PRO/Wireless 2100 devices.
	Copyright (C) 2006 Michael Lotz <mmlr@mlotz.ch>
	Released under the terms of the MIT license.
*/

#ifndef _IPW2100_HW_H_
#define _IPW2100_HW_H_

// buffer descriptor
struct ipw_bd {
	uint32	physical_address;
	uint32	length;
	uint8	flags;
	uint8	fragment_count;
	uint8	reserved[6];
} _PACKED;

#define	IPW_BD_FLAG_TYPE_802_3				0x00
#define	IPW_BD_FLAG_NOT_LAST_FRAGMENT		0x01
#define	IPW_BD_FLAG_COMMAND					0x02
#define	IPW_BD_FLAG_TYPE_802_11				0x04
#define	IPW_BD_FLAG_INTERRUPT				0x08

// status descriptor
struct ipw_status {
	uint32	length;
	uint16	code;
	uint8	flags;
	uint8	rssi; // received signal strength indicator
} _PACKED;

#define	IPW_STATUS_CODE_COMMAND			0
#define	IPW_STATUS_CODE_STATUS			1
#define	IPW_STATUS_CODE_DATA_802_11		2
#define	IPW_STATUS_CODE_DATA_802_3		3
#define	IPW_STATUS_CODE_NOTIFICATION	4
#define IPW_STATUS_CODE_MASK			0x0f

#define	IPW_STATUS_FLAG_DECRYPTED		0x01
#define	IPW_STATUS_FLAG_WEP_ENCRYPTED	0x02

// adapter states
#define IPW_STATE_INITIALIZED			0x00000001
#define IPW_STATE_COUNTRY_FOUND			0x00000002
#define	IPW_STATE_ASSOCIATED			0x00000004
#define IPW_STATE_ASSOCIATION_LOST		0x00000008
#define IPW_STATE_ASSOCIATION_CHANGED	0x00000010
#define IPW_STATE_SCAN_COMPLETE			0x00000020
#define IPW_STATE_PSP_ENTERED			0x00000040
#define IPW_STATE_PSP_LEFT				0x00000080
#define IPW_STATE_RF_KILL				0x00000100
#define IPW_STATE_DISABLED				0x00000200
#define IPW_STATE_POWER_DOWN			0x00000400
#define IPW_STATE_SCANNING				0x00000800

// data descriptor
struct ipw_data {
	uint32	command;
	uint32	unused;
	uint8	encrypted;
	uint8	needs_encryption;
	uint8	key_index;
	uint8	key_size;
	uint8	key[16];
	uint8	reserved[10];
	uint8	source_address[6];
	uint8	dest_address[6];
	uint16	fragment_size;
} _PACKED;

// data holders
struct ipw_tx {
	uint8 data[2500];
} _PACKED;

struct ipw_rx {
	uint8 data[2500];
} _PACKED;

// command descriptor
struct ipw_command {
	uint32	command;
	uint64	unused;
	uint32	length;
	uint8	data[400];
	uint32	status;
	uint8	reserved[68];
} _PACKED;

#define	IPW_COMMAND_ENABLE					2
#define	IPW_COMMAND_SET_CONFIGURATION		6
#define	IPW_COMMAND_SET_ESSID				8
#define	IPW_COMMAND_SET_MANDATORY_BSSID		9
#define	IPW_COMMAND_SET_MAC_ADDRESS			11
#define	IPW_COMMAND_SET_MODE				12
#define	IPW_COMMAND_SET_CHANNEL				14
#define	IPW_COMMAND_SET_RTS_THRESHOLD		15
#define	IPW_COMMAND_SET_FRAG_THRESHOLD		16
#define	IPW_COMMAND_SET_POWER_MODE			17
#define	IPW_COMMAND_SET_TX_RATES			18
#define	IPW_COMMAND_SET_BASIC_TX_RATES		19
#define	IPW_COMMAND_SET_WEP_KEY				20
#define	IPW_COMMAND_SET_WEP_KEY_INDEX		25
#define	IPW_COMMAND_SET_WEP_FLAGS			26
#define	IPW_COMMAND_ADD_MULTICAST			27
#define	IPW_COMMAND_SET_BEACON_INTERVAL		29
#define IPW_COMMAND_SEND_DATA				33
#define	IPW_COMMAND_SET_TX_POWER_INDEX		36
#define	IPW_COMMAND_BROADCAST_SCAN			43
#define	IPW_COMMAND_DISABLE					44
#define	IPW_COMMAND_SET_DESIRED_BSSID		45
#define	IPW_COMMAND_SET_SCAN_OPTIONS		46
#define	IPW_COMMAND_PREPARE_POWER_DOWN		58
#define	IPW_COMMAND_DISABLE_PHY				61
#define IPW_COMMAND_SET_MSDU_TX_RATES		62
#define	IPW_COMMAND_SET_SECURITY_INFO		67
#define	IPW_COMMAND_SET_WPA_IE				69

// values for IPW_COMMAND_SET_POWER_MODE
#define	IPW_POWER_MODE_CAM					0
#define	IPW_POWER_MODE_AUTOMATIC			6

// values for IPW_COMMAND_SET_MODE
#define	IPW_MODE_BSS						1
#define	IPW_MODE_MONITOR					2
#define	IPW_MODE_IBSS						3

// values for IPW_COMMAND_SET_WEP_FLAGS
#define	IPW_WEP_FLAGS_HW_DECRYPT	0x00000001
#define IPW_WEP_FLAGS_HW_ENCRYPT	0x00000008

// structure for IPW_COMMAND_SET_WEP_KEY
struct ipw_wep_key {
	uint8	index;
	uint8	length;
	uint8	key[13];
} _PACKED;

// structure for IPW_COMMAND_SET_SECURITY_INFO
struct ipw_security {
	uint32	ciphers;
	uint16	reserved1;
	uint8	auth_mode;
	uint16	reserved2;
} _PACKED;

#define	IPW_CIPHER_NONE				0x00000001
#define	IPW_CIPHER_WEP40			0x00000002
#define	IPW_CIPHER_TKIP				0x00000004
#define	IPW_CIPHER_CCMP				0x00000010
#define	IPW_CIPHER_WEP104			0x00000020
#define	IPW_CIPHER_CKIP				0x00000040

#define	IPW_AUTH_MODE_OPEN			0
#define	IPW_AUTH_MODE_SHARED		1

// structure for IPW_COMMAND_SET_SCAN_OPTIONS
struct ipw_scan_options {
	uint32	flags;
	uint32	channels;
} _PACKED;

#define	IPW_SCAN_DO_NOT_ASSOCIATE	0x00000001
#define IPW_SCAN_MIXED_CELL			0x00000002
#define	IPW_SCAN_PASSIVE			0x00000008

// structure for IPW_COMMAND_SET_CONFIGURATION
struct ipw_configuration {
	uint32	flags;
	uint32	bss_channel_mask;
	uint32	ibss_channel_mask;
} _PACKED;

#define	IPW_CONFIG_PROMISCUOUS		0x00000004
#define	IPW_CONFIG_PREAMBLE_AUTO	0x00000010
#define	IPW_CONFIG_IBSS_AUTO_START	0x00000020
#define	IPW_CONFIG_802_1X_ENABLE	0x00004000
#define	IPW_CONFIG_BSS_MASK			0x00008000
#define	IPW_CONFIG_IBSS_MASK		0x00010000

// default values
#define IPW_BSS_CHANNEL_MASK		0x000003fff
#define IPW_IBSS_CHANNEL_MASK		0x0000087ff
#define IPW_DEFAULT_BEACON_INTERVAL	100
#define IPW_DEFAULT_TX_POWER		32

// structure for IPW_COMMAND_SET_WPA_IE
struct wpa_ie {
	uint8	id;
	uint8	length;
	uint8	oui[3];
	uint8	oui_type;
	uint16	version;
	uint32	multicast_cipher;
	uint16	unicast_cipher_count;
	uint32	unicast_ciphers[8];
	uint16	auth_selector_count;
	uint32	auth_selectors[8];
	uint16	capabilities;
	uint16	pmkid_count;
	uint16	pmkids[8];
} _PACKED;

struct ipw_wpa_ie {
	uint16	mask;
	uint16	capability_info;
	uint16	lintval;
	uint8	bssid[6];
	uint32	length;
	wpa_ie	wpa;
} _PACKED;	

// bitmask for IPW_COMMAND_SET_[BASIC|MSDU]_TX_RATES
#define IPW_TX_RATE_1_MBIT			0x00000001
#define IPW_TX_RATE_2_MBIT			0x00000002
#define IPW_TX_RATE_5_5_MBIT		0x00000004
#define IPW_TX_RATE_11_MBIT			0x00000008
#define IPW_TX_RATE_ALL				0x0000000f

// values for IPW_COMMAND_SET_RTS_THRESHOLD
#define IPW_RTS_THRESHOLD_MIN		1
#define IPW_RTS_THRESHOLD_MAX		2304
#define IPW_RTS_THRESHOLD_DEFAULT	1000

// buffers
#define	IPW_TX_BUFFER_COUNT			128
#define	IPW_TX_BUFFER_SIZE			(IPW_TX_BUFFER_COUNT * sizeof(ipw_bd))
#define	IPW_TX_PACKET_SIZE			(IPW_TX_BUFFER_COUNT * sizeof(ipw_tx))
#define	IPW_RX_BUFFER_COUNT			128
#define	IPW_RX_BUFFER_SIZE			(IPW_RX_BUFFER_COUNT * sizeof(ipw_bd))
#define IPW_RX_PACKET_SIZE			(IPW_RX_BUFFER_COUNT * sizeof(ipw_rx))
#define	IPW_STATUS_BUFFER_COUNT		IPW_RX_BUFFER_COUNT
#define	IPW_STATUS_BUFFER_SIZE		(IPW_STATUS_BUFFER_COUNT * sizeof(ipw_status))

// registers
#define IPW_REG_INTERRUPT			0x0008
#define	IPW_REG_INTERRUPT_MASK		0x000c
#define	IPW_REG_INDIRECT_ADDRESS	0x0010
#define	IPW_REG_INDIRECT_DATA		0x0014
#define	IPW_REG_RESET				0x0020
#define	IPW_REG_CONTROL				0x0024
#define IPW_REG_IO					0x0030
#define	IPW_REG_DEBUG				0x0090
#define IPW_REG_TX_BASE				0x0200
#define IPW_REG_TX_SIZE				0x0204
#define IPW_REG_TX_READ				0x0280
#define IPW_REG_TX_WRITE			0x0f80
#define IPW_REG_RX_BASE				0x0240
#define IPW_REG_RX_SIZE				0x0248
#define IPW_REG_RX_READ				0x02a0
#define IPW_REG_RX_WRITE			0x0fa0
#define IPW_REG_STATUS_BASE			0x0244

#define IPW_INDIRECT_ADDRESS_MASK	0x00fffffc

// flags for IPW_REG_INTERRUPT(_MASK)
#define IPW_INTERRUPT_TX_TRANSFER	0x00000001
#define IPW_INTERRUPT_RX_TRANSFER	0x00000002
#define IPW_INTERRUPT_STATUS_CHANGE	0x00000010
#define IPW_INTERRUPT_COMMAND_DONE	0x00010000
#define IPW_INTERRUPT_FW_INIT_DONE	0x01000000
#define IPW_INTERRUPT_FATAL_ERROR	0x40000000
#define IPW_INTERRUPT_PARITY_ERROR	0x80000000

// flags for IPW_REG_RESET
#define	IPW_RESET_PRINCETON_RESET	0x00000001
#define	IPW_RESET_SW_RESET			0x00000080
#define	IPW_RESET_MASTER_DISABLED	0x00000100
#define	IPW_RESET_STOP_MASTER		0x00000200

// flags for IPW_REG_CONTROL
#define	IPW_CONTROL_CLOCK_READY		0x00000001
#define IPW_CONTROL_ALLOW_STANDBY	0x00000002
#define	IPW_CONTROL_INIT_COMPLETE	0x00000004

// flags for IPW_REG_IO
#define IPW_IO_GPIO1_ENABLE			0x00000008
#define IPW_IO_GPIO1_MASK			0x0000000c
#define IPW_IO_GPIO3_MASK			0x000000c0
#define IPW_IO_LED_OFF				0x00002000
#define IPW_IO_RADIO_DISABLED		0x00010000

// the value at the debug register base is always
// the magic data value. used to verify register access.
#define	IPW_DEBUG_DATA				0xd55555d5

// shared memory
#define IPW_SHARED_MEMORY_BASE_0	0x0002f200
#define IPW_SHARED_MEMORY_LENGTH_0	784
#define IPW_SHARED_MEMORY_BASE_1	0x0002f610
#define IPW_SHARED_MEMORY_LENGTH_1	32
#define IPW_SHARED_MEMORY_BASE_2	0x0002fa00
#define IPW_SHARED_MEMORY_LENGTH_2	32
#define IPW_SHARED_MEMORY_BASE_3	0x0002fc00
#define IPW_SHARED_MEMORY_LENGTH_3	16
#define IPW_INTERRUPT_MEMORY_BASE	0x0002ff80
#define IPW_INTERRUPT_MEMORY_LENGTH	128

// ordinal tables
#define IPW_ORDINAL_TABLE_1_ADDRESS	0x00000380
#define IPW_ORDINAL_TABLE_2_ADDRESS	0x00000384
#define IPW_ORDINAL_TABLE_1_START	1
#define IPW_ORDINAL_TABLE_2_START	1000

enum ipw_ordinal_table_1 {
	IPW_ORD_FIRMWARE_DB_LOCK = 120,
	IPW_ORD_CARD_DISABLED = 157,
	IPW_ORD_GET_AP_BSSID = 1014,
};

// firmware db lock
#define IPW_FIRMWARE_LOCK_NONE		0
#define IPW_FIRMWARE_LOCK_DRIVER	1
#define IPW_FIRMWARE_LOCK_FIRMWARE	2

// firmware binary image header
struct ipw_firmware_header {
	uint16	version;
	uint16	mode;
	uint32	main_size;
	uint32	ucode_size;
} _PACKED;

// symbol alive response to check microcode
struct symbol_alive_response {
	uint8	command_id;
	uint8	sequence_number;
	uint8	microcode_revision;
	uint8	eeprom_valid;
	uint16	valid_flags;
	uint8	ieee_address[6];
	uint16	flags;
	uint16	pcb_revision;
	uint16	clock_settle_time;
	uint16	powerup_settle_time;
	uint16	hop_settle_time;
	uint8	date[3];
	uint8	time[2];
	uint8	microcode_valid;
} _PACKED;

#endif // _IPW2100_HW_H_
