/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2003 Jory Stone.  All rights reserved.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.
**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: MemIOCallback.cpp 693 2004-07-31 08:56:28Z robux4 $
	\author Jory Stone <jcsston @ toughguy.net>
*/

#include "ebml/MemIOCallback.h"
#include "ebml/Debug.h"
#include "ebml/EbmlConfig.h"

START_LIBEBML_NAMESPACE

MemIOCallback::MemIOCallback(uint64 DefaultSize)
{
	//The default size of the buffer is 128 bytes
	dataBuffer = (binary *)malloc(DefaultSize);
	if (dataBuffer == NULL) {
		mOk = false;
		std::stringstream Msg;
		Msg << "Failed to alloc memory block of size ";
// not working with VC6		Msg << DefaultSize;
		mLastErrorStr = Msg.str();				
		return;
	}
	
	dataBufferMemorySize = DefaultSize;
	dataBufferPos = 0;
	dataBufferTotalSize = 0;
	mOk = true;
}

MemIOCallback::~MemIOCallback()
{
	if (dataBuffer != NULL)
		free(dataBuffer);
}

uint32 MemIOCallback::read(void *Buffer, size_t Size)
{
	if (Buffer == NULL || Size < 1)
		return 0;
	//If the size is larger than than the amount left in the buffer
	if (Size + dataBufferPos > dataBufferTotalSize)
	{
		//We will only return the remaining data
		memcpy(Buffer, dataBuffer + dataBufferPos, dataBufferTotalSize - dataBufferPos);
		dataBufferPos = dataBufferTotalSize;
		return dataBufferTotalSize - dataBufferPos;
	}
		
	//Well... We made it here, so do a quick and simple copy
	memcpy(Buffer, dataBuffer+dataBufferPos, Size);
	dataBufferPos += Size;

	return Size;
}

void MemIOCallback::setFilePointer(int64 Offset, seek_mode Mode)
{
	if (Mode == seek_beginning)
		dataBufferPos = Offset;
	else if (Mode == seek_current)
		dataBufferPos = dataBufferPos + Offset;
	else if (Mode == seek_end)
		dataBufferPos = dataBufferTotalSize + Offset;
}

size_t MemIOCallback::write(const void *Buffer, size_t Size)
{
	if (dataBufferMemorySize < dataBufferPos + Size)
	{
		//We need more memory!
		dataBuffer = (binary *)realloc((void *)dataBuffer, dataBufferPos + Size);		
	}
	memcpy(dataBuffer+dataBufferPos, Buffer, Size);
	dataBufferPos += Size;
	if (dataBufferPos > dataBufferTotalSize)
		dataBufferTotalSize = dataBufferPos;

	return Size;
}

uint32 MemIOCallback::write(IOCallback & IOToRead, size_t Size)
{
	if (dataBufferMemorySize < dataBufferPos + Size)
	{
		//We need more memory!
		dataBuffer = (binary *)realloc((void *)dataBuffer, dataBufferPos + Size);		
	}
	IOToRead.readFully(&dataBuffer[dataBufferPos], Size);
	dataBufferTotalSize = Size;
	return Size;
}

END_LIBEBML_NAMESPACE
