//------------------------------------------------------------------------------
//	Copyright (c) 2003, Niels S. Reedijk
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

#include "usb_p.h"

Packet::Packet()
{
	d = new usb_packet_t;
}

Packet::~Packet()
{
	delete d;
}

void Packet::SetPipe( uint16 pipe )
{
	d->pipe = pipe;
}

void Packet::SetAddress( uint8 address )
{
	d->address = address;
}

void Packet::SetRequestData( uint8 *data )
{
	d->requestdata = data;
}

void Packet::SetBuffer( uint8 *buffer )
{
	d->buffer = buffer;
}

void Packet::SetBufferLength( int16 length )
{
	d->bufferlength = length;
}

void Packet::SetActualLength( size_t *actual_length )
{
	d->actual_length = actual_length;
};

usb_packet_t * Packet::GetData()
{
	return d;
}