/*
 * Copyright (c) 2001-2007 pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef SIS900_H
#define SIS900_H

/*! SiS 900 driver/chip specific definitions */

#include <ether_driver.h>

#include <KernelExport.h>
#include <PCI.h>


#define VENDOR_ID_SiS		0x1039
#define DEVICE_ID_SiS900	0x900

#define DEVICE_ID_SiS_ISA_BRIDGE 0x0008

enum SiS_revisions {
	SiS900_REVISION_B			= 0x03,
	SiS900_REVISION_SiS630A		= 0x80,
	SiS900_REVISION_SiS630E		= 0x81,
	SiS900_REVISION_SiS630S		= 0x82,
	SiS900_REVISION_SiS630EA1	= 0x83,
	SiS900_REVISION_SiS630ET	= 0x84,
	SiS900_REVISION_SiS635A		= 0x90,
	SiS900_REVISION_SiS96x		= 0x91,
};

#define DEVICE_NAME "sis900"
#define DEVICE_DRIVERNAME "net/" DEVICE_NAME


/***************************** Buffer & Buffer Descriptors *****************************/

struct buffer_desc {
	uint32	link;
	uint32	status;
	uint32	buffer;
};

enum SiS900_buffer_desc_bits {
	SiS900_DESCR_OWN			= 0x80000000,
	SiS900_DESCR_MORE			= 0x40000000,
	SiS900_DESCR_INTR			= 0x20000000,
	SiS900_DESCR_SUPPRESS_CRC	= 0x10000000,
	SiS900_DESCR_OK				= 0x08000000,
	
	SiS900_DESCR_Tx_ABORT		= 0x04000000,
	SiS900_DESCR_Tx_UNDERRUN	= 0x02000000,
	SiS900_DESCR_Tx_CRS			= 0x01000000,
	SiS900_DESCR_Tx_DEFERRED	= 0x00800000,
	SiS900_DESCR_Tx_EXC_DEFERRED	= 0x00400000,
	SiS900_DESCR_Tx_OOW_COLLISION	= 0x00200000,
	SiS900_DESCR_Tx_EXC_COLLISIONS	= 0x00100000,

	SiS900_DESCR_Rx_ABORT		= 0x04000000,
	SiS900_DESCR_Rx_OVERRUN		= 0x02000000,
	SiS900_DESCR_Rx_DESTCLASS0	= 0x01000000,
	SiS900_DESCR_Rx_DESTCLASS1	= 0x00800000,
	SiS900_DESCR_Rx_LONG_PACKET	= 0x00400000,
	SiS900_DESCR_Rx_RUNT_PACKET	= 0x00200000,
	SiS900_DESCR_Rx_INVALID_SYM	= 0x00100000,
	SiS900_DESCR_Rx_CRC_ERROR	= 0x00080000,
	SiS900_DESCR_Rx_FRAME_ALIGN	= 0x00040000,
	SiS900_DESCR_Rx_LOOPBACK	= 0x00020000,
	SiS900_DESCR_Rx_COLLISION	= 0x00010000,
};

#define SiS900_DESCR_SIZE_MASK 0x0fff

#define NUM_Rx_DESCR 32
#define NUM_Tx_DESCR 16

#define NUM_Rx_MASK (NUM_Rx_DESCR - 1)
#define NUM_Tx_MASK (NUM_Tx_DESCR - 1)


#define MAX_FRAME_SIZE	1514
#define BUFFER_SIZE	2048
#define CRC_SIZE 4


/***************************** Private Device Data *****************************/

struct sis_info {
	timer			timer;
	uint32			cookieMagic;
	area_id			thisArea;
	uint32			id;
	spinlock		lock;
	pci_info		*pciInfo;
	volatile addr_t	registers;
	ether_address_t	address;
	volatile int32	blockFlag;

	struct mii_phy	*firstPHY;
	struct mii_phy	*currentPHY;
	uint16			phy;
	bool			autoNegotiationComplete;
	bool			link;
	bool			full_duplex;
	uint16			speed;
	uint16			fixedMode;

