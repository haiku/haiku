#ifndef _PORTLINK_H
#define _PORTLINK_H

#include <BeBuild.h>
#include <OS.h>

class PortMessage;

class PortLink
{
public:
	PortLink(port_id port);
	PortLink(const PortLink &link);
	virtual ~PortLink(void);

	void SetOpCode(int32 code);

	void SetPort(port_id port);
	port_id	GetPort();

	status_t Flush(bigtime_t timeout = B_INFINITE_TIMEOUT);
	status_t FlushWithReply(PortMessage *msg,bigtime_t timeout=B_INFINITE_TIMEOUT);
	
	void MakeEmpty();
	status_t Attach(const void *data, size_t size);
	status_t AttachString(const char *string);
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
	bool port_ok;
	port_id	fSendPort;
	port_id fReceivePort;
	char	*fSendBuffer;
	int32	fSendPosition;
	int32	fSendCode;
};

#endif
