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
//	File Name:		Session.cpp
//	Author:			Adi Oanca <adioanca@mymail.ro>
//	Description:	Stream-based messaging class
//  
//------------------------------------------------------------------------------
#include <Session.h>
#include <stdio.h>

/*
#ifdef DEBUG_BSESSION
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif
*/

/*!
	\brief Standard constructor
	\param receivePort ID of the port the BSession is to receive messages from
	\param sendPort ID of the port the BSession is to send messages to
*/
BSession::BSession(port_id receivePort, port_id sendPort)
{
	fSendPort		= sendPort;
	fReceivePort	= receivePort;

	fSendBuffer			= NULL;
	fSendPosition		= 0;

	fReceiveBuffer		= NULL;
	fReceiveSize		= SESSION_BUFFER_SIZE;
	fReceivePosition	= SESSION_BUFFER_SIZE;

	fSendBuffer			= new char[SESSION_BUFFER_SIZE];
	fReceiveBuffer		= new char[SESSION_BUFFER_SIZE];
}

/*!
	\brief Copy constructor
	\param ses Session to copy
	
	The instantiated BSession utilizes the same ports but contains no data
*/
BSession::BSession( const BSession &ses)
{
	fSendPort		= ses.fSendPort;
	fReceivePort	= ses.fReceivePort;

	fSendBuffer			= NULL;
	fSendPosition		= 0;

	fReceiveBuffer		= NULL;
	fReceiveSize		= SESSION_BUFFER_SIZE;
	fReceivePosition	= SESSION_BUFFER_SIZE;

	fSendBuffer			= new char[SESSION_BUFFER_SIZE];
	fReceiveBuffer		= new char[SESSION_BUFFER_SIZE];
}

//! Deletes the allocated messaging buffers. Ports used are not deleted.
BSession::~BSession(void)
{
	delete [] fSendBuffer;
	delete [] fReceiveBuffer;
}

/*!
	\brief Sets the sender port to the passed ID
	\param port ID of the new sender port
	
	If the port ID is not valid, the Session will not send messages until a valid 
	port ID is passed to it.
*/
void BSession::SetSendPort( port_id port )
{
	port_info pi;
	
	if( get_port_info(port,&pi) != B_OK )
		fSendPort=0;
	else
		fSendPort=port;
}

/*!
	\brief Sets the receiver port to the passed ID
	\param port ID of the new receiver port
	
	If the port ID is not valid, the Session will not receive messages until a valid 
	port ID is passed to it.
*/
void BSession::SetRecvPort( port_id port )
{
	port_info pi;
	
	if( get_port_info(port,&pi) != B_OK )
		fReceivePort=0;
	else
		fReceivePort=port;
}

/*!
	\brief Returns a copy of the string in the message buffer
	\return A new string from the message buffer
	
	Caller is responsible for deleting the pointer
*/
char *BSession::ReadString(void)
{
	int16 len = 0;
	char *str = NULL;

	ReadData(&len, sizeof(int16));

	if(len > 0)
	{
		str = new char[len];
		ReadData(str, len);
	}
	return str;
}

/*!
	\brief Reads a boolean value from the message buffer
	\param b boolean variable to receive the value
*/
void BSession::ReadBool( bool* b )
{
	ReadData( b, sizeof(bool) );
}

/*!
	\brief Reads an 8-bit integer value from the message buffer
	\param i integer variable to receive the value
*/
void BSession::ReadInt8( int8* i )
{
	ReadData( i, sizeof(int8) );
}

/*!
	\brief Reads an unsigned 8-bit integer value from the message buffer
	\param i integer variable to receive the value
*/
void BSession::ReadUInt8( uint8* i )
{
	ReadData( i, sizeof(uint8) );
}

/*!
	\brief Reads an 16-bit integer value from the message buffer
	\param i integer variable to receive the value
*/
void BSession::ReadInt16( int16* i )
{
	ReadData( i, sizeof(int16) );
}

/*!
	\brief Reads an unsigned 16-bit integer value from the message buffer
	\param i integer variable to receive the value
*/
void BSession::ReadUInt16( uint16* i )
{
	ReadData( i, sizeof(uint16) );
}

/*!
	\brief Reads an 32-bit integer value from the message buffer
	\param i integer variable to receive the value
*/
void BSession::ReadInt32( int32* i )
{
	ReadData( i, sizeof(int32) );
}

