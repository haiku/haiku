#ifndef _SESSION_H
#define _SESSION_H

#include <BeBuild.h>
#include <OS.h>
#include <Region.h>
#include <Rect.h>
#include <Point.h>

#define AS_SESSION_MSG		'assm'

class BSession {

public:
				BSession(port_id receivePort, port_id sendPort, bool isPortLink = false);
				BSession(int32, char *);
				BSession( const BSession &ses );
virtual			~BSession();
		void	SetSendPort( port_id port );
		void	SetRecvPort( port_id port );
		port_id GetSendPort(void) const { return fSendPort; }
		port_id GetRecvPort(void) const { return fReceivePort; }
		bool	DropInputBuffer();

		char*		ReadString();
		status_t	ReadBool( bool *b );
		status_t	ReadInt8( int8 *i );
		status_t	ReadUInt8( uint8 *i );		
		status_t	ReadInt16( int16 *i );
		status_t	ReadUInt16( uint16 *i );		
		status_t	ReadInt32( int32 *i );
		status_t	ReadUInt32( uint32 *i );		
		status_t	ReadInt64( int64 *i );
		status_t	ReadUInt64( uint64 *i );		
		status_t	ReadFloat( float *f );
		status_t	ReadFloatFromInt( float *f );
		status_t	ReadDouble( double *d );
		status_t	ReadDoubleFromInt( double *d );
		status_t	ReadPoint( BPoint *point );
		status_t	ReadPointFromInts( BPoint *point );
		status_t	ReadRectCR( clipping_rect *rect );
		status_t	ReadRect( BRect *rect );
		status_t	ReadRectFromInts( BRect *rect );
		status_t	ReadData( void* data, int32 size );

		status_t	WriteString( const char* string );
		status_t	WriteBool( const bool& b );
		status_t	WriteInt8( const int8& i );
		status_t	WriteUInt8( const uint8& i );
		status_t	WriteInt16( const int16& i );
		status_t	WriteUInt16( const uint16& i );		
		status_t	WriteInt32( const int32& i );
		status_t	WriteUInt32( const uint32& i );		
		status_t	WriteInt64( const int64& i );
		status_t	WriteUInt64( const uint64& i );		
		status_t	WriteFloat(const float& f );
		status_t	WriteFloatAsInt( const float& f );
		status_t	WriteDouble( const double& d );
		status_t	WriteDoubleAsInt( const double& d );
		status_t	WritePoint(const BPoint& point);
		status_t	WritePointAsInt(const BPoint& point);
		status_t	WriteRect(const clipping_rect& rect);
		status_t	WriteRect(const BRect& rect);
		status_t	WriteRectAsInt(const BRect& rect);
		status_t	WriteData(const void* data, int32 size);

		status_t	Sync();
		void		Close();
private:
friend class PortLink;

		port_id	fSendPort;
		port_id	fReceivePort;

		char	*fSendBuffer;
		int32	fSendPosition;
		int32	fSendCode;

		char	*fReceiveBuffer;
		int32	fReceiveSize;
		int32	fReceivePosition;
};

//extern _IMPEXP_BE _BSession_ *main_session;

#endif
