//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include <KPPPConfigurePacket.h>
#include <KPPPInterface.h>

#include <core_funcs.h>


KPPPConfigurePacket::KPPPConfigurePacket(uint8 code)
	: fCode(code),
	fID(0)
{
}


KPPPConfigurePacket::KPPPConfigurePacket(struct mbuf *packet)
{
	// decode packet
	ppp_lcp_packet *header = mtod(packet, ppp_lcp_packet*);
	
	SetID(header->id);
	if(!SetCode(header->code))
		return;
	
	uint16 length = ntohs(header->length);
	
	if(length < 6 || length > packet->m_len)
		return;
			// there are no items (or one corrupted item)
	
	int32 position = 0;
	ppp_configure_item *item;
	
	while(position < length - 4) {
		item = (ppp_configure_item*) (header->data + position);
		if(item->length < 2)
			return;
				// found a corrupted item
		
		position += item->length;
		AddItem(item);
	}
}


KPPPConfigurePacket::~KPPPConfigurePacket()
{
	for(int32 index = 0; index < CountItems(); index++)
		free(ItemAt(index));
}


bool
KPPPConfigurePacket::SetCode(uint8 code)
{
	// only configure codes are allowed!
	if(code < PPP_CONFIGURE_REQUEST || code > PPP_CONFIGURE_REJECT)
		return false;
	
	fCode = code;
	
	return true;
}


bool
KPPPConfigurePacket::AddItem(const ppp_configure_item *item, int32 index = -1)
{
	if(!item || item->length < 2)
		return false;
	
	ppp_configure_item *add = (ppp_configure_item*) malloc(item->length);
	memcpy(add, item, item->length);
	
	bool status;
	if(index < 0)
		status = fItems.AddItem(add);
	else
		status = fItems.AddItem(add, index);
	if(!status) {
		free(add);
		return false;
	}
	
	return true;
}


bool
KPPPConfigurePacket::RemoveItem(ppp_configure_item *item)
{
	if(!fItems.HasItem(item))
		return false;
	
	fItems.RemoveItem(item);
	free(item);
	
	return true;
}


ppp_configure_item*
KPPPConfigurePacket::ItemAt(int32 index) const
{
	ppp_configure_item *item = fItems.ItemAt(index);
	
	if(item == fItems.GetDefaultItem())
		return NULL;
	
	return item;
}


ppp_configure_item*
KPPPConfigurePacket::ItemWithType(uint8 type) const
{
	ppp_configure_item *item;
	
	for(int32 index = 0; index < CountItems(); index++) {
		item = ItemAt(index);
		if(item && item->type == type)
			return item;
	}
	
	return NULL;
}


struct mbuf*
KPPPConfigurePacket::ToMbuf(uint32 MRU, uint32 reserve = 0)
{
	struct mbuf *packet = m_gethdr(MT_DATA);
	packet->m_data += reserve;
	
	ppp_lcp_packet *header = mtod(packet, ppp_lcp_packet*);
	
	header->code = Code();
	header->id = ID();
	
	uint16 length = 0;
	ppp_configure_item *item;
	
	for(int32 index = 0; index < CountItems(); index++) {
		item = ItemAt(index);
		
		// make sure we have enough space left
		if(MRU - length < item->length) {
			m_freem(packet);
			return NULL;
		}
		
		memcpy(header->data + length, item, item->length);
		length += item->length;
	}
	
	length += 4;
	header->length = htons(length);
	packet->m_pkthdr.len = packet->m_len = length;
	
	return packet;
}
