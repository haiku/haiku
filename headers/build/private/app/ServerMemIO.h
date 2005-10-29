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
//	File Name:		DataIO.cpp
//	Author(s):		Ingo Weinhold (bonefish@users.sf.net)
//					Stefano Ceccherini (burton666@libero.it)
//					The Storage Team
//	Description:	Pure virtual BDataIO and BPositionIO classes provide
//					the protocol for Read()/Write()/Seek().
//
//					BMallocIO and ServerMemIO classes implement the protocol,
//					as does BFile in the Storage Kit.
//------------------------------------------------------------------------------
#ifndef _SERVERMEM_IO_H
#define _SERVERMEM_IO_H

#include <SupportDefs.h>
#include <DataIO.h>
#include <OS.h>

class ServerMemIO : public BPositionIO 
{
public:
	ServerMemIO(size_t size);
	virtual ~ServerMemIO();

	virtual ssize_t ReadAt(off_t position, void *buffer, size_t size);
	virtual ssize_t WriteAt(off_t position, const void *buffer, size_t size);

	virtual off_t Seek(off_t position, uint32 seekMode);
	virtual off_t Position() const;

	virtual status_t SetSize(off_t size);
	size_t GetSize(void) const;
	
	void *GetPointer(void) const;
	
private:
	ServerMemIO(const ServerMemIO &);
	ServerMemIO &operator=(const ServerMemIO &);

	char *fBuf;
	size_t fLen;
	size_t fPhys;
	size_t fPos;
	size_t fOffset;
	area_id fArea, fSourceArea;
};

#endif
