#ifndef _PORTLINK_H_
#define _PORTLINK_H_

#include <Errors.h>
#include <OS.h>
#include <SupportDefs.h>
#include <Rect.h>

#ifndef _PORTLINK_BUFFERSIZE
#define _PORTLINK_MAX_ATTACHMENTS 50
#endif

#ifdef OPENBEOS
namespace OpenBeOS {
#endif

class PortLinkData;

class PortLink
{
public:
	PortLink(port_id port);
	~PortLink(void);
	void SetOpCode(int32 code);
	void SetPort(port_id port);
	port_id GetPort(void);
	void Flush(void);
	int8* FlushWithReply(int32 *code, status_t *status, ssize_t *buffersize,
		bigtime_t timeout=B_INFINITE_TIMEOUT);
	void Attach(void *data, size_t size);
	void Attach(int32 data);
	void Attach(int16 data);
	void Attach(int8 data);
	void Attach(float data);
	void Attach(bool data);
	void Attach(BRect data);
	void Attach(BPoint data);
	void MakeEmpty(void);
protected:	
	void FlattenData(int8 **buffer,int32 *size);
	port_id target, replyport;
	int32 	opcode, bufferlength;
	int		num_attachments;
	PortLinkData *attachments[_PORTLINK_MAX_ATTACHMENTS];
};

#ifdef OPENBEOS
}
#endif

#endif