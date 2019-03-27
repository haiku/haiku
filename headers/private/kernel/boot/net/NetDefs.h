/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2010, Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BOOT_NET_DEFS_H
#define _BOOT_NET_DEFS_H


#include <string.h>

#include <ByteOrder.h>
#include <SupportDefs.h>

#include <util/kernel_cpp.h>


// Network endianness
#ifndef htonl
#	define htonl(x) B_HOST_TO_BENDIAN_INT32(x)
#	define ntohl(x) B_BENDIAN_TO_HOST_INT32(x)
#	define htons(x) B_HOST_TO_BENDIAN_INT16(x)
#	define ntohs(x) B_BENDIAN_TO_HOST_INT16(x)
#endif


// Ethernet

#define ETH_ALEN			6
#define ETHERTYPE_IP		0x0800	// IP
#define ETHERTYPE_ARP		0x0806	// Address resolution

#define ETHER_MIN_TRANSFER_UNIT	46
#define ETHER_MAX_TRANSFER_UNIT	1500

struct mac_addr_t {
	mac_addr_t() {}

	mac_addr_t(uint8 *address)
	{
		memcpy(this->address, address, ETH_ALEN);
	}

	mac_addr_t(const mac_addr_t& other)
	{
		memcpy(address, other.address, sizeof(address));
	}

	uint64 ToUInt64() const
	{
		return ((uint64)address[0] << 40)
			| ((uint64)address[1] << 32)
			| ((uint64)address[2] << 24)
			| ((uint64)address[3] << 16)
			| ((uint64)address[4] << 8)
			| (uint64)address[5];
	}

	uint8 operator[](int index)
	{
		return address[index];
	}

	mac_addr_t& operator=(const mac_addr_t& other)
	{
		memcpy(address, other.address, sizeof(address));
		return *this;
	}

	bool operator==(const mac_addr_t& other) const
	{
		return memcmp(address, other.address, sizeof(address)) == 0;
	}

	bool operator!=(const mac_addr_t& other) const
	{
		return !(*this == other);
	}

	uint8 address[ETH_ALEN];
} __attribute__ ((__packed__));

extern const mac_addr_t kBroadcastMACAddress;
extern const mac_addr_t kNoMACAddress;

// 10/100 Mb/s ethernet header
struct ether_header {
	mac_addr_t	destination;	/* destination eth addr */
	mac_addr_t	source;			/* source ether addr    */
	uint16		type;			/* packet type ID field */
} __attribute__ ((__packed__));


// #pragma mark -

// Address Resolution Protocol (ARP)

typedef uint32 ip_addr_t;

// ARP protocol opcodes
#define ARPOP_REQUEST   1               /* ARP request.  */
#define ARPOP_REPLY     2               /* ARP reply.  */
#define ARPOP_RREQUEST  3               /* RARP request.  */
#define ARPOP_RREPLY    4               /* RARP reply.  */
#define ARPOP_InREQUEST 8               /* InARP request.  */
#define ARPOP_InREPLY   9               /* InARP reply.  */
#define ARPOP_NAK       10              /* (ATM)ARP NAK.  */

// ARP header for IP over ethernet (RFC 826)
struct arp_header {
	uint16	hardware_format;	/* Format of hardware address.  */
	uint16	protocol_format;	/* Format of protocol address.  */
	uint8	hardware_length;	/* Length of hardware address.  */
	uint8	protocol_length;	/* Length of protocol address.  */
	uint16	opcode;				/* ARP opcode (command).  */

	// IP over ethernet
	mac_addr_t	sender_mac;		/* Sender hardware address.  */
    ip_addr_t	sender_ip;		/* Sender IP address.  */
	mac_addr_t	target_mac;		/* Target hardware address.  */
    ip_addr_t	target_ip;		/* Target IP address.  */
} __attribute__ ((__packed__));

// ARP protocol HARDWARE identifiers.
#define ARPHRD_ETHER    1               /* Ethernet 10/100Mbps.  */


// #pragma mark -

// Internet Protocol (IP)

