#include <Session.h>
#include <malloc.h>
#include <stdio.h>

//extern BSession *main_session = NULL;

//------------------------------------------------------------------------------
BSession::BSession(port_id receivePort, port_id sendPort, bool isPortLink = false)
{
	fSendPort		= sendPort;
	fReceivePort	= receivePort;

	fSendCode			= 0;
	fSendBuffer			= NULL;
	fSendPosition		= 4;

	fReceiveBuffer		= NULL;
	fReceiveSize		= 1024;
	fReceivePosition	= 1024;

	if ( !isPortLink ){
		fSendBuffer			= (char*)malloc(1024);
		fReceiveBuffer		= (char*)malloc(1024);
	}
}
//------------------------------------------------------------------------------
BSession::BSession( const BSession &ses){
	fSendPort		= ses.fSendPort;
	fReceivePort	= ses.fReceivePort;

	fSendCode			= 0;
	fSendBuffer			= NULL;
	fSendPosition		= 4;

	fReceiveBuffer		= NULL;
	fReceiveSize		= 1024;
	fReceivePosition	= 1024;

	fSendBuffer			= (char*)malloc(1024);
	fReceiveBuffer		= (char*)malloc(1024);
}
//------------------------------------------------------------------------------
BSession::~BSession(){
	free(fSendBuffer);
	free(fReceiveBuffer);
}
//------------------------------------------------------------------------------
void BSession::SetSendPort( port_id port ){
	fSendPort		= port;
}
//------------------------------------------------------------------------------
void BSession::SetRecvPort( port_id port ){
	fReceivePort	= port;
}
//------------------------------------------------------------------------------
bool BSession::DropInputBuffer(void){
	// doesn't matter their value, they just need to be equal.
	fReceiveSize		= 1024;
	fReceivePosition	= 1024;
	return true;
}
//------------------------------------------------------------------------------
void BSession::SetMsgCode(int32 code){
	fSendCode=code;
}
//------------------------------------------------------------------------------
char *BSession::ReadString(){
	int16		len = 0;
	char		*str = NULL;

	if ( ReadData( &len, sizeof(int16)) != B_OK )
		return NULL;

	if (len){
		str = (char*)malloc(len);
		if ( ReadData(str, len) != B_OK ){
			delete str;
			str		= NULL;
		}
	}

	return str;
}
//------------------------------------------------------------------------------
status_t BSession::ReadBool( bool* b ){
	return ReadData( b, sizeof(bool) );
}
//------------------------------------------------------------------------------
status_t BSession::ReadInt8( int8* i ){
	return ReadData( i, sizeof(int8) );
}
//------------------------------------------------------------------------------
status_t BSession::ReadUInt8( uint8* i ){
	return ReadData( i, sizeof(uint8) );
}
//------------------------------------------------------------------------------
status_t BSession::ReadInt16( int16* i ){
	return ReadData( i, sizeof(int16) );
}
//------------------------------------------------------------------------------
status_t BSession::ReadUInt16( uint16* i ){
	return ReadData( i, sizeof(uint16) );
}
//------------------------------------------------------------------------------
status_t BSession::ReadInt32( int32* i ){
	return ReadData( i, sizeof(int32) );
}
//------------------------------------------------------------------------------
status_t BSession::ReadUInt32( uint32* i ){
	return ReadData( i, sizeof(uint32) );
}
//------------------------------------------------------------------------------
status_t BSession::ReadInt64( int64* i ){
	return ReadData( i, sizeof(int64) );
}
//------------------------------------------------------------------------------
status_t BSession::ReadUInt64( uint64* i ){
	return ReadData( i, sizeof(uint64) );
}
//------------------------------------------------------------------------------
status_t BSession::ReadFloat(float *f)
{
	return ReadData(f, sizeof(float));
}
//------------------------------------------------------------------------------
status_t BSession::ReadFloatFromInt(float *f)
{
	int32 i;
	status_t rv;
	rv = ReadData( &i, sizeof(int32));
	*f = (float)i;
	return rv;
}
//------------------------------------------------------------------------------
status_t BSession::ReadDouble(double *d)
{
	return ReadData(d, sizeof(double));
}
//------------------------------------------------------------------------------
status_t BSession::ReadDoubleFromInt(double *d)
{
	int64 i;
	status_t rv;
	rv = ReadData( &i, sizeof(int64));
	*d = (double)i;
	return rv;
}
//------------------------------------------------------------------------------
status_t BSession::ReadPoint(BPoint *point)
{
	return ReadData( point, sizeof(BPoint));
}
//------------------------------------------------------------------------------
status_t BSession::ReadPointFromInts(BPoint *point)
{
	int32 ipoint[2];
	status_t rv;
	rv = ReadData( &ipoint, 2 * sizeof(int32));
	point->x = (float)ipoint[0];
	point->y = (float)ipoint[1];
	return rv;
}
//------------------------------------------------------------------------------
status_t BSession::ReadRectCR(clipping_rect *rect)
{
	return ReadData( rect, sizeof(clipping_rect));
}
//------------------------------------------------------------------------------
status_t BSession::ReadRect(BRect *rect)
{
	return ReadData( rect, sizeof(BRect));
}
//------------------------------------------------------------------------------
status_t BSession::ReadRectFromInts(BRect *rect)
{
	int32 irect[4];
	status_t rv;
	rv = ReadData( irect, 4 * sizeof(int32));
	rect->left = (float)irect[0];
	rect->top = (float)irect[1];
	rect->right = (float)irect[2];
	rect->bottom = (float)irect[3];
	return rv;
}
//------------------------------------------------------------------------------
status_t BSession::ReadData( void *data, int32 size)
{
	if ( fReceivePort == 0 )
		return B_BAD_VALUE;

	while (size > 0)
	{
		while (fReceiveSize == fReceivePosition)
		{
			int32 		fRecvCode;
			status_t	rv;
			while( (rv = port_buffer_size(fReceivePort) ) == B_WOULD_BLOCK);
					
			rv=read_port(fReceivePort, &fRecvCode, fReceiveBuffer, 1024);
			
			if ( rv == B_BAD_PORT_ID )
				return B_BAD_PORT_ID;
			
			fReceivePosition = 0;
			fReceiveSize = rv;
		}

		int32 copySize = fReceiveSize - fReceivePosition;
		
		if (size < copySize)
			copySize = size;

		memcpy(data, fReceiveBuffer + fReceivePosition, copySize);

			/* workaround for: data += copySize */
		uint8	*d = (uint8*)data;
		d += copySize;
		data = d;
		
		size -= copySize;
		fReceivePosition += copySize;
	}
	return B_OK;
}