	// receive data
	volatile struct buffer_desc rxDescriptor[NUM_Rx_DESCR];
	volatile char	*rxBuffer[NUM_Rx_DESCR];
	area_id			rxArea;
	sem_id			rxSem;
	spinlock		rxSpinlock;
	int32			rxLock;
	int16			rxCurrent;
	int16			rxInterruptIndex;
	int16			rxFree;

	// transmit data
	volatile struct buffer_desc txDescriptor[NUM_Tx_DESCR];
	volatile char	*txBuffer[NUM_Tx_DESCR];
	area_id			txArea;
	sem_id			txSem;
	spinlock		txSpinlock;
	int32			txLock;
	int16			txCurrent;
	int16			txInterruptIndex;
	int16			txSent;
};

#define SiS_COOKIE_MAGIC 's900'
#define SiS_FREE_COOKIE_MAGIC 's9fr'

enum link_modes {
	LINK_HALF_DUPLEX	= 0x0100,
	LINK_FULL_DUPLEX	= 0x0200,
	LINK_DUPLEX_MASK	= 0xff00,

	LINK_SPEED_HOME		= 1,
	LINK_SPEED_10_MBIT	= 10,
	LINK_SPEED_100_MBIT	= 100,
	LINK_SPEED_DEFAULT	= LINK_SPEED_100_MBIT,
	LINK_SPEED_MASK		= 0x00ff
};

/***************************** Media Access Control *****************************/

enum SiS900_MAC_registers {
	SiS900_MAC_COMMAND			= 0x00,
	SiS900_MAC_CONFIG			= 0x04,
	SiS900_MAC_EEPROM_ACCESS	= 0x08,
	SiS900_MAC_PCI_TEST			= 0x0c,
	SiS900_MAC_INTR_STATUS		= 0x10,
	SiS900_MAC_INTR_MASK		= 0x14,
	SiS900_MAC_INTR_ENABLE		= 0x18,
	SiS900_MAC_PHY_ACCESS		= 0x1c,
	SiS900_MAC_Tx_DESCR			= 0x20,
	SiS900_MAC_Tx_CONFIG		= 0x24,
	/* reserved 0x28 - 0x2c */
	SiS900_MAC_Rx_DESCR			= 0x30,
	SiS900_MAC_Rx_CONFIG		= 0x34,
	SiS900_MAC_FLOW_CONTROL		= 0x38,
	/* reserved 0x3c - 0x44 */
	SiS900_MAC_Rx_FILTER_CONTROL	= 0x48,
	SiS900_MAC_Rx_FILTER_DATA	= 0x4c,
	/* reserved 0x50 - 0xac */
	SiS900_MAC_WAKEUP_CONTROL	= 0xb0,
	SiS900_MAC_WAKEUP_EVENT		= 0xb4
	/* reserved, wake-up registers */
};

enum SiS900_MAC_commands {
	SiS900_MAC_CMD_RESET		= 0x0100,
	SiS900_MAC_CMD_Rx_RESET		= 0x0020,
	SiS900_MAC_CMD_Tx_RESET		= 0x0010,
	SiS900_MAC_CMD_Rx_DISABLE	= 0x0008,
	SiS900_MAC_CMD_Rx_ENABLE	= 0x0004,
	SiS900_MAC_CMD_Tx_DISABLE	= 0x0002,
	SiS900_MAC_CMD_Tx_ENABLE	= 0x0001
};

enum SiS900_MAC_configuration_bits {
	SiS900_MAC_CONFIG_BIG_ENDIAN			= 0x00000001,
	SiS900_MAC_CONFIG_EXCESSIVE_DEFERRAL	= 0x00000010,
	SiS900_MAC_CONFIG_EDB_MASTER			= 0x00002000,
};

enum SiS900_MAC_rxfilter {
	SiS900_RxF_ENABLE				= 0x80000000,
	SiS900_RxF_ACCEPT_ALL_BROADCAST	= 0x40000000,
	SiS900_RxF_ACCEPT_ALL_MULTICAST	= 0x20000000,
	SiS900_RxF_ACCEPT_ALL_ADDRESSES	= 0x10000000
};

#define SiS900_Rx_FILTER_ADDRESS_SHIFT 16

