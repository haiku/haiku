/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

/*!	\class KPPPConfigurePacket
	\brief Helper class for LCP configure packets.
	
	This class allows iterating over configure items and adding/removing items.
*/

#include <KPPPConfigurePacket.h>
#include <KPPPInterface.h>

#include <core_funcs.h>


/*!	\brief Constructor.
	
	\param code The code value (e.g.: PPP_CONFIGURE_REQUEST) of this packet.
*/
KPPPConfigurePacket::KPPPConfigurePacket(uint8 code)
	: fCode(code),
	fID(0)
{
}


//!	Decodes a packet and adds its items to this object.
KPPPConfigurePacket::KPPPConfigurePacket(struct mbuf *packet)
{
	// decode packet
	ppp_lcp_packet *header = mtod(packet, ppp_lcp_packet*);
	
	SetID(header->id);
	if (!SetCode(header->code))
		return;
	
	uint16 length = ntohs(header->length);
	
	if (length < 6 || length > packet->m_len)
		return;
			// there are no items (or one corrupted item)
	
	int32 position = 0;
	ppp_configure_item *item;
	
	while (position < length - 4) {
		item = (ppp_configure_item*) (header->data + position);
		if (item->length < 2)
			return;
				// found a corrupted item
		
		position += item->length;
		AddItem(item);
	}
}


//!	Frees all items.
KPPPConfigurePacket::~KPPPConfigurePacket()
{
	for (int32 index = 0; index < CountItems(); index++)
		free(ItemAt(index));
}


//!	Sets the packet's code value (e.g.: PPP_CONFIGURE_REQUEST).
bool
KPPPConfigurePacket::SetCode(uint8 code)
{
	// only configure codes are allowed!
	if (code < PPP_CONFIGURE_REQUEST || code > PPP_CONFIGURE_REJECT)
		return false;
	
	fCode = code;
	
	return true;
}


/*!	\brief Adds a new configure item to this packet.
	
	Make sure all values are correct because the item will be copied. If the item's
	length field is incorrect you will get bad results.
	
	\param item The item.
	\param index Item's index. Adds after the last item if not specified or negative.
	
	\return \c true if successful, \c false otherwise.
	
	\sa ppp_configure_item
*/
bool
KPPPConfigurePacket::AddItem(const ppp_configure_item *item, int32 index)
{
	if (!item || item->length < 2)
		return false;
	
	ppp_configure_item *add = (ppp_configure_item*) malloc(item->length);
	memcpy(add, item, item->length);
	
	bool status;
	if (index < 0)
		status = fItems.AddItem(add);
	else
		status = fItems.AddItem(add, index);
	if (!status) {
		free(add);
		return false;
	}
	
	return true;
}


//!	Removes an item. The item \e must belong to this packet.
bool
KPPPConfigurePacket::RemoveItem(ppp_configure_item *item)
{
	if (!fItems.HasItem(item))
		return false;
	
	fItems.RemoveItem(item);
	free(item);
	
	return true;
}


//!	Returns the item at \a index or \c NULL.
ppp_configure_item*
KPPPConfigurePacket::ItemAt(int32 index) const
{
	ppp_configure_item *item = fItems.ItemAt(index);
	
	if (item == fItems.GetDefaultItem())
		return NULL;
	
	return item;
}


//!	Returns the item of a special \a type or \c NULL.
ppp_configure_item*
KPPPConfigurePacket::ItemWithType(uint8 type) const
{
	ppp_configure_item *item;
	
	for (int32 index = 0; index < CountItems(); index++) {
		item = ItemAt(index);
		if (item && item->type == type)
			return item;
	}
	
	return NULL;
}


/*!	\brief Converts this packet into an mbuf structure.
	
	ATTENTION: You are responsible for freeing this packet by calling \c m_freem()!
	
	\param MRU The interface's maximum receive unit (MRU).
	\param reserve Number of bytes to reserve at the beginning of the packet.
	
	\return The mbuf structure or \c NULL on error (e.g.: too big for given MRU).
*/
struct mbuf*
KPPPConfigurePacket::ToMbuf(uint32 MRU, uint32 reserve)
{
	struct mbuf *packet = m_gethdr(MT_DATA);
	packet->m_data += reserve;
	
	ppp_lcp_packet *header = mtod(packet, ppp_lcp_packet*);
	
	header->code = Code();
	header->id = ID();
	
	uint16 length = 0;
	ppp_configure_item *item;
	
	for (int32 index = 0; index < CountItems(); index++) {
		item = ItemAt(index);
		
		// make sure we have enough space left
		if (MRU - length < item->length) {
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