/*!
	\brief Reads an unsigned 32-bit integer value from the message buffer
	\param i integer variable to receive the value
*/
void BSession::ReadUInt32( uint32* i )
{
	ReadData( i, sizeof(uint32) );
}

/*!
	\brief Reads an 64-bit integer value from the message buffer
	\param i integer variable to receive the value
*/
void BSession::ReadInt64( int64* i )
{
	ReadData( i, sizeof(int64) );
}

/*!
	\brief Reads an unisgned 64-bit integer value from the message buffer
	\param i integer variable to receive the value
*/
void BSession::ReadUInt64( uint64* i )
{
	ReadData( i, sizeof(uint64) );
}

/*!
	\brief Reads a floating point value from the message buffer
	\param f floating point variable to receive the value
*/
void BSession::ReadFloat(float *f)
{
	ReadData(f, sizeof(float));
}

/*!
	\brief Reads a 32-bit integer value into a floating point variable
	\param f floating point variable to receive the value
*/
void BSession::ReadFloatFromInt32(float *f)
{
	int32 i;
	ReadData( &i, sizeof(int32));
	*f = (float)i;
}

/*!
	\brief Reads a double precision value into a double precision variable
	\param d double precision variable to receive the value
*/
void BSession::ReadDouble(double *d)
{
	ReadData(d, sizeof(double));
}

/*!
	\brief Reads a 64-bit integer value into a double precision variable
	\param d double precision variable to receive the value
*/
void BSession::ReadDoubleFromInt64(double *d)
{
	int64 i;
	ReadData( &i, sizeof(int64));
	*d = (double)i;
}

/*!
	\brief Reads a BPoint value from the message buffer
	\param point BPoint variable to receive the value
*/
void BSession::ReadPoint(BPoint *point)
{
	ReadData( point, sizeof(BPoint));
}

/*!
	\brief Reads a BPoint value from two 32-bit integers in the message buffer
	\param point BPoint variable to receive the values
*/
void BSession::ReadPointFromInts(BPoint *point)
{
	int32 ipoint[2];
	ReadData( &ipoint, 2 * sizeof(int32));
	point->x = (float)ipoint[0];
	point->y = (float)ipoint[1];
}

/*!
	\brief Reads a clipping_rect from the message buffer
	\param rect clipping_rect variable to receive the value
*/
void BSession::ReadRectCR(clipping_rect *rect)
{
	ReadData( rect, sizeof(clipping_rect));
}

/*!
	\brief Reads a BRect value from the message buffer
	\param rect BRect variable to receive the value
*/
void BSession::ReadRect(BRect *rect)
{
	ReadData( rect, sizeof(BRect));
}

/*!
	\brief Reads a BRect from four 32-bit integers in the message buffer
	\param rect BRect variable to receive the values
*/
void BSession::ReadRectFromInts(BRect *rect)
{
	int32 irect[4];
	ReadData( irect, 4 * sizeof(int32));
	rect->left = (float)irect[0];
	rect->top = (float)irect[1];
	rect->right = (float)irect[2];
	rect->bottom = (float)irect[3];
}

/*!
	\brief Generic data-reading function
	\param data buffer to receive data
	\param size number of bytes to read from the message buffer
	
	This function fails if the receiver port ID is invalid or if the data pointer 
	is NULL. This function will cause a crash if the buffer size is less than size.
*/
void BSession::ReadData( void *data, int32 size)
{
	if(!data)
		return;
	
	// 0 = no receive port - Don't try to receive any data
	if ( fReceivePort == 0 )
		return;
	
	while (size > 0)
	{
		while (fReceiveSize == fReceivePosition)
		{
			ssize_t	portSize;
			portSize = port_buffer_size(fReceivePort);
			
			if (portSize == B_BAD_PORT_ID)
				return;
			else
				fReceiveSize= (portSize > SESSION_BUFFER_SIZE)? SESSION_BUFFER_SIZE: portSize;
			
			ssize_t rv;
			do
			{
				rv = read_port(fReceivePort, &fRecvCode, fReceiveBuffer, SESSION_BUFFER_SIZE);

				#ifdef DEBUG_BSESSION
				if (fRecvCode != AS_SERVER_SESSION && fRecvCodw != AS_SERVER_PORTLINK)
				{
					printf("BSession received a message not using BSession/PortLink protocol");
					printf("offset: %ld", fRecvCode - SERVER_TRUE);
				}
				printf("did read something...\n");
				#endif
			
			} while( fRecvCode != AS_SERVER_SESSION && fRecvCode != AS_SERVER_PORTLINK );
					
			fReceivePosition = 0;

			if( rv == B_BAD_PORT_ID )
			{
				fReceivePort=0;
				return;
			}
		}

		int32 copySize = fReceiveSize - fReceivePosition;
		
		if (size < copySize)
			copySize = size;

		memcpy(data, fReceiveBuffer + fReceivePosition, copySize);

		// workaround for: data += copySize
		uint8 *d = (uint8*)data;
		d += copySize;
		data = d;
		
		size -= copySize;
		fReceivePosition += copySize;
	}
}

