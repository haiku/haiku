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
//	File Name:		Session.h
//	Author:			Adi Oanca <adioanca@mymail.ro>
//	Description:	Stream-based messaging class
//  
//------------------------------------------------------------------------------
#ifndef _SESSION_H
#define _SESSION_H

#include <BeBuild.h>
#include <OS.h>
#include <Region.h>
#include <Rect.h>
#include <Point.h>
#include <ServerProtocol.h>

#define SESSION_BUFFER_SIZE 1024

class BSession
{
public:
	BSession(port_id receivePort, port_id sendPort);
	BSession(int32, char *);
	BSession( const BSession &ses );
	virtual	~BSession(void);
	void	SetSendPort( port_id port );
	void	SetRecvPort( port_id port );
	port_id	GetSendPort(void) const { return fSendPort; }
	port_id	GetRecvPort(void) const { return fReceivePort; }
	
	char	*ReadString();
	void	ReadBool( bool *b );
	void	ReadInt8( int8 *i );
	void	ReadUInt8( uint8 *i );		
	void	ReadInt16( int16 *i );
	void	ReadUInt16( uint16 *i );		
	void	ReadInt32( int32 *i );
	void	ReadUInt32( uint32 *i );		
	void	ReadInt64( int64 *i );
	void	ReadUInt64( uint64 *i );		
	void	ReadFloat( float *f );
	void	ReadFloatFromInt32( float *f );
	void	ReadDouble( double *d );
	void	ReadDoubleFromInt64( double *d );
	void	ReadPoint( BPoint *point );
	void	ReadPointFromInts( BPoint *point );
	void	ReadRectCR( clipping_rect *rect );
	void	ReadRect( BRect *rect );
	void	ReadRectFromInts( BRect *rect );
	void	ReadData( void* data, int32 size );
	
	void	WriteString( const char* string );
	void	WriteBool( const bool& b );
	void	WriteInt8( const int8& i );
	void	WriteUInt8( const uint8& i );
	void	WriteInt16( const int16& i );
	void	WriteUInt16( const uint16& i );		
	void	WriteInt32( const int32& i );
	void	WriteUInt32( const uint32& i );		
	void	WriteInt64( const int64& i );
	void	WriteUInt64( const uint64& i );		
	void	WriteFloat(const float& f );
	void	WriteFloatAsInt32( const float& f );
	void	WriteDouble( const double& d );
	void	WriteDoubleAsInt64( const double& d );
	void	WritePoint(const BPoint& point);
	void	WritePointAsInt32s(const BPoint& point);
	void	WriteRect(const clipping_rect& rect);
	void	WriteRect(const BRect& rect);
	void	WriteRectAsInt32s(const BRect& rect);
	void	WriteData(const void* data, int32 size);
	
	void	Sync();

	// Deprecated. Do not use.
	void	CopyToSendBuffer(void* buffer, int32 count);

private:
	port_id	fSendPort;
	port_id	fReceivePort;
	
	char	*fSendBuffer;
	int32	fSendPosition;
	int32	fRecvCode;
	
	char	*fReceiveBuffer;
	int32	fReceiveSize;
	int32	fReceivePosition;
};


#endif
