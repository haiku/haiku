#include "XDRInPacket.h"

#include <malloc.h>
#include <string.h>

#include <ByteOrder.h>
#include <KernelExport.h>

#include <kernel.h>

#include "nfs.h"

extern void 
XDRInPacketInit(struct XDRInPacket *packet)
{
	packet->fBuffer=NULL;
	packet->fOffset=0;
}

extern void 
XDRInPacketDestroy(struct XDRInPacket *packet)
{
	free (packet->fBuffer);
}

extern int32 
XDRInPacketGetInt32(struct XDRInPacket *packet)
{
	int32 val=B_BENDIAN_TO_HOST_INT32(*((int32 *)&packet->fBuffer[packet->fOffset]));
	
	packet->fOffset+=4;
	
	return val;
}


status_t
XDRInPacketGetFixed(struct XDRInPacket *packet, void *buffer, size_t len)
{
	if (!IS_USER_ADDRESS(buffer))
		memcpy(buffer, &packet->fBuffer[packet->fOffset], len);
	else if (user_memcpy(buffer, &packet->fBuffer[packet->fOffset], len) != B_OK)
		return B_BAD_ADDRESS;

	packet->fOffset += (len + 3) & ~3;
	return B_OK;
}


status_t
XDRInPacketGetDynamic(struct XDRInPacket *packet, void *buffer, size_t *len)
{
	*len = XDRInPacketGetInt32(packet);
	return XDRInPacketGetFixed(packet, buffer, *len);
}


extern char *
XDRInPacketGetString(struct XDRInPacket *packet)
{
	int32 size=XDRInPacketGetInt32(packet);
	char *string=(char *)malloc(size+1);
	string[size]=0;
	XDRInPacketGetFixed(packet,string,size);
	
	return string;
}

extern void 
XDRInPacketSetTo(struct XDRInPacket *packet, uint8 *buffer, size_t offset)
{
	free (packet->fBuffer);
	packet->fBuffer=buffer;
	packet->fOffset=offset;
}