//------------------------------------------------------------------------------
status_t BSession::WriteString(const char *string)
{
	int16 len = (int16)strlen(string);
	len++;

	WriteData(&len, sizeof(int16));
	return WriteData(string, len);
}
//------------------------------------------------------------------------------
status_t BSession::WriteBool(const bool& b)
{
	return WriteData(&b, sizeof(bool));
}
//------------------------------------------------------------------------------
status_t BSession::WriteInt8(const int8& i)
{
	return WriteData(&i, sizeof(int8));
}
//------------------------------------------------------------------------------
status_t BSession::WriteUInt8(const uint8& i)
{
	return WriteData(&i, sizeof(uint8));
}
//------------------------------------------------------------------------------
status_t BSession::WriteInt16(const int16& i)
{
	return WriteData(&i, sizeof(int16));
}
//------------------------------------------------------------------------------
status_t BSession::WriteUInt16(const uint16& i)
{
	return WriteData(&i, sizeof(uint16));
}
//------------------------------------------------------------------------------
status_t BSession::WriteInt32(const int32& i)
{
	return WriteData(&i, sizeof(int32));
}
//------------------------------------------------------------------------------
status_t BSession::WriteUInt32(const uint32& i)
{
	return WriteData(&i, sizeof(uint32));
}
//------------------------------------------------------------------------------
status_t BSession::WriteInt64(const int64& i)
{
	return WriteData(&i, sizeof(int64));
}
//------------------------------------------------------------------------------
status_t BSession::WriteUInt64(const uint64& i)
{
	return WriteData(&i, sizeof(uint64));
}
//------------------------------------------------------------------------------
status_t BSession::WriteFloat(const float& f)
{
	return WriteData(&f, sizeof(float));
}
//------------------------------------------------------------------------------
status_t BSession::WriteFloatAsInt(const float& f)
{
	int32 i;
	i = (int32)f;
	return WriteData(&i, sizeof(int32));
}
//------------------------------------------------------------------------------
status_t BSession::WriteDouble(const double& d)
{
	return WriteData(&d, sizeof(double));
}
//------------------------------------------------------------------------------
status_t BSession::WriteDoubleAsInt(const double& d)
{
	int64 i;
	i = (int64)d;
	return WriteData(&i, sizeof(int64));
}
//------------------------------------------------------------------------------
status_t BSession::WritePoint(const BPoint& point)
{
	return WriteData(&point, sizeof(BPoint));
}
//------------------------------------------------------------------------------
status_t BSession::WritePointAsInt(const BPoint& point)
{
	int32 ipoint[2];
	ipoint[0] = (int32)point.x;
	ipoint[1] = (int32)point.y;
	return WriteData(ipoint, 2 * sizeof(int32));
}
//------------------------------------------------------------------------------
status_t BSession::WriteRect(const clipping_rect& rect)
{
	return WriteData(&rect, sizeof(clipping_rect));
}
//------------------------------------------------------------------------------
status_t BSession::WriteRectAsInt(const BRect& rect)
{
	int32 irect[4];
	irect[0] = (int32)floor(rect.left);
	irect[1] = (int32)floor(rect.top);
	irect[2] = (int32)floor(rect.right);
	irect[3] = (int32)floor(rect.bottom);
	return WriteData(&irect, 4 * sizeof(int32));
}
//------------------------------------------------------------------------------
status_t BSession::WriteRect(const BRect& rect)
{
	return WriteData(&rect, sizeof(BRect));
}
//------------------------------------------------------------------------------
status_t BSession::WriteData(const void *data, int32 size)
{
	if ( fSendPort == 0 )
		return B_BAD_VALUE;

	if (size <= 0)
		return B_BAD_VALUE;
  
	if (1024 - fSendPosition > size)
	{
		memcpy(fSendBuffer + fSendPosition, data, size);
		fSendPosition += size;
		return B_OK;
	}

	status_t	rv = B_OK;
	status_t	temp_rv;
	do
	{
		int32 copySize = 1024 - fSendPosition;
		
		if (size < copySize)
			copySize = size;

		memcpy(fSendBuffer + fSendPosition, data, copySize);

			/* workaround for: data += copySize */
		uint8	*d = (uint8*)data;
		d += copySize;
		data = d;
		
		size -= copySize;
		fSendPosition += copySize;

		if (fSendPosition != 1024)
			return B_OK;

		*((int32*)fSendBuffer) = fSendPosition;
		while( (temp_rv = write_port(fSendPort, fSendCode, fSendBuffer,
			fSendPosition)) == B_WOULD_BLOCK);
			
		if (temp_rv != B_OK)
			rv	= temp_rv;

		fSendPosition = 4;
	} while (size > 0);

	return rv;
}
//------------------------------------------------------------------------------
status_t BSession::Sync()
{
	if (fSendPosition <= 4)
		return B_OK;
	
	status_t	rv;

	*(int32*)fSendBuffer = fSendCode;
	while( (rv = write_port(fSendPort, fSendCode, fSendBuffer,
		fSendPosition)) == B_WOULD_BLOCK);
	
	fSendPosition = 4;
	
	return rv;
}
//------------------------------------------------------------------------------
void BSession::Close()
{
	Sync();
	delete this;	
}
//------------------------------------------------------------------------------
