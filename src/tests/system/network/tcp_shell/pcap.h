/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PCAP_H
#define PCAP_H

#include <SupportDefs.h>


struct pcap_header {
	uint32 magic;
	uint16 version_major;
	uint16 version_minor;
	int32  timezone;
	uint32 timestamp_accuracy;
	uint32 max_packet_length;
	uint32 linktype;
} _PACKED;

#define PCAP_MAGIC	(0xa1b2c3d4)

#define PCAP_LINKTYPE_RAW		(101)		/* Raw IPv4/IPv6 */
#define PCAP_LINKTYPE_IPV4		(228)
#define PCAP_LINKTYPE_IPV6		(229)


struct pcap_packet_header {
	uint32 ts_sec;
	uint32 ts_usec;
	uint32 included_len;
	uint32 original_len;
} _PACKED;


#endif	// PCAP_H
