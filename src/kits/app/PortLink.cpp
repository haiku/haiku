/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pahtz <pahtz@yahoo.com.au>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/** Class for low-overhead port-based messaging */

#include <stdlib.h>
#include <string.h>
#include <new>
#include <Region.h>
#include <Shape.h>

#include <LinkMsgReader.h>
#include <LinkMsgSender.h>
#include <PortLink.h>
#include <ServerProtocol.h>


BPortLink::BPortLink(port_id send, port_id receive)
	:
	fReader(new LinkMsgReader(receive)), 
	fSender(new LinkMsgSender(send))
{
}


BPortLink::~BPortLink()
{
	delete fReader;
	delete fSender;
}


status_t
BPortLink::ReadRegion(BRegion *region)
{
	fReader->Read(&region->count, sizeof(long));
	fReader->Read(&region->bound, sizeof(clipping_rect));
	region->set_size(region->count + 1);
	return fReader->Read(region->data, region->count * sizeof(clipping_rect));
}


status_t
BPortLink::AttachRegion(const BRegion &region)
{
	fSender->Attach(&region.count, sizeof(long));
	fSender->Attach(&region.bound, sizeof(clipping_rect));
	return fSender->Attach(region.data, region.count * sizeof(clipping_rect));
}


status_t
BPortLink::ReadShape(BShape *shape)
{
	int32 opCount, ptCount;
	fReader->Read(&opCount, sizeof(int32));
	fReader->Read(&ptCount, sizeof(int32));
	
	uint32 opList[opCount];
	fReader->Read(opList, opCount * sizeof(uint32));
	
	BPoint ptList[ptCount];
	fReader->Read(ptList, ptCount * sizeof(BPoint));
	
	shape->SetData(opCount, ptCount, opList, ptList);
	return B_OK;
}


status_t
BPortLink::AttachShape(BShape &shape)
{
	int32 opCount, ptCount;
	uint32 *opList;
	BPoint *ptList;
	
	shape.GetData(&opCount, &ptCount, &opList, &ptList);
	
	fSender->Attach(&opCount, sizeof(int32));
	fSender->Attach(&ptCount, sizeof(int32));
	fSender->Attach(opList, opCount * sizeof(uint32));
	return fSender->Attach(ptList, ptCount * sizeof(BPoint));
}


status_t
BPortLink::FlushWithReply(int32 &code)
{
	status_t status = Flush(B_INFINITE_TIMEOUT, true);
	if (status < B_OK)
		return status;

	return GetNextMessage(code);
}
