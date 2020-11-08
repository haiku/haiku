#ifndef _XDRINPACKET_H

#define _XDRINPACKET_H

#include <SupportDefs.h>

struct XDRInPacket
{
	uint8 *fBuffer;
	size_t fOffset;	
};

void XDRInPacketInit (struct XDRInPacket *packet);
void XDRInPacketDestroy (struct XDRInPacket *packet);
int32 XDRInPacketGetInt32 (struct XDRInPacket *packet);
int32 XDRInPacketGetFixed (struct XDRInPacket *packet, void *buffer,
	size_t len);
size_t XDRInPacketGetDynamic (struct XDRInPacket *packet, void *buffer);
char *XDRInPacketGetString (struct XDRInPacket *packet);
void XDRInPacketSetTo (struct XDRInPacket *packet, uint8 *buffer, size_t offset);

#endif