/*!
	\brief Adds a string to the message buffer.
	\param string string to copy
	
	Internally speaking, the length of the string is written as a 16-bit integer immediately 
	prior to the actual string.
	
	This function will fail if the pointer is NULL.
*/
void BSession::WriteString(const char *string)
{
	int16 len = (int16)strlen(string) + 1;

	WriteData(&len, sizeof(int16));
	WriteData(string, len);
}

/*!
	\brief Adds a boolean value to the message buffer.
	\param b boolean value to add to the buffer
*/
void BSession::WriteBool(const bool& b)
{
	WriteData(&b, sizeof(bool));
}

/*!
	\brief Adds an 8-bit integer value to the message buffer.
	\param i integer value to add to the buffer
*/
void BSession::WriteInt8(const int8& i)
{
	WriteData(&i, sizeof(int8));
}

/*!
	\brief Adds an unsigned 8-bit integer value to the message buffer.
	\param i integer value to add to the buffer
*/
void BSession::WriteUInt8(const uint8& i)
{
	WriteData(&i, sizeof(uint8));
}

/*!
	\brief Adds a 16-bit integer value to the message buffer.
	\param i integer value to add to the buffer
*/
void BSession::WriteInt16(const int16& i)
{
	WriteData(&i, sizeof(int16));
}

/*!
	\brief Adds an unsigned 16-bit integer value to the message buffer.
	\param i integer value to add to the buffer
*/
void BSession::WriteUInt16(const uint16& i)
{
	WriteData(&i, sizeof(uint16));
}

/*!
	\brief Adds a 32-bit integer value to the message buffer.
	\param i integer value to add to the buffer
*/
void BSession::WriteInt32(const int32& i)
{
	WriteData(&i, sizeof(int32));
}

/*!
	\brief Adds an unsigned 32-bit integer value to the message buffer.
	\param i integer value to add to the buffer
*/
void BSession::WriteUInt32(const uint32& i)
{
	WriteData(&i, sizeof(uint32));
}

/*!
	\brief Adds a 64-bit integer value to the message buffer.
	\param i integer value to add to the buffer
*/
void BSession::WriteInt64(const int64& i)
{
	WriteData(&i, sizeof(int64));
}

/*!
	\brief Adds an unsigned 64-bit integer value to the message buffer.
	\param i integer value to add to the buffer
*/
void BSession::WriteUInt64(const uint64& i)
{
	WriteData(&i, sizeof(uint64));
}

/*!
	\brief Adds a floating point value to the message buffer.
	\param f floating point value to add to the buffer
*/
void BSession::WriteFloat(const float& f)
{
	WriteData(&f, sizeof(float));
}

/*!
	\brief Adds a floating point value to the message buffer as a 32-bit integer
	\param f floating point value to add to the buffer
*/
void BSession::WriteFloatAsInt32(const float& f)
{
	int32 i;
	i = (int32)f;
	WriteData(&i, sizeof(int32));
}

/*!
	\brief Adds a double precision value to the message buffer.
	\param d double precision value to add to the buffer
*/
void BSession::WriteDouble(const double& d)
{
	WriteData(&d, sizeof(double));
}

/*!
	\brief Adds a double precision value to the message buffer as a 64-bit integer
	\param d double precision value to add to the buffer
*/
void BSession::WriteDoubleAsInt64(const double& d)
{
	int64 i;
	i = (int64)d;
	WriteData(&i, sizeof(int64));
}

