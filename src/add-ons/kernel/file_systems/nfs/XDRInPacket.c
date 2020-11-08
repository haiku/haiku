#include "XDRInPacket.h"
#include <malloc.h>
#include <string.h>
#include <ByteOrder.h>
#include <KernelExport.h>

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

extern int32
XDRInPacketGetFixed(struct XDRInPacket *packet, void *buffer, size_t len)
{
	if (user_memcpy(buffer, &packet->fBuffer[packet->fOffset], len)
		!= B_OK) {
		return NFSERR_IO;
	}

	packet->fOffset += (len + 3) & ~3;
	return NFS_OK;
}

extern size_t 
XDRInPacketGetDynamic(struct XDRInPacket *packet, void *buffer)
{
	size_t size=XDRInPacketGetInt32(packet);
	XDRInPacketGetFixed(packet,buffer,size);
	return size;
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
