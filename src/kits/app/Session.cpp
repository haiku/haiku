#include <Session.h>
#include <malloc.h>
#include <stdio.h>

#ifdef DEBUG_BSESSION
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

//------------------------------------------------------------------------------
BSession::BSession(port_id receivePort, port_id sendPort)
{
	fSendPort		= sendPort;
	fReceivePort	= receivePort;

	fSendCode			= AS_SERVER_SESSION;
	fSendBuffer			= NULL;
	fSendPosition		= 0;

	fReceiveBuffer		= NULL;
	fReceiveSize		= 1024;
	fReceivePosition	= 1024;

	fSendBuffer			= (char*)malloc(1024);
	fReceiveBuffer		= (char*)malloc(1024);
}
//------------------------------------------------------------------------------
BSession::BSession( const BSession &ses){
	fSendPort		= ses.fSendPort;
	fReceivePort	= ses.fReceivePort;

	fSendCode			= AS_SERVER_SESSION;
	fSendBuffer			= NULL;
	fSendPosition		= 0;

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
//------------DO NOT USE!-------------------------------------------------------
void BSession::SetMsgCode(int32 code){
	fSendCode=code;
}
//------------------------------------------------------------------------------
char* BSession::ReadString(){
	int16		len = 0;
	char		*str = NULL;

	ReadData( &len, sizeof(int16));

	if (len > 0){
		str = (char*)malloc(len);
		ReadData(str, len);
	}
	return str;
}
//------------------------------------------------------------------------------
void BSession::ReadBool( bool* b ){
	ReadData( b, sizeof(bool) );
}
//------------------------------------------------------------------------------
void BSession::ReadInt8( int8* i ){
	ReadData( i, sizeof(int8) );
}
//------------------------------------------------------------------------------
void BSession::ReadUInt8( uint8* i ){
	ReadData( i, sizeof(uint8) );
}
//------------------------------------------------------------------------------
void BSession::ReadInt16( int16* i ){
	ReadData( i, sizeof(int16) );
}
//------------------------------------------------------------------------------
void BSession::ReadUInt16( uint16* i ){
	ReadData( i, sizeof(uint16) );
}
//------------------------------------------------------------------------------
void BSession::ReadInt32( int32* i ){
	ReadData( i, sizeof(int32) );
}
//------------------------------------------------------------------------------
void BSession::ReadUInt32( uint32* i ){
	ReadData( i, sizeof(uint32) );
}
//------------------------------------------------------------------------------
void BSession::ReadInt64( int64* i ){
	ReadData( i, sizeof(int64) );
}
//------------------------------------------------------------------------------
void BSession::ReadUInt64( uint64* i ){
	ReadData( i, sizeof(uint64) );
}
//------------------------------------------------------------------------------
void BSession::ReadFloat(float *f)
{
	ReadData(f, sizeof(float));
}
//------------------------------------------------------------------------------
void BSession::ReadFloatFromInt32(float *f)
{
	int32 i;
	ReadData( &i, sizeof(int32));
	*f = (float)i;
}
//------------------------------------------------------------------------------
void BSession::ReadDouble(double *d)
{
	ReadData(d, sizeof(double));
}
//------------------------------------------------------------------------------
void BSession::ReadDoubleFromInt64(double *d)
{
	int64 i;
	ReadData( &i, sizeof(int64));
	*d = (double)i;
}
//------------------------------------------------------------------------------
void BSession::ReadPoint(BPoint *point)
{
	ReadData( point, sizeof(BPoint));
}
//------------------------------------------------------------------------------
void BSession::ReadPointFromInts(BPoint *point)
{
	int32 ipoint[2];
	ReadData( &ipoint, 2 * sizeof(int32));
	point->x = (float)ipoint[0];
	point->y = (float)ipoint[1];
}
//------------------------------------------------------------------------------
void BSession::ReadRectCR(clipping_rect *rect)
{
	ReadData( rect, sizeof(clipping_rect));
}
//------------------------------------------------------------------------------
void BSession::ReadRect(BRect *rect)
{
	ReadData( rect, sizeof(BRect));
}
//------------------------------------------------------------------------------
void BSession::ReadRectFromInts(BRect *rect)
{
	int32 irect[4];
	ReadData( irect, 4 * sizeof(int32));
	rect->left = (float)irect[0];
	rect->top = (float)irect[1];
	rect->right = (float)irect[2];
	rect->bottom = (float)irect[3];
}
//------------------------------------------------------------------------------
void BSession::ReadData( void *data, int32 size)
{
		// 0 = no receive port - Don't try to receive any data
	if ( fReceivePort == 0 )
		return;

	while (size > 0)
	{
		while (fReceiveSize == fReceivePosition)
		{
			ssize_t			portSize;
			portSize		= port_buffer_size(fReceivePort);
			if (portSize == B_BAD_PORT_ID)
				return;
			else
				fReceiveSize= portSize > 1024? 1024: portSize;

			ssize_t			rv;
			do{
				rv = read_port(fReceivePort, &fRecvCode, fReceiveBuffer, 1024);
#ifdef DEBUG_BSESSION
				if (fRecvCode != AS_SERVER_SESSION){
					printf("BSession received a code DIFFERENT from AS_SERVER_SESSION. ");
					printf("offset: %ld", fRecvCode - SERVER_TRUE);
				}
				printf("did read something...\n");
#endif
			}while( fRecvCode != AS_SERVER_SESSION );
					
			fReceivePosition = 0;

			if ( rv == B_BAD_PORT_ID )
				return;
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
}

//------------------------------------------------------------------------------
void BSession::WriteString(const char *string)
{
	int16 len = (int16)strlen(string);
	len++;

	WriteData(&len, sizeof(int16));
	WriteData(string, len);
}
//------------------------------------------------------------------------------
void BSession::WriteBool(const bool& b)
{
	WriteData(&b, sizeof(bool));
}
//------------------------------------------------------------------------------
void BSession::WriteInt8(const int8& i)
{
	WriteData(&i, sizeof(int8));
}
//------------------------------------------------------------------------------
void BSession::WriteUInt8(const uint8& i)
{
	WriteData(&i, sizeof(uint8));
}
//------------------------------------------------------------------------------
void BSession::WriteInt16(const int16& i)
{
	WriteData(&i, sizeof(int16));
}
//------------------------------------------------------------------------------
void BSession::WriteUInt16(const uint16& i)
{
	WriteData(&i, sizeof(uint16));
}
//------------------------------------------------------------------------------
void BSession::WriteInt32(const int32& i)
{
	WriteData(&i, sizeof(int32));
}
//------------------------------------------------------------------------------
void BSession::WriteUInt32(const uint32& i)
{
	WriteData(&i, sizeof(uint32));
}
//------------------------------------------------------------------------------
void BSession::WriteInt64(const int64& i)
{
	WriteData(&i, sizeof(int64));
}
//------------------------------------------------------------------------------
void BSession::WriteUInt64(const uint64& i)
{
	WriteData(&i, sizeof(uint64));
}
//------------------------------------------------------------------------------
void BSession::WriteFloat(const float& f)
{
	WriteData(&f, sizeof(float));
}
//------------------------------------------------------------------------------
void BSession::WriteFloatAsInt32(const float& f)
{
	int32 i;
	i = (int32)f;
	WriteData(&i, sizeof(int32));
}
//------------------------------------------------------------------------------
void BSession::WriteDouble(const double& d)
{
	WriteData(&d, sizeof(double));
}
//------------------------------------------------------------------------------
void BSession::WriteDoubleAsInt64(const double& d)
{
	int64 i;
	i = (int64)d;
	WriteData(&i, sizeof(int64));
}
//------------------------------------------------------------------------------
void BSession::WritePoint(const BPoint& point)
{
	WriteData(&point, sizeof(BPoint));
}
//------------------------------------------------------------------------------
void BSession::WritePointAsInt32s(const BPoint& point)
{
	int32 ipoint[2];
	ipoint[0] = (int32)point.x;
	ipoint[1] = (int32)point.y;
	WriteData(ipoint, 2 * sizeof(int32));
}
//------------------------------------------------------------------------------
void BSession::WriteRect(const clipping_rect& rect)
{
	WriteData(&rect, sizeof(clipping_rect));
}
//------------------------------------------------------------------------------
void BSession::WriteRectAsInt32s(const BRect& rect)
{
	int32 irect[4];
	irect[0] = (int32)floor(rect.left);
	irect[1] = (int32)floor(rect.top);
	irect[2] = (int32)floor(rect.right);
	irect[3] = (int32)floor(rect.bottom);
	WriteData(&irect, 4 * sizeof(int32));
}
//------------------------------------------------------------------------------
void BSession::WriteRect(const BRect& rect)
{
	WriteData(&rect, sizeof(BRect));
}
//------------------------------------------------------------------------------
void BSession::WriteData(const void *data, int32 size)
{
		// 0 = no send port - Don't send any data
	if ( fSendPort == 0 )
		return;

	if (size <= 0)
		return;
  
	if (1024 - fSendPosition > size)
	{
		memcpy(fSendBuffer + fSendPosition, data, size);
		fSendPosition += size;
		return;
	}

	status_t	rv;
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
			return;

		rv = write_port(fSendPort, fSendCode, fSendBuffer, fSendPosition);
		
		fSendPosition = 0;

		if (rv == B_BAD_PORT_ID)
			return;

	} while (size > 0);
}
//------------------------------------------------------------------------------
void BSession::Sync()
{
	if (fSendPosition <= 0)
		return;
	
	status_t	rv;
	rv = write_port(fSendPort, fSendCode, fSendBuffer, fSendPosition);
		
	fSendPosition = 0;
}
//------------------------------------------------------------------------------
void BSession::Close()
{
	Sync();
	delete this;	
}
//------------------if possible - no dot use!-----------------------------------
void BSession::CopyToSendBuffer(void* buffer, int32 count){
// Note: 'count' is there because of future modifications to PortLink class
	int32		code;
	int32		size;

	// be compatible with ANSI C++ standard... ufff
	uint8		*buf;
	buf			= (uint8*)buffer;
	
	code		= *((int32*)buf);
	size		= *((int32*)(buf+sizeof(int32)));
	//size		= count;

	*((int32*)fSendBuffer)	= code;
	fSendPosition			= sizeof(int32); // = 4

	memcpy(fSendBuffer + sizeof(int32), buf + 2*sizeof(int32), size > 1020? 1020 : size);
	fSendPosition			+= size > 1020? 1020 : size;
}
