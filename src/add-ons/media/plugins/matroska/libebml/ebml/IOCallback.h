/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2004 Ingo Ralf Blum.  All rights reserved.
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
	\version \$Id: IOCallback.h 639 2004-07-09 20:59:14Z mosu $
*/
#ifndef MATROSKA_IOCALLBACK_H
#define MATROSKA_IOCALLBACK_H

#include "EbmlTypes.h"

#include <exception>
#include <cstdio>
// #include <iostream>


START_LIBEBML_NAMESPACE

enum seek_mode
{
	seek_beginning=SEEK_SET
	,seek_end=SEEK_END
	,seek_current=SEEK_CUR
};

class EBML_DLL_API IOCallback
{
public:
	virtual ~IOCallback(){}

	// The read callback works like most other read functions. You specify the
	// file, the buffer and the size and the function returns the bytes read.
	// If an error occurs or the file pointer points to the end of the file 0 is returned.
	// Users are encouraged to throw a descriptive exception, when an error occurs.
	virtual uint32 read(void*Buffer,size_t Size)=0;

	// Seek to the specified position. The mode can have either SEEK_SET, SEEK_CUR
	// or SEEK_END. The callback should return true(1) if the seek operation succeeded
	// or false (0), when the seek fails.
	virtual void setFilePointer(int64 Offset,seek_mode Mode=seek_beginning)=0;

	// This callback just works like its read pendant. It returns the number of bytes written.
	virtual size_t write(const void*Buffer,size_t Size)=0;

	// Although the position is always positive, the return value of this callback is signed to
	// easily allow negative values for returning errors. When an error occurs, the implementor
	// should return -1 and the file pointer otherwise.
	//
	// If an error occurs, an exception should be thrown.
	virtual uint64 getFilePointer()=0;

	// The close callback flushes the file buffers to disk and closes the file. When using the stdio
	// library, this is equivalent to calling fclose. When the close is not successful, an exception
	// should be thrown.
	virtual void close()=0;


	// The readFully is made virtual to allow derived classes to use another
	// implementation for this method, which e.g. does not read any data
	// unlike this does
	void readFully(void*Buffer,size_t Size);

	template<class STRUCT> void readStruct(STRUCT&Struct){readFully(&Struct,sizeof(Struct));}

	void writeFully(const void*Buffer,size_t Size);

	template<class STRUCT> void writeStruct(const STRUCT&Struct){writeFully(&Struct,sizeof(Struct));}
};

/* cygwin incompatible
template<class TRAITS> std::basic_ostream<char,TRAITS>&operator<<(std::basic_ostream<char,TRAITS>&Stream,seek_mode Mode)
{
	switch(Mode)
	{
#define x(y) case seek_##y: Stream<<"seek_" #y; break
		x(beginning);
		x(current);
		x(end);
#undef x
	default:
		assert(false);
	}

	return Stream;
}
*/

END_LIBEBML_NAMESPACE

#endif // MATROSKA_IOCALLBACK_H
