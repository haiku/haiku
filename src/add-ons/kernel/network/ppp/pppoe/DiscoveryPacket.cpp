//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "DiscoveryPacket.h"

#include <core_funcs.h>
#include <kernel_cpp.h>


DiscoveryPacket::DiscoveryPacket(uint8 code, uint16 sessionID = 0x0000)
	: fCode(code),
	fSessionID(sessionID)
{
}


DiscoveryPacket::DiscoveryPacket(struct mbuf *packet)
{
	// decode packet
	pppoe_header *header = mtod(packet, pppoe_header*);
	
	SetCode(header->code);
	
	if(ntohs(header->length) < 4)
		return;
			// there are no tags (or one corrupted tag)
	
	int32 position = 0;
	pppoe_tag *tag;
	
	while(position <= ntohs(header->length) - 4) {
		tag = (pppoe_tag*) (header->data + position);
		position += ntohs(tag->length) + 4;
		
		AddTag(ntohs(tag->type), ntohs(tag->length), tag->data);
	}
}


DiscoveryPacket::~DiscoveryPacket()
{
	for(int32 index = 0; index < CountTags(); index++)
		free(TagAt(index));
}


bool
DiscoveryPacket::AddTag(uint16 type, uint16 length, void *data, int32 index = -1)
{
	pppoe_tag *add = (pppoe_tag*) malloc(length + 4);
	add->type = type;
	add->length = length;
	memcpy(add->data, data, length);
	
	bool status;
	if(index < 0)
		status = fTags.AddItem(add);
	else
		status = fTags.AddItem(add, index);
	if(!status) {
		free(add);
		return false;
	}
	
	return true;
}


bool
DiscoveryPacket::RemoveTag(pppoe_tag *tag)
{
	if(!fTags.HasItem(tag))
		return false;
	
	fTags.RemoveItem(tag);
	free(tag);
	
	return true;
}


pppoe_tag*
DiscoveryPacket::TagAt(int32 index) const
{
	pppoe_tag *tag = fTags.ItemAt(index);
	
	if(tag == fTags.GetDefaultItem())
		return NULL;
	
	return tag;
}


pppoe_tag*
DiscoveryPacket::TagWithType(uint16 type) const
{
	pppoe_tag *tag;
	
	for(int32 index = 0; index < CountTags(); index++) {
		tag = TagAt(index);
		if(tag && tag->type == type)
			return tag;
	}
	
	return NULL;
}


struct mbuf*
DiscoveryPacket::ToMbuf(uint32 reserve = 0)
{
	struct mbuf *packet = m_gethdr(MT_DATA);
	packet->m_data += reserve;
	
	pppoe_header *header = mtod(packet, pppoe_header*);
	
	memset(header, 0, sizeof(header));
	header->ethernetHeader.ether_type = ETHERTYPE_PPPOEDISC;
	header->version = PPPoE_VERSION;
	header->type = PPPoE_TYPE;
	header->code = Code();
	header->sessionID = SessionID();
	
	uint16 length = 0;
	pppoe_tag *tag;
	
	for(int32 index = 0; index < CountTags(); index++) {
		tag = TagAt(index);
		
		// make sure we have enough space left
		if(1494 - length < tag->length) {
			m_freem(packet);
			return NULL;
		}
		
		*((uint16*)(header->data + length)) = htons(tag->type);
		length += 2;
		*((uint16*)(header->data + length)) = htons(tag->length);
		length += 2;
		memcpy(header->data + length, tag->data, tag->length);
		length += tag->length;
	}
	
	header->length = htons(length);
	packet->m_len = length;
	
	return packet;
}
