/*
 * Copyright 2003-2004, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _K_PPP_CONFIGURE_PACKET__H
#define _K_PPP_CONFIGURE_PACKET__H

#include <TemplateList.h>

struct mbuf;

//!	An abstract configure item.
typedef struct ppp_configure_item {
	uint8 type;
		//!< Item's type.
	uint8 length;
		//!< Item length (including appended data, if any).
	uint8 data[0];
		//!< Additional data may be appended to this structure.
} ppp_configure_item;


class KPPPConfigurePacket {
	private:
		// copies are not allowed!
		KPPPConfigurePacket(const KPPPConfigurePacket& copy);
		KPPPConfigurePacket& operator= (const KPPPConfigurePacket& copy);

	public:
		KPPPConfigurePacket(uint8 code);
		KPPPConfigurePacket(struct mbuf *packet);
		~KPPPConfigurePacket();
		
		bool SetCode(uint8 code);
		//!	Returns this packet's code value.
		uint8 Code() const
			{ return fCode; }
		
		//!	Sets the packet's (unique) ID. Use KPPPLCP::NextID() to get a unique ID.
		void SetID(uint8 id)
			{ fID = id; }
		//!	Returns this packet's ID.
		uint8 ID() const
			{ return fID; }
		
		bool AddItem(const ppp_configure_item *item, int32 index = -1);
		bool RemoveItem(ppp_configure_item *item);
		//!	Returns number of items in this packet.
		int32 CountItems() const
			{ return fItems.CountItems(); }
		ppp_configure_item *ItemAt(int32 index) const;
		ppp_configure_item *ItemWithType(uint8 type) const;
		
		struct mbuf *ToMbuf(uint32 MRU, uint32 reserve = 0);
			// the user is responsible for freeing the mbuf

	private:
		uint8 fCode, fID;
		TemplateList<ppp_configure_item*> fItems;
};


#endif
