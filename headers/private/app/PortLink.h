#ifndef _PORTLINK_H_
#define _PORTLINK_H_

#include <Errors.h>
#include <OS.h>
#include <SupportDefs.h>
#include <Rect.h>
#include <List.h>

class PortLinkData;
class PortMessage;

class PortLink
{
public:
	class ReplyData
	{
		public:
			ReplyData(void) { code=0; buffersize=0; buffer=NULL; }
			~ReplyData(void) { if(buffer) delete buffer; }
		int32 code;
		ssize_t buffersize;
		int8 *buffer;
	};
	PortLink(port_id port);
	PortLink(const PortLink &link);
	~PortLink(void);
	void SetOpCode(int32 code);
	void SetPort(port_id port);
	port_id GetPort(void);
	status_t Flush(bigtime_t timeout=B_INFINITE_TIMEOUT);
	int8* FlushWithReply(int32 *code, status_t *status, ssize_t *buffersize,
		bigtime_t timeout=B_INFINITE_TIMEOUT);
	status_t FlushWithReply(PortLink::ReplyData *data,bigtime_t timeout=B_INFINITE_TIMEOUT);
	status_t FlushWithReply(PortMessage *msg,bigtime_t timeout=B_INFINITE_TIMEOUT);
	
	status_t Attach(const void *data, size_t size);
	template <class Type> status_t Attach(Type data);
	void MakeEmpty(void);
protected:	
	void FlattenData(int8 **buffer,int32 *size);
	port_id target, replyport;
	int32 opcode;
	uint32 bufferlength,capacity;
	bool port_ok;
	BList *attachlist;
};

#endif