/*!
	\brief Adds a BPoint to the message buffer.
	\param point BPoint to add to the buffer
*/
void BSession::WritePoint(const BPoint& point)
{
	WriteData(&point, sizeof(BPoint));
}

/*!
	\brief Adds a BPoint to the message buffer as two 32-bit integers
	\param point BPoint to add to the buffer
*/
void BSession::WritePointAsInt32s(const BPoint& point)
{
	int32 ipoint[2];
	ipoint[0] = (int32)point.x;
	ipoint[1] = (int32)point.y;
	WriteData(ipoint, 2 * sizeof(int32));
}

/*!
	\brief Adds a clipping_rect to the message buffer.
	\param rect clipping_rect to add to the buffer
*/
void BSession::WriteRect(const clipping_rect& rect)
{
	WriteData(&rect, sizeof(clipping_rect));
}

/*!
	\brief Adds a BRect to the message buffer as four 32-bit integers
	\param rect BRect to add to the buffer
*/
void BSession::WriteRectAsInt32s(const BRect& rect)
{
	int32 irect[4];
	irect[0] = (int32)floor(rect.left);
	irect[1] = (int32)floor(rect.top);
	irect[2] = (int32)floor(rect.right);
	irect[3] = (int32)floor(rect.bottom);
	WriteData(&irect, 4 * sizeof(int32));
}

/*!
	\brief Adds a BRect to the message buffer.
	\param rect BRect to add to the buffer
*/
void BSession::WriteRect(const BRect& rect)
{
	WriteData(&rect, sizeof(BRect));
}

/*!
	\brief Generic data-writing function
	\param data buffer to write to the message buffer
	\param size number of bytes to write to the message buffer
	
	This function fails if the receiver port ID is invalid or if the data pointer 
	is NULL. This function will cause a crash if the buffer size is less than size.
*/
void BSession::WriteData(const void *data, int32 size)
{
	if(!data)
		return;
	
	// 0 = no send port - Don't send any data
	if ( fSendPort == 0 )
		return;

	if (size <= 0)
		return;
  
	if (SESSION_BUFFER_SIZE - fSendPosition > size)
	{
		memcpy(fSendBuffer + fSendPosition, data, size);
		fSendPosition += size;
		return;
	}

	status_t rv;
	do
	{
		int32 copySize = SESSION_BUFFER_SIZE - fSendPosition;
		
		if (size < copySize)
			copySize = size;

		memcpy(fSendBuffer + fSendPosition, data, copySize);

		// workaround for: data += copySize
		uint8	*d = (uint8*)data;
		d += copySize;
		data = d;
		
		size -= copySize;
		fSendPosition += copySize;

		if (fSendPosition != SESSION_BUFFER_SIZE)
			return;

		rv = write_port(fSendPort, AS_SERVER_SESSION, fSendBuffer, fSendPosition);
		
		fSendPosition = 0;

		if (rv == B_BAD_PORT_ID)
			return;

	} while (size > 0);
}

//! Flushes the message cache
void BSession::Sync(void)
{
	if (fSendPosition <= 0)
		return;
	
	status_t rv;
	rv = write_port(fSendPort, AS_SERVER_SESSION, fSendBuffer, fSendPosition);
		
	fSendPosition = 0;
}

/*!
	\brief Directly copies the passed buffer to the message buffer
	\param buffer The buffer to copy
	\count count currently unused
	
	Use of this function is discouraged wherever possible
*/
void BSession::CopyToSendBuffer(void* buffer, int32 count)
{
	// Note: 'count' is there because of future modifications to PortLink class
	int32 code;
	int32 size;

	uint8 *buf = (uint8*)buffer;
	
	code = *((int32*)buf);
	size = *((int32*)(buf+sizeof(int32)));
	//size		= count;

	*((int32*)fSendBuffer)	= code;

	fSendPosition = sizeof(int32); // = 4
	
	int32 compensated_buffer_size=SESSION_BUFFER_SIZE-sizeof(int32);
	
	memcpy( fSendBuffer + sizeof(int32),
			buf + 2*sizeof(int32), 
			size > compensated_buffer_size? compensated_buffer_size : size);

	fSendPosition += size > compensated_buffer_size? compensated_buffer_size : size;
}