enum SiS900_MAC_txconfig {
	SiS900_Tx_CS_IGNORE		= 0x80000000,
	SiS900_Tx_HB_IGNORE		= 0x40000000,
	SiS900_Tx_MAC_LOOPBACK	= 0x20000000,
	SiS900_Tx_AUTO_PADDING	= 0x10000000
};

#define SiS900_DMA_SHIFT	20
#define SiS900_Tx_FILL_THRES			(16 << 8)	/* 1/4 of FIFO */
#define SiS900_Tx_100_MBIT_DRAIN_THRES	48			/* 3/4 */
#define SiS900_Tx_10_MBIT_DRAIN_THRES	16	//	32	/* 1/2 */

enum SiS900_MAC_rxconfig {
	SiS900_Rx_ACCEPT_ERROR_PACKETS	= 0x80000000,
	SiS900_Rx_ACCEPT_RUNT_PACKETS	= 0x40000000,
	SiS900_Rx_ACCEPT_Tx_PACKETS		= 0x10000000,
	SiS900_Rx_ACCEPT_JABBER_PACKETS	= 0x08000000
};

#define SiS900_Rx_100_MBIT_DRAIN_THRES	32			/* 1/2 of FIFO */
#define SiS900_Rx_10_MBIT_DRAIN_THRES	48			/* 3/4 */

enum SiS900_MAC_wakeup {
	SiS900_WAKEUP_LINK_ON	= 0x02,
	SiS900_WAKEUP_LINK_LOSS	= 0x01
};

enum SiS900_MAC_interrupts {
	SiS900_INTR_WAKEUP_EVENT		= 0x10000000,
	SiS900_INTR_Tx_RESET_COMPLETE	= 0x02000000,
	SiS900_INTR_Rx_RESET_COMPLETE	= 0x01000000,
	SiS900_INTR_Rx_STATUS_OVERRUN	= 0x00010000,
	SiS900_INTR_Tx_UNDERRUN			= 0x00000400,
	SiS900_INTR_Tx_IDLE				= 0x00000200,
	SiS900_INTR_Tx_ERROR			= 0x00000100,
	SiS900_INTR_Tx_DESCRIPTION		= 0x00000080,
	SiS900_INTR_Tx_OK				= 0x00000040,
	SiS900_INTR_Rx_OVERRUN			= 0x00000020,
	SiS900_INTR_Rx_IDLE				= 0x00000010,
	SiS900_INTR_Rx_EARLY_THRESHOLD	= 0x00000008,
	SiS900_INTR_Rx_ERROR			= 0x00000004,
	SiS900_INTR_Rx_DESCRIPTION		= 0x00000002,
	SiS900_INTR_Rx_OK				= 0x00000001
};

/***************************** EEPROM *****************************/

enum SiS900_EEPROM_bits {
	SiS900_EEPROM_SELECT	= 0x08,
	SiS900_EEPROM_CLOCK		= 0x04,
	SiS900_EEPROM_DATA_OUT	= 0x02,
	SiS900_EEPROM_DATA_IN	= 0x01
};

// the EEPROM definitions are taken from linux' sis900 driver header
enum SiS900_EEPROM_address {
	SiS900_EEPROM_SIGNATURE		= 0x00,
	SiS900_EEPROM_VENDOR_ID		= 0x02,
	SiS900_EEPROM_DEVICE_ID		= 0x03,
	SiS900_EEPROM_MAC_ADDRESS	= 0x08,
	SiS900_EEPROM_CHECKSUM		= 0x0b
};

enum SiS900_EEPROM_commands {
	SiS900_EEPROM_CMD_READ = 0x0180
};

enum SiS96x_EEPROM_command {
	SiS96x_EEPROM_CMD_REQ	= 0x00000400, 
	SiS96x_EEPROM_CMD_DONE	= 0x00000200, 
	SiS96x_EEPROM_CMD_GRANT	= 0x00000100
};

/***************************** Media Independent Interface *****************************/

struct mii_phy {
	struct mii_phy *next;
	uint16	id0,id1;
	uint16	address;
	uint8	types;
};

enum SiS900_MII_bits {
//	SiS900_MII_WRITE	= 0x00,
//	SiS900_MII_READ		= 0x20,
//	SiS900_MII_ACCESS	= 0x10
	SiS900_MII_MDC		= 0x40,
	SiS900_MII_MDDIR	= 0x20,
	SiS900_MII_MDIO		= 0x10,
};

