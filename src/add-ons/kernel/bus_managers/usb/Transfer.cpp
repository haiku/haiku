//------------------------------------------------------------------------------
//	Copyright (c) 2003-2004, Niels S. Reedijk
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

Transfer::Transfer( Pipe &pipe )
{
	d = new usb_transfer_t;
	d->pipe = pipe.d;
}

Transfer::~Transfer()
{
	delete d;
}

void Transfer::SetRequestData( usb_request_data *data )
{
	d->request = data;
}

void Transfer::SetBuffer( uint8 *buffer )
{
	d->buffer = buffer;
}

void Transfer::SetBufferLength( int16 length )
{
	d->bufferlength = length;
}

void Transfer::SetActualLength( size_t *actual_length )
{
	d->actual_length = actual_length;
};

usb_transfer_t * Transfer::GetData()
{
	return d;
}