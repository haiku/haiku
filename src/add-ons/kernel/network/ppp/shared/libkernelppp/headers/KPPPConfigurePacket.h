//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_CONFIGURE_PACKET__H
#define _K_PPP_CONFIGURE_PACKET__H

#include <mbuf.h>
#include <List.h>


typedef struct ppp_configure_item {
	uint8 type;
	uint8 length;
	int8 data[0];
		// the data follows this structure
} ppp_configure_item;


class PPPConfigurePacket {
	private:
		// copies are not allowed!
		PPPConfigurePacket(const PPPConfigurePacket& copy);
		PPPConfigurePacket& operator= (const PPPConfigurePacket& copy);

	public:
		PPPConfigurePacket(uint8 code);
		PPPConfigurePacket(struct mbuf *packet);
		~PPPConfigurePacket();
		
		bool SetCode(uint8 code);
		uint8 Code() const
			{ return fCode; }
		
		void SetID(uint8 id)
			{ fID = id; }
		uint8 ID() const
			{ return fID; }
		
		bool AddItem(const ppp_configure_item *item, int32 index = -1);
		bool RemoveItem(ppp_configure_item *item);
		int32 CountItems() const
			{ return fItems.CountItems(); }
		ppp_configure_item *ItemAt(int32 index) const;
		ppp_configure_item *ItemWithType(uint8 type) const;
		
		struct mbuf *ToMbuf(uint32 reserve = 0);
			// the user is responsible for freeing the mbuf

	private:
		uint8 fCode, fID;
		List<ppp_configure_item*> fItems;
};


#endif
