#include <malloc.h>

#include "PortLink.h"
#include "PortMessage.h"

PortLink::PortLink( port_id port)
	: BSession( 0, 0, true )
{

	port_info		pi;
	port_ok			= (get_port_info(port, &pi) == B_OK)? true: false;

	fSendPort 		= port;
	fReceivePort	= create_port(30,"PortLink reply port");	

	fSendCode			= 0;
	fSendBuffer			= NULL;	
	fSendPosition		= 4;
	
	fReceiveBuffer		= NULL;
	fReceiveSize		= 0;
	fReceivePosition	= 0;

	fSendBuffer			= (char*)malloc(4096);
	//fReceiveBuffer		= (char*)malloc(4096);
}
//------------------------------------------------------------------------------
PortLink::PortLink( const PortLink &link )
	: BSession( 0, 0, true )
{
	port_ok			= link.port_ok;

	fSendPort		= link.fSendPort;
	fReceivePort	= create_port(30,"PortLink reply port");
	
	fSendCode			= 0;
	fSendBuffer			= NULL;	
	fSendPosition		= 4;
	
	fReceiveBuffer		= NULL;
	fReceiveSize		= 0;
	fReceivePosition	= 0;

	fSendBuffer			= (char*)malloc(4096);
	//fReceiveBuffer		= (char*)malloc(4096);

}
//------------------------------------------------------------------------------
void PortLink::SetOpCode( int32 code ){
	fSendCode	= code;
}
//------------------------------------------------------------------------------
void PortLink::SetPort( port_id port ){
	port_info	pi;
	
	fSendPort	= port;
	port_ok		= (get_port_info(port, &pi) == B_OK)? true: false;
}
//------------------------------------------------------------------------------
port_id PortLink::GetPort(){
	return fSendPort;
}
//------------------------------------------------------------------------------
status_t PortLink::Flush( bigtime_t timeout = B_INFINITE_TIMEOUT ){
	status_t	write_stat;
	
	if(!port_ok) {
		return B_BAD_VALUE;
	}

	if( timeout != B_INFINITE_TIMEOUT )
		write_stat = write_port_etc(fSendPort, fSendCode, fSendBuffer + 4,
									fSendPosition - 4 , B_TIMEOUT, timeout);
	else
		write_stat = write_port( fSendPort, fSendCode, fSendBuffer + 4,
								 fSendPosition - 4 );
	
	fSendPosition	= 4;

	return write_stat;
}
//------------------------------------------------------------------------------
int8* PortLink::FlushWithReply(int32 *code, status_t *status, ssize_t *buffersize,
		bigtime_t timeout=B_INFINITE_TIMEOUT)
{
	if(!port_ok){
		*status	= B_BAD_VALUE;
		return NULL;
	}

	// attach our port_id first
	*((int32*)fSendBuffer)	= fReceivePort;

	// Flush the thing....FOOSH! :P
	write_port(fSendPort, fSendCode, fSendBuffer, fSendPosition);
	fSendPosition	= 4;
	
	// Now we wait for the reply
	int8	*buffer = NULL;
	if( timeout == B_INFINITE_TIMEOUT )
	{
		*buffersize		= port_buffer_size(fReceivePort);
		if( *buffersize > 0 )
			buffer		= new int8[*buffersize];
		read_port( fReceivePort, code, buffer, *buffersize);
	}
	else
	{
		*buffersize		= port_buffer_size_etc( fReceivePort, 0, timeout);
		if( *buffersize == B_TIMED_OUT )
		{
			*status		= *buffersize;
			return NULL;
		}
		if( *buffersize > 0 )
			buffer		= new int8[*buffersize];
		read_port(fReceivePort, code, buffer, *buffersize);
	}

	// We got this far, so we apparently have some data
	*status	= B_OK;
	return buffer;
}
//------------------------------------------------------------------------------
status_t PortLink::FlushWithReply( PortLink::ReplyData *data,
						bigtime_t timeout=B_INFINITE_TIMEOUT )
{
	if(!port_ok || !data)
		return B_BAD_VALUE;

	// attach our port_id first
	*((int32*)fSendBuffer)	= fReceivePort;

	// Flush the thing....FOOSH! :P
	write_port(fSendPort, fSendCode, fSendBuffer, fSendPosition);
	fSendPosition	= 4;
	
	// Now we wait for the reply
	if( timeout == B_INFINITE_TIMEOUT )
	{
		data->buffersize	= port_buffer_size(fReceivePort);
		if( data->buffersize > 0 )
			data->buffer	= new int8[data->buffersize];
		read_port( fReceivePort, &(data->code), data->buffer, data->buffersize);
	}
	else
	{
		data->buffersize	= port_buffer_size_etc( fReceivePort, 0, timeout);
		if( data->buffersize == B_TIMED_OUT )
			return B_TIMED_OUT;
			
		if( data->buffersize > 0 )
			data->buffer	= new int8[data->buffersize];
		read_port( fReceivePort, &(data->code), data->buffer, data->buffersize);
	}

	// We got this far, so we apparently have some data
	return B_OK;
}
//------------------------------------------------------------------------------
status_t PortLink::FlushWithReply( PortMessage *msg,
						bigtime_t timeout=B_INFINITE_TIMEOUT )
{
	if(!port_ok || !msg)
		return B_BAD_VALUE;

	// attach our port_id first
	*((int32*)fSendBuffer)	= fReceivePort;

	// Flush the thing....FOOSH! :P
	write_port(fSendPort, fSendCode, fSendBuffer, fSendPosition);
	fSendPosition	= 4;
	
	// Now we wait for the reply
	ssize_t		rbuffersize;
	int8		*rbuffer = NULL;
	int32		rcode;
	
	if( timeout == B_INFINITE_TIMEOUT )
	{
		rbuffersize	= port_buffer_size(fReceivePort);
		if( rbuffersize > 0 )
			rbuffer	= new int8[rbuffersize];
		read_port( fReceivePort, &rcode, rbuffer, rbuffersize);
	}
	else
	{
		rbuffersize	= port_buffer_size_etc( fReceivePort, 0, timeout);
		if( rbuffersize == B_TIMED_OUT )
			return B_TIMED_OUT;
			
		if( rbuffersize > 0 )
			rbuffer		= new int8[rbuffersize];
		read_port( fReceivePort, &rcode, rbuffer, rbuffersize);
	}

	// We got this far, so we apparently have some data
	msg->SetCode(rcode);
	msg->SetBuffer(rbuffer,rbuffersize,false);

	return B_OK;
}
//------------------------------------------------------------------------------
status_t PortLink::Attach(const void *data, size_t size){
	if (size <= 0)
		return B_ERROR;
  
	if (4096 - fSendPosition > (int32)size){
		memcpy(fSendBuffer + fSendPosition, data, size);
		fSendPosition += size;
		return B_OK;
	}
	return B_NO_MEMORY;
}
//------------------------------------------------------------------------------
void PortLink::MakeEmpty(){
	fSendPosition	= 4;
}
//------------------------------------------------------------------------------
