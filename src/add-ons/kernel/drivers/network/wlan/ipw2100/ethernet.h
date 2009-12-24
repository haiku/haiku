#ifndef _ETHERNET_H_
#define _ETHERNET_H_

#include <Drivers.h>

enum {
	ETHER_GETADDR = B_DEVICE_OP_CODES_END,	/* get ethernet address */
	ETHER_INIT,								/* set irq and port */
	ETHER_NONBLOCK,							/* set/unset nonblocking mode */
	ETHER_ADDMULTI,							/* add multicast addr */
	ETHER_REMMULTI,							/* rem multicast addr */
	ETHER_SETPROMISC,						/* set promiscuous */
	ETHER_GETFRAMESIZE,						/* get frame size */
	ETHER_ADDTIMESTAMP,						/* (try to) add timestamps to packets (BONE ext) */
	ETHER_HASIOVECS,						/* does the driver implement writev ? (BONE ext) (bool *) */
	ETHER_GETIFTYPE,						/* get the IFT_ type of the interface (int *) */
	ETHER_GETLINKSTATE						/* get line speed, quality, duplex mode, etc. */
};

// ethernet data
#define ETHER_ADDRESS_SIZE	6

struct ethernet_header {
	uint8	dest_address[ETHER_ADDRESS_SIZE];
	uint8	source_address[ETHER_ADDRESS_SIZE];
	uint16	ether_type;
	uint8	data[0];
} _PACKED;


struct llc_header {
	uint8	dsap;
	uint8	ssap;
	uint8	control;
	uint8	org_code[3];
	uint16	ether_type;
} _PACKED;

#define LLC_LSAP_SNAP	0xaa
#define LLC_CONTROL_UI	0x03


struct ieee80211_header {
	uint8		frame_control[2];
	uint8		duration[2];
	uint8		address1[ETHER_ADDRESS_SIZE];
	uint8		address2[ETHER_ADDRESS_SIZE];
	uint8		address3[ETHER_ADDRESS_SIZE];
	uint8		sequence[2];
	uint8		data[0];
} _PACKED;

// flags for frame_control[0]
#define IEEE80211_DATA_FRAME	0x08

// flags for frame_control[1]
#define IEEE80211_WEP_ENCRYPTED	0x40

#endif // _ETHERNET_H_
