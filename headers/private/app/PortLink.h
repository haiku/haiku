#ifndef _PORTLINK_H
#define _PORTLINK_H


#include <BeBuild.h>
#include <OS.h>

#include "Session.h"

class PortMessage;

class PortLink : public BSession{
public:
		class ReplyData
		{
			public:
				ReplyData(void) { code=0; buffersize=0; buffer=NULL; }
				~ReplyData(void) { if(buffer) delete buffer; }
				
			int32		code;
			ssize_t		buffersize;
			int8		*buffer;
		};
		
					PortLink( port_id port );
					PortLink( const PortLink &link );
	virtual			~PortLink() { }

		void		SetOpCode( int32 code );
		void		SetPort( port_id port );
		port_id		GetPort();
		status_t	Flush( bigtime_t timeout = B_INFINITE_TIMEOUT );
		int8*		FlushWithReply( int32 *code, status_t *status, ssize_t *buffersize,
						bigtime_t timeout=B_INFINITE_TIMEOUT );
		status_t	FlushWithReply( PortLink::ReplyData *data,bigtime_t timeout=B_INFINITE_TIMEOUT );
		status_t	FlushWithReply( PortMessage *msg,bigtime_t timeout=B_INFINITE_TIMEOUT );
	
		status_t	Attach(const void *data, size_t size);
		void		MakeEmpty();
		template <class Type> status_t Attach(Type data)
		{
			int32 size	= sizeof(Type);

			if (4096 - fSendPosition > size){
				memcpy(fSendBuffer + fSendPosition, &data, size);
				fSendPosition += size;
				return B_OK;
			}
			return B_NO_MEMORY;
		}
		
private:
		bool	port_ok;
};

//extern _IMPEXP_BE _PortLink_ *main_session;

#endif
