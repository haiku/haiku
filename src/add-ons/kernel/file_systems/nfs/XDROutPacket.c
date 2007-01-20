#include "XDROutPacket.h"

#include <malloc.h>
#include <string.h>
#include <ByteOrder.h>

extern const uint8 *
XDROutPacketBuffer(struct XDROutPacket *packet)
{
	return packet->fBuffer;
}

extern size_t 
XDROutPacketLength(struct XDROutPacket *packet)
{
	return packet->fLength;
}

extern void 
XDROutPacketInit(struct XDROutPacket *packet)
{
	packet->fBuffer=NULL;
	packet->fSize=packet->fLength=0;
}

extern void 
XDROutPacketDestroy(struct XDROutPacket *packet)
{
	free (packet->fBuffer);
}

extern void 
XDROutPacketGrow(struct XDROutPacket *packet, size_t size)
{
	if (packet->fLength+size>packet->fSize)
	{
		while (packet->fLength+size>packet->fSize)
			packet->fSize+=XDROUTPACKET_BUFFER_INCREMENT;
			
		packet->fBuffer=(uint8 *)realloc(packet->fBuffer,packet->fSize);
	}
}

extern void 
XDROutPacketAddInt32(struct XDROutPacket *packet, int32 val)
{
	XDROutPacketGrow (packet,4);
	*(int32 *)(&packet->fBuffer[packet->fLength])=B_HOST_TO_BENDIAN_INT32(val);
	packet->fLength+=4;
}

extern void 
XDROutPacketAddDynamic(struct XDROutPacket *packet, const void *data, size_t size)
{
	XDROutPacketAddInt32 (packet,size);
	XDROutPacketAddFixed (packet,data,size);
}

extern void 
XDROutPacketAddFixed(struct XDROutPacket *packet, const void *data, size_t size)
{
	size_t roundedSize=(size+3)&~3;
	XDROutPacketGrow (packet,roundedSize);
	memcpy (&packet->fBuffer[packet->fLength],data,size);
	memset (&packet->fBuffer[packet->fLength+size],0,roundedSize-size);
	packet->fLength+=roundedSize;
}

extern void 
XDROutPacketAddString(struct XDROutPacket *packet, const char *string)
{
	XDROutPacketAddDynamic(packet,string,strlen(string));
}

extern void 
XDROutPacketAppend(struct XDROutPacket *me, const struct XDROutPacket *packet)
{
	XDROutPacketGrow (me,packet->fLength);
	memcpy (&me->fBuffer[me->fLength],packet->fBuffer,packet->fLength);
	me->fLength+=packet->fLength;
}

extern void 
XDROutPacketClear(struct XDROutPacket *packet)
{
	packet->fLength=0;
}

