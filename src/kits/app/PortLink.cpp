//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		PortLink.cpp
//	Author:			Pahtz <pahtz@yahoo.com.au>
//	Description:	Class for low-overhead port-based messaging
//  
//------------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>
#include <new>
#include <Region.h>
#include <Shape.h>

#include <ServerProtocol.h>
#include <PortLink.h>

BPortLink::BPortLink(port_id send, port_id receive) :
	fReader(new LinkMsgReader(receive)), fSender(new LinkMsgSender(send))
{
}

BPortLink::~BPortLink()
{
	delete fReader;
	delete fSender;
}

status_t BPortLink::StartMessage(int32 code)
{
	return fSender->StartMessage(code);
}

status_t BPortLink::EndMessage()
{
	return fSender->EndMessage();
}

void BPortLink::CancelMessage()
{
	fSender->CancelMessage();
}

status_t BPortLink::Attach(const void *data, ssize_t size)
{
	return fSender->Attach(data,size);
}

void BPortLink::SetSendPort( port_id port )
{
	fSender->SetPort(port);
}

port_id BPortLink::GetSendPort()
{
	return fSender->GetPort();
}

void BPortLink::SetReplyPort( port_id port )
{
	fReader->SetPort(port);
}

port_id BPortLink::GetReplyPort()
{
	return fReader->GetPort();
}

status_t BPortLink::Flush(bigtime_t timeout)
{
	return fSender->Flush(timeout);
}

status_t BPortLink::GetNextReply(int32 *code, bigtime_t timeout)
{
	return fReader->GetNextMessage(code,timeout);
}

status_t BPortLink::Read(void *data, ssize_t size)
{
	return fReader->Read(data,size);
}

status_t BPortLink::ReadString(char **string)
{
	return fReader->ReadString(string);
}

status_t BPortLink::AttachString(const char *string)
{
	return fSender->AttachString(string);
}

status_t BPortLink::ReadRegion(BRegion *region)
{
	fReader->Read(&region->count, sizeof(long));
	fReader->Read(&region->bound, sizeof(clipping_rect));
	region->set_size(region->count + 1);
	return fReader->Read(region->data, region->count * sizeof(clipping_rect));
}

status_t BPortLink::AttachRegion(const BRegion &region)
{
	fSender->Attach(&region.count, sizeof(long));
	fSender->Attach(&region.bound, sizeof(clipping_rect));
	return fSender->Attach(region.data, region.count * sizeof(clipping_rect));
}

status_t BPortLink::ReadShape(BShape *shape)
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

status_t BPortLink::AttachShape(BShape &shape)
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
