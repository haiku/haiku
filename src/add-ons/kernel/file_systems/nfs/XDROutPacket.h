#ifndef _XDROUTPACKET_H

#define _XDROUTPACKET_H

#include <SupportDefs.h>

#define XDROUTPACKET_BUFFER_INCREMENT 512

struct XDROutPacket
{
	uint8 *fBuffer;
	size_t fSize;
	size_t fLength;
};

void XDROutPacketInit (struct XDROutPacket *packet);
void XDROutPacketDestroy (struct XDROutPacket *packet);

void XDROutPacketGrow (struct XDROutPacket *packet, size_t size);
void XDROutPacketAddInt32 (struct XDROutPacket *packet, int32 val);
void XDROutPacketAddDynamic (struct XDROutPacket *packet, const void *data, size_t size);
void XDROutPacketAddFixed (struct XDROutPacket *packet, const void *data, size_t size);
void XDROutPacketAddString (struct XDROutPacket *packet, const char *string);
void XDROutPacketAppend (struct XDROutPacket *me, const struct XDROutPacket *packet);

const uint8 *XDROutPacketBuffer (struct XDROutPacket *packet);
size_t XDROutPacketLength (struct XDROutPacket *packet);
void XDROutPacketClear (struct XDROutPacket *packet);

#endif
