//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
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
//	File Name:		ServerMemIO.cpp
//	Author(s):		Stefano Ceccherini (burton666@libero.it)
//					The Storage Team
//					DarkWyrm <bpmagic@columbus.rr.com>
//
//	Description:	Adaptation of BMemoryIO code to allow for client-side usage
//					of shared memory which is allocated by the server. Model is
//					similar to BBitmap in this respect.
//
//------------------------------------------------------------------------------

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <AppServerLink.h>
#include <ServerProtocol.h>
#include "ServerMemIO.h"

ServerMemIO::ServerMemIO(size_t size)
		: fLen(0),
		fPhys(0),
		fPos(0)
{
	if(size>0)
	{
		BPrivate::BAppServerLink link;
		link.StartMessage(AS_ACQUIRE_SERVERMEM);
		link.Attach<size_t>(size);
		link.Attach<port_id>(link.GetReplyPort());
		link.Flush();
		
		int32 code;
		link.GetNextReply(&code);
		
		if(code==SERVER_TRUE)
		{
			area_info ai;
			
			link.Read<area_id>(&fSourceArea);
			link.Read<size_t>(&fOffset);
			
			if(fSourceArea>0 && get_area_info(fSourceArea,&ai)==B_OK)
			{
				fPhys=size;
				
				fArea=clone_area("ServerMemIO area",(void**)&fBuf,B_CLONE_ADDRESS,
						B_READ_AREA|B_WRITE_AREA,fSourceArea);
				
				fBuf+=fOffset;
			}
			else
			{
				debugger("PANIC: bad data or something in ServerMemIO constructor");
			}
		}
	}
}


// Destruction
ServerMemIO::~ServerMemIO()
{
	BPrivate::BAppServerLink link;
	link.StartMessage(AS_RELEASE_SERVERMEM);
	link.Attach<area_id>(fSourceArea);
	link.Attach<int32>(fOffset);
	link.Flush();
}


// ReadAt
ssize_t
ServerMemIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (buffer == NULL || pos < 0)
		return B_BAD_VALUE;
		
	ssize_t sizeRead = 0;
	if (pos < fLen) 
	{
		sizeRead = min_c(static_cast<off_t>(size), fLen - pos);
		memcpy(buffer, fBuf + pos, sizeRead);
	}
	return sizeRead;
}


// WriteAt
ssize_t
ServerMemIO::WriteAt(off_t pos, const void *buffer, size_t size)
{	
	if (buffer == NULL || pos < 0)
		return B_BAD_VALUE;
		
	ssize_t sizeWritten = 0;	
	if (pos < fPhys) 
	{
		sizeWritten = min_c(static_cast<off_t>(size), fPhys - pos);
		memcpy(fBuf + pos, buffer, sizeWritten);
	}
	
	if (pos + sizeWritten > fLen)
		fLen = pos + sizeWritten;
						
	return sizeWritten;
}


// Seek
off_t
ServerMemIO::Seek(off_t position, uint32 seek_mode)
{
	switch (seek_mode) 
	{
		case SEEK_SET:
			fPos = position;
			break;
		case SEEK_CUR:
			fPos += position;
			break;		
		case SEEK_END:
			fPos = fLen + position;
			break;
		default:
			break;
	}	
	return fPos;
}


// Position
off_t
ServerMemIO::Position() const
{
	return fPos;
}


// SetSize
status_t
ServerMemIO::SetSize(off_t size)
{
	return B_NOT_ALLOWED;
}


size_t
ServerMemIO::GetSize(void) const
{
	return fLen;
}


void *
ServerMemIO::GetPointer(void) const
{
	return (void*)fBuf;
}


// Private or Reserved
ServerMemIO::ServerMemIO(const ServerMemIO &)
{
	//Copying not allowed
}


ServerMemIO &
ServerMemIO::operator=(const ServerMemIO &)
{
	//Copying not allowed
	return *this;
}

