#ifndef _PORTLINK_H_
#define _PORTLINK_H_

#include <Errors.h>
#include <OS.h>
#include <SupportDefs.h>
#include <Rect.h>

#ifndef _PORTLINK_BUFFERSIZE
#define _PORTLINK_MAX_ATTACHMENTS 50
#endif

class PortLinkData;

class PortLink
{
	class ReplyData
	{
		public:
			ReplyData(void) { code=0; buffersize=0; buffer=NULL; }
		int32 code;
		ssize_t buffersize;
		int8 *buffer;
	};
public:
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
	status_t Attach(void *data, size_t size);
	status_t Attach(int32 data);
	status_t Attach(int16 data);
	status_t Attach(int8 data);
	status_t Attach(float data);
	status_t Attach(bool data);
	status_t Attach(BRect data);
	status_t Attach(BPoint data);
	void MakeEmpty(void);
protected:	
	void FlattenData(int8 **buffer,int32 *size);
	port_id target, replyport;
	int32 opcode;
	uint32 bufferlength,capacity;
	int		num_attachments;
	bool port_ok;
	PortLinkData *attachments[_PORTLINK_MAX_ATTACHMENTS];
};

#endif
