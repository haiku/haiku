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
//	File Name:		PortMessage.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Package class for port messaging-based data
//  
//------------------------------------------------------------------------------
#include <Point.h>
#include <Rect.h>
#include "PortMessage.h"

PortMessage::PortMessage(const int32 &code, const void *buffer, const ssize_t &buffersize,
		const bool &copy)
{
	_code=code;
	SetBuffer(buffer,buffersize,copy);
}

PortMessage::PortMessage(void)
{
	_code=0;
	_buffer=NULL;
	_buffersize=0;
}

PortMessage::~PortMessage(void)
{
	if(_buffersize>0 && _buffer!=NULL)
		delete _buffer;
}


status_t PortMessage::ReadFromPort(const port_id &port, const bigtime_t &timeout=B_INFINITE_TIMEOUT)
{
	if(_buffersize>0 && _buffer!=NULL)
		delete _buffer;
	_buffer=NULL;
	_buffersize=0;

	if(timeout==B_INFINITE_TIMEOUT)
	{
		_buffersize=port_buffer_size(port);
		if(_buffersize==B_BAD_PORT_ID)
			return (status_t)_buffersize;

		if(_buffersize>0)
			_buffer=new uint8[_buffersize];
		read_port(port, &_code, _buffer, _buffersize);
	}
	else
	{
		_buffersize=port_buffer_size_etc(port,B_TIMEOUT, timeout);
		if(_buffersize==B_TIMED_OUT || _buffersize==B_BAD_PORT_ID || 
				_buffersize==B_WOULD_BLOCK)
			return (status_t)_buffersize;
			
		if(_buffersize>0)
			_buffer=new uint8[_buffersize];
		read_port(port, &_code, _buffer, _buffersize);
	}
	
	return B_OK;
}

status_t PortMessage::WriteToPort(const port_id &port)
{
	// Check port validity
	port_info pi;
	if(get_port_info(port,&pi)!=B_OK)
		return B_BAD_VALUE;
	
	write_port(port,_code, _buffer, _buffersize);
	
	if(_buffer && _buffersize>0)
	{
		delete _buffer;
		_buffersize=0;
		_buffer=NULL;
	}
	return B_OK;
}
	
void PortMessage::SetCode(const int32 &code)
{
	_code=code;
}

void PortMessage::SetBuffer(const void *buffer, const ssize_t &size, const bool &copy)
{
	if(_buffersize>0 && _buffer!=NULL)
		delete _buffer;
	_buffer=NULL;
	_buffersize=0;

	if(!buffer || size<1)
		return;

	if(copy)
	{
		_buffersize=size;
		_buffer=new uint8[_buffersize];
		memcpy(_buffer,buffer,_buffersize);
	}
	else
	{
		_buffersize=size;
		_buffer=(uint8 *)buffer;
	}
}

status_t PortMessage::Read(BPoint *data)
{
	if(!data)
		return B_BAD_VALUE;

	if( !_buffer || 
		_buffersize<sizeof(BPoint) ||
		((_index+sizeof(BPoint))>(_buffer+_buffersize)) )
		return B_NO_MEMORY;
	
	*data=*((BPoint*)_index);
	_index+=sizeof(BPoint);
	
	return B_OK;
}

status_t PortMessage::Read(BRect *data)
{
	if(!data)
		return B_BAD_VALUE;

	if( !_buffer || 
		_buffersize<sizeof(BRect) ||
		((_index+sizeof(BRect))>(_buffer+_buffersize)) )
		return B_NO_MEMORY;
	
	*data=*((BRect*)_index);
	_index+=sizeof(BRect);
	
	return B_OK;
}

status_t PortMessage::Read(float *data)
{
	if(!data)
		return B_BAD_VALUE;

	if( !_buffer || 
		_buffersize<sizeof(float) ||
		((_index+sizeof(float))>(_buffer+_buffersize)) )
		return B_NO_MEMORY;
	
	*data=*((float*)_index);
	_index+=sizeof(float);
	
	return B_OK;
}

status_t PortMessage::Read(int *data)
{
	if(!data)
		return B_BAD_VALUE;

	if( !_buffer || 
		_buffersize<sizeof(int) ||
		((_index+sizeof(int))>(_buffer+_buffersize)) )
		return B_NO_MEMORY;
	
	*data=*((int*)_index);
	_index+=sizeof(int);
	
	return B_OK;
}

status_t PortMessage::Read(long *data)
{
	if(!data)
		return B_BAD_VALUE;

	if( !_buffer || 
		_buffersize<sizeof(long) ||
		((_index+sizeof(long))>(_buffer+_buffersize)) )
		return B_NO_MEMORY;
	
	*data=*((long*)_index);
	_index+=sizeof(long);
	
	return B_OK;
}

status_t PortMessage::Read(double *data)
{
	if(!data)
		return B_BAD_VALUE;

	if( !_buffer || 
		_buffersize<sizeof(double) ||
		((_index+sizeof(double))>(_buffer+_buffersize)) )
		return B_NO_MEMORY;
	
	*data=*((double*)_index);
	_index+=sizeof(double);
	
	return B_OK;
}

status_t PortMessage::Read(void *data, ssize_t size)
{
	if(!data || size<1)
		return B_BAD_VALUE;

	if( !_buffer || 
		_buffersize<size ||
		((_index+size)>(_buffer+_buffersize)) )
		return B_NO_MEMORY;
	
	memcpy(data,_index,size);
	_index+=size;
	
	return B_OK;
}

void PortMessage::Rewind(void)
{
	_index=_buffer;
}

