#ifndef __HARDWARE_H
#define __HARDWARE_H

#define PCI_PCICMD_IOS	0x01
#define PCI_PCICMD_MSE	0x02
#define PCI_PCICMD_BME	0x04


typedef struct {
	uint32 stat_len;
	uint32 vlan;
	uint32 buf_low;
	uint32 buf_high;
} _PACKED tx_desc;

enum {
	TX_DESC_OWN			= 0x80000000,
	TX_DESC_EOR			= 0x40000000,
	TX_DESC_FS			= 0x20000000,
	TX_DESC_LS			= 0x10000000,
	TX_DESC_LEN_MASK	= 0x00007fff,
};

typedef struct {
	uint32 stat_len;
	uint32 vlan;
	uint32 buf_low;		// must be 8 byte aligned
	uint32 buf_high;
} _PACKED rx_desc;

enum {
	RX_DESC_OWN			= 0x80000000,
	RX_DESC_EOR			= 0x40000000,
	RX_DESC_FS			= 0x20000000,
	RX_DESC_LS			= 0x10000000,
	RX_DESC_MAR			= 0x08000000,
	RX_DESC_PAM			= 0x04000000,
	RX_DESC_BAR			= 0x02000000,
	RX_DESC_BOVF		= 0x01000000,
	RX_DESC_FOVF		= 0x00800000,
	RX_DESC_RWT			= 0x00400000,
	RX_DESC_RES			= 0x00200000,
	RX_DESC_RUNT		= 0x00100000,
	RX_DESC_CRC			= 0x00080000,
	RX_DESC_LEN_MASK	= 0x00007fff,
};

enum {
	REG_TNPDS_LOW		= 0x20,
	REG_TNPDS_HIGH		= 0x24,
	REG_THPDS_LOW		= 0x28,
	REG_THPDS_HIGH		= 0x2c,
	REG_RDSAR_LOW		= 0xe4,
	REG_RDSAR_HIGH		= 0xe8,

	REG_CR				= 0x37,
};

enum {
	REG_CR_RST	= 0x10,
	REG_CR_RE	= 0x08,
	REG_CR_TE	= 0x04,
};

#endif
