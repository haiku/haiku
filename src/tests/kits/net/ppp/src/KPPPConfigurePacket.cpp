//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "KPPPConfigurePacket.h"


PPPConfigurePacket::PPPConfigurePacket(uint8 code)
	: fCode(code)
{
}


PPPConfigurePacket::PPPConfigurePacket(mbuf *packet)
{
	// decode packet
	lcp_packet *data = mtod(packet, lcp_packet*);
	
	if(!SetCode(data->code))
		return;
	
	if(data->length < 4)
		return;
			// there are no items (or one corrupted item)
	
	int32 position = 0;
	ppp_configure_item *item;
	
	while(position <= data->length - 4) {
		item = data->data + position;
		position += item->length;
		
		AddItem(item);
	}
}


PPPConfigurePacket::~PPPConfigurePacket()
{
	for(int32 index = 0; index < CountItems(); index++)
		free(ItemAt(index));
}


bool
PPPConfigurePacket::SetCode(uint8 code)
{
	// only configure codes are allowed!
	if(code < PPP_CONFIGURE_REQUEST || code > PPP_CONFIGURE_REJECT)
		return false;
	
	fCode = code;
	
	return true;
}


bool
PPPConfigurePacket::AddItem(const ppp_configure_item *item, int32 index = -1)
{
	if(item->length < 2)
		return false;
	
	ppp_configure_item *add = malloc(item->length);
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
PPPConfigurePacket::RemoveItem(ppp_configure_item *item)
{
	if(!fItems.HasItem(item))
		return false;
	
	fItems.RemoveItem(item);
	free(item);
	
	return true;
}


ppp_configure_item*
PPPConfigurePacket::ItemAt(int32 index) const
{
	ppp_configure_item *item = fItems.ItemAt(index);
	
	if(item == fItems.GetDefaultItem())
		return NULL;
	
	return item;
}


mbuf*
PPPConfigurePacket::ToMbuf(uint32 reserve = 0)
{
	mbuf *packet = m_gethdr(MT_DATA);
	packet->m_data += reserve;
	
	lcp_packet *data = mtod(packet, lcp_packet*);
	
	data->code = Code();
	
	uint8 length = 0;
	ppp_configure_item *item;
	
	for(int32 index = 0; index < CountItems(); index++) {
		item = ItemAt(index);
		
		if(0xFF - length < item->length) {
			m_freem(packet);
			return NULL;
		}
		
		memcpy(data->data + length, item, item->length);
		length += item->length;
	}
	
	data->length = length + 2;
	packet->m_len = data->length;
	
	return packet;
}
