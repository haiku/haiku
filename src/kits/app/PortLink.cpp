//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class for low-overhead port-based messaging
//  
//------------------------------------------------------------------------------
#include <ServerProtocol.h>
#include "PortLink.h"
#include "PortMessage.h"

PortLink::PortLink(port_id port)
{
	port_info pi;
	port_ok	= (get_port_info(port, &pi)==B_OK)? true: false;

	fSendPort=port;
	fReceivePort=create_port(30,"PortLink reply port");	

	fSendCode=0;
	fSendBuffer=new char[4096];
	fSendPosition=4;
}

PortLink::PortLink( const PortLink &link )
{
	port_ok=link.port_ok;

	fSendPort=link.fSendPort;
	fReceivePort=create_port(30,"PortLink reply port");
	
	fSendCode			= 0;
	fSendBuffer=new char[4096];
	fSendPosition		= 4;
}

PortLink::~PortLink(void)
{
	delete [] fSendBuffer;
}

void PortLink::SetOpCode( int32 code )
{
	fSendCode=code;
	int32 *cast=(int32*)fSendBuffer;
	*cast=code;
}

void PortLink::SetPort( port_id port )
{
	port_info pi;
	
	fSendPort=port;
	port_ok=(get_port_info(port, &pi) == B_OK)? true: false;
}

port_id PortLink::GetPort()
{
	return fSendPort;
}

status_t PortLink::Flush(bigtime_t timeout=B_INFINITE_TIMEOUT)
{
	status_t	write_stat;
	
	if(!port_ok)
		return B_BAD_VALUE;
	
	if(timeout!=B_INFINITE_TIMEOUT)
		write_stat=write_port_etc(fSendPort, fSendCode, fSendBuffer,
									fSendPosition, B_TIMEOUT, timeout);
	else
		write_stat=write_port(fSendPort, fSendCode, fSendBuffer, fSendPosition);
	
	fSendPosition=4;

	return write_stat;
}

status_t PortLink::FlushWithReply( PortMessage *msg,bigtime_t timeout=B_INFINITE_TIMEOUT )
{
	if(!port_ok || !msg)
		return B_BAD_VALUE;

	// attach our reply port_id at the end
	Attach<int32>(fReceivePort);

	// Flush the thing....FOOSH! :P
	write_port(fSendPort, fSendCode, fSendBuffer, fSendPosition);
	fSendPosition	= 4;
	
	// Now we wait for the reply
	ssize_t	rbuffersize;
	int8 *rbuffer = NULL;
	int32 rcode;
	
	if( timeout == B_INFINITE_TIMEOUT )
		rbuffersize	= port_buffer_size(fReceivePort);
	else
	{
		rbuffersize	= port_buffer_size_etc( fReceivePort, 0, timeout);
		if( rbuffersize == B_TIMED_OUT )
			return B_TIMED_OUT;
	}

	if( rbuffersize > 0 )
		rbuffer	= new int8[rbuffersize];
	read_port( fReceivePort, &rcode, rbuffer, rbuffersize);

	// We got this far, so we apparently have some data
	msg->SetCode(rcode);
	msg->SetBuffer(rbuffer,rbuffersize,false);
	
	return B_OK;
}

status_t PortLink::Attach(const void *data, size_t size)
{
	if (size <= 0)
		return B_ERROR;
  
	if (4096 - fSendPosition > (int32)size){
		memcpy(fSendBuffer + fSendPosition, data, size);
		fSendPosition += size;
		return B_OK;
	}
	return B_NO_MEMORY;
}

status_t PortLink::AttachString(const char *string)
{
	int16 len = (int16)strlen(string)+1;

	Attach<int16>(len);
	return Attach(string, len);
}

void PortLink::MakeEmpty()
{
	fSendPosition=4;
}