enum SiS900_MII_address {
	// standard registers
	MII_CONTROL		= 0x00,
	MII_STATUS		= 0x01,
	MII_PHY_ID0		= 0x02,
	MII_PHY_ID1		= 0x03,
	MII_AUTONEG_ADV				= 0x04,
	MII_AUTONEG_LINK_PARTNER	= 0x05,
	MII_AUTONEG_EXT				= 0x06,

	// SiS900 specific registers
	MII_CONFIG1		= 0x10,
	MII_CONFIG2		= 0x11,
	MII_LINK_STATUS	= 0x12,
	MII_MASK		= 0x13,
	MII_RESERVED	= 0x14
};

enum MII_control {
	MII_CONTROL_RESET			= 0x8000,
	MII_CONTROL_RESET_AUTONEG	= 0x0200,
	MII_CONTROL_AUTO			= 0x1000,
	MII_CONTROL_FULL_DUPLEX		= 0x0100,
	MII_CONTROL_ISOLATE			= 0x0400
};

enum SiS900_MII_commands {
	MII_CMD_READ		= 0x6000,
	MII_CMD_WRITE		= 0x5002,

	MII_PHY_SHIFT		= 7,
	MII_REG_SHIFT		= 2,
};

enum MII_status_bits {
	MII_STATUS_EXT			= 0x0001,
	MII_STATUS_JAB			= 0x0002,
	MII_STATUS_LINK			= 0x0004,
	MII_STATUS_CAN_AUTO		= 0x0008,
	MII_STATUS_FAULT		= 0x0010,
	MII_STATUS_AUTO_DONE	= 0x0020,
	MII_STATUS_CAN_T		= 0x0800,
	MII_STATUS_CAN_T_FDX	= 0x1000,
	MII_STATUS_CAN_TX		= 0x2000,
	MII_STATUS_CAN_TX_FDX	= 0x4000,
	MII_STATUS_CAN_T4		= 0x8000
};

enum MII_auto_negotiation {
	// taken from the linux sis900.h header file
	MII_NWAY_NODE_SEL	= 0x001f,
	MII_NWAY_CSMA_CD	= 0x0001,
	MII_NWAY_T			= 0x0020,
	MII_NWAY_T_FDX		= 0x0040,
	MII_NWAY_TX			= 0x0080,
	MII_NWAY_TX_FDX		= 0x0100,
	MII_NWAY_T4			= 0x0200,
	MII_NWAY_PAUSE		= 0x0400,
	MII_NWAY_RF			= 0x2000,
	MII_NWAY_ACK		= 0x4000,
	MII_NWAY_NP			= 0x8000
};


enum SiS900_MII_link_status {
	MII_LINK_FAIL			= 0x4000,
	MII_LINK_100_MBIT		= 0x0080,
	MII_LINK_FULL_DUPLEX	= 0x0040
};

#define SiS900_MII_REGISTER_SHIFT	6
#define SiS900_MII_DATA_SHIFT		16


/***************************** Misc. & Prototypes *****************************/

#define HACK(x) ;

#ifdef EXCESSIVE_DEBUG
#	define TRACE(x) dprintf x
	extern int bug(const char *, ...);
#elif defined(DEBUG)
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define ROUND_TO_PAGE_SIZE(x) (((x) + (B_PAGE_SIZE) - 1) & ~((B_PAGE_SIZE) - 1))

// chip specific network functions
extern int32 sis900_interrupt(void *data);
extern void	sis900_disableInterrupts(struct sis_info *info);
extern void	sis900_enableInterrupts(struct sis_info *info);

extern int32 sis900_timer(timer *t);
extern status_t sis900_initPHYs(struct sis_info *info);
extern void sis900_checkMode(struct sis_info *info);
extern bool sis900_getMACAddress(struct sis_info *info);
extern status_t sis900_reset(struct sis_info *info);

extern void sis900_setPromiscuous(struct sis_info *info, bool on);
extern void sis900_setRxFilter(struct sis_info *info);

extern void sis900_deleteRings(struct sis_info *info);
extern status_t sis900_createRings(struct sis_info *info);


#endif /* SIS900_H */
