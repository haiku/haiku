/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include <ByteOrder.h>
#include <NetBufferUtilities.h>
#include <net_buffer.h>

#include "DiscoveryPacket.h"

DiscoveryPacket::DiscoveryPacket(uint8 code, uint16 sessionID)
	: fCode(code),
	fSessionID(sessionID),
	fInitStatus(B_OK)
{
}


DiscoveryPacket::DiscoveryPacket(net_buffer *packet, uint32 start)
{
	// decode packet
	NetBufferHeaderReader<pppoe_header> bufferheader(packet);
	if (bufferheader.Status() != B_OK) {
		dprintf("NetBufferHeaderReader create fail\n");
		fInitStatus = B_ERROR;
		return;
	}
	pppoe_header &header = bufferheader.Data();
	
	SetCode(header.code);
	
	uint16 length = ntohs(header.length);
	
	if(length > packet->size - PPPoE_HEADER_SIZE) {
		dprintf("packet size less than pppoe payload\n");
		dprintf("pppoe payload: %d\n", length);
		dprintf("PPPoE_HEADER_SIZE: %d\n", PPPoE_HEADER_SIZE);
		dprintf("packet->size: %" B_PRIu32 "\n", packet->size);
		fInitStatus = B_ERROR;
		return;
			// there are no tags (or one corrupted tag)
	}
	
	int32 position = 0;
	pppoe_tag *tag;
	
	while(position <= length - 4) {
		tag = (pppoe_tag*) (header.data + position);
		position += ntohs(tag->length) + 4;
		
		AddTag(ntohs(tag->type), tag->data, ntohs(tag->length));
	}
	
	fInitStatus = B_OK;
}


DiscoveryPacket::~DiscoveryPacket()
{
	for(int32 index = 0; index < CountTags(); index++)
		free(TagAt(index));
}


bool
DiscoveryPacket::AddTag(uint16 type, const void *data, uint16 length, int32 index)
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


net_buffer*
DiscoveryPacket::ToNetBuffer(uint32 MTU)
{
	net_buffer *packet = gBufferModule->create(256);
	if (packet == NULL) {
		dprintf("create buffer failure\n");
		return NULL;
	}

	pppoe_header *header ;
	status_t status = gBufferModule->append_size(packet, 1492, (void **)&header);
	if (status != B_OK) {
		dprintf("append size failure\n");
		return NULL;
	}
	
	header->version = PPPoE_VERSION;
	header->type = PPPoE_TYPE;
	header->code = Code();
	header->sessionID = SessionID();
	
	uint16 length = 0;
	pppoe_tag *tag;
	
	for(int32 index = 0; index < CountTags(); index++) {
		tag = TagAt(index);
		
		// make sure we have enough space left
		if(MTU - length < tag->length) {
			gBufferModule->free(packet);
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
	status = gBufferModule->trim(packet, length + PPPoE_HEADER_SIZE);
	if (status != B_OK) {
		dprintf("trim buffer failure\n");
		return NULL;
	}

	return packet;
}
