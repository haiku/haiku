#ifndef _SESSION_H
#define _SESSION_H

#include <BeBuild.h>
#include <OS.h>
#include <Region.h>
#include <Rect.h>
#include <Point.h>
#include <ServerProtocol.h>

class BSession {

public:
		BSession(port_id receivePort, port_id sendPort);
		BSession(int32, char *);
		BSession( const BSession &ses );
		virtual	~BSession();
		void	SetSendPort( port_id port );
		void	SetRecvPort( port_id port );
		port_id	GetSendPort(void) const { return fSendPort; }
		port_id	GetRecvPort(void) const { return fReceivePort; }
		void	SetMsgCode(int32 code);

		char*	ReadString();
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
		void	Close();
	// if possible - do not use!
		void	CopyToSendBuffer(void* buffer, int32 count);
private:

		port_id	fSendPort;
		port_id	fReceivePort;

		char	*fSendBuffer;
		int32	fSendPosition;
		int32	fSendCode;
		int32	fRecvCode;

		char	*fReceiveBuffer;
		int32	fReceiveSize;
		int32	fReceivePosition;
};


#endif