#define INADDR_ANY              ((ip_addr_t) 0x00000000)
	/* Address to send to all hosts.  */
#define INADDR_BROADCAST        ((ip_addr_t) 0xffffffff)
	/* Address indicating an error return.  */
#define INADDR_NONE             ((ip_addr_t) 0xffffffff)

// IP packet header (no options
struct ip_header {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	uint8 		header_length:4;	// header length
	uint8		version:4;			// IP protocol version
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
	uint8		version:4;			// IP protocol version
	uint8 		header_length:4;	// header length
#endif
	uint8		type_of_service;	// type of service
	uint16		total_length;		// total IP packet length
	uint16		identifier;			// fragment identification
	uint16		fragment_offset;	// fragment offset and flags (0xe000)
	uint8		time_to_live;		// time to live
	uint8		protocol;			// protocol
	uint16		checksum;			// checksum (header)
	ip_addr_t	source;				// source IP address
	ip_addr_t	destination;		// destination IP address
} __attribute__ ((__packed__));

// IP protocol version 4
#define	IP_PROTOCOL_VERSION_4		4

// fragment flags/offset mask
#define IP_DONT_FRAGMENT			0x4000	/* dont fragment flag */
#define IP_FRAGMENT_OFFSET_MASK		0x1fff	/* mask for fragment offset */

// Internet implementation parameters.
#define IP_MAX_TIME_TO_LIVE			255		/* maximum time to live */
#define IP_DEFAULT_TIME_TO_LIVE		64		/* default ttl, from RFC 1340 */

// IP protocols
#define IPPROTO_TCP					6
#define IPPROTO_UDP					17


// #pragma mark -

// User Datagram Protocol (UDP)

// UDP header (RFC 768)
struct udp_header {
	uint16	source;			// source port
	uint16	destination;	// destination port
	uint16	length;			// length of UDP packet (header + data)
	uint16	checksum;		// checksum
} __attribute__ ((__packed__));


// Transmission Control Protocol (TCP)

// TCP header (RFC 793, RFC 3168)
struct tcp_header {
	uint16	source;			// source port
	uint16	destination;	// destination port
	uint32	seqNumber;		// sequence number
	uint32	ackNumber;		// acknowledgment number
#if __BYTE_ORDER == __BIG_ENDIAN
	uint8	dataOffset : 4;	// data offset
	uint8	reserved : 4;	// reserved
#elif __BYTE_ORDER == __LITTLE_ENDIAN
	uint8	reserved : 4;
	uint8	dataOffset : 4;
#endif
	uint8	flags;			// ACK, SYN, FIN, etc.
	uint16	window;			// window size
	uint16	checksum;		// checksum
	uint16	urgentPointer;	// urgent pointer
} __attribute__ ((__packed__));

#define TCP_FIN		(1 << 0)
#define TCP_SYN		(1 << 1)
#define TCP_RST		(1 << 2)
#define TCP_PSH		(1 << 3)
#define TCP_ACK		(1 << 4)
#define TCP_URG		(1 << 5)
#define TCP_ECE		(1 << 6)	// RFC 3168
#define TCP_CWR		(1 << 7)	// RFC 3168


// #pragma mark -

// NetService

// net service names
extern const char *const kEthernetServiceName;
extern const char *const kARPServiceName;
extern const char *const kIPServiceName;
extern const char *const kUDPServiceName;
extern const char *const kTCPServiceName;

class NetService {
public:
	NetService(const char *name);
	virtual ~NetService();

	const char *NetServiceName();

	virtual int CountSubNetServices() const;
	virtual NetService *SubNetServiceAt(int index) const;
	virtual NetService *FindSubNetService(const char *name) const;

	template<typename ServiceType>
	ServiceType *FindSubNetService(const char *name) const
	{
		// We should actually use dynamic_cast<>(), but we better spare us the
		// RTTI stuff.
		if (NetService *service = FindSubNetService(name))
			return static_cast<ServiceType*>(service);
		return NULL;
	}

private:
	const char	*fName;
};


#endif	// _BOOT_NET_DEFS_H
