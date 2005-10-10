/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef DISCOVERY_PACKET__H
#define DISCOVERY_PACKET__H

#include "PPPoE.h"

#include <TemplateList.h>


enum PPPoE_TAG_TYPE {
	END_OF_LIST = 0x0000,
	SERVICE_NAME = 0x0101,
	AC_NAME = 0x0102,
	HOST_UNIQ = 0x0103,
	AC_COOKIE = 0x0104,
	VENDOR_SPECIFIC = 0x0105,
	RELAY_SESSION_ID = 0x0110,
	SERVICE_NAME_ERROR = 0x0201,
	AC_SYSTEM_ERROR = 0x0202,
	GENERIC_ERROR = 0x0203
};

enum PPPoE_CODE {
	PADI = 0x09,
	PADO = 0x07,
	PADR = 0x19,
	PADS = 0x65,
	PADT = 0xA7
};


typedef struct pppoe_tag {
	uint16 type;
	uint16 length;
	uint8 data[0];
};


class DiscoveryPacket {
	public:
		DiscoveryPacket(uint8 code, uint16 sessionID = 0x0000);
		DiscoveryPacket(struct mbuf *packet, uint32 start = 0);
		~DiscoveryPacket();
		
		status_t InitCheck() const
			{ return fInitStatus; }
		
		void SetCode(uint8 code)
			{ fCode = code; }
		uint8 Code() const
			{ return fCode; }
		
		void SetSessionID(uint16 sessionID)
			{ fSessionID = sessionID; }
		uint16 SessionID() const
			{ return fSessionID; }
		
		bool AddTag(uint16 type, const void *data, uint16 length, int32 index = -1);
		bool RemoveTag(pppoe_tag *tag);
		int32 CountTags() const
			{ return fTags.CountItems(); }
		pppoe_tag *TagAt(int32 index) const;
		pppoe_tag *TagWithType(uint16 type) const;
		
		struct mbuf *ToMbuf(uint32 MTU, uint32 reserve = ETHER_HDR_LEN);
			// the user is responsible for freeing the mbuf

	private:
		uint8 fCode;
		uint16 fSessionID;
		TemplateList<pppoe_tag*> fTags;
		status_t fInitStatus;
};


#endif
