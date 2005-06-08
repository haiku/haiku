/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Pahtz <pahtz@yahoo.com.au>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _PORTLINK_H
#define _PORTLINK_H

#include <OS.h>
#include <LinkMsgReader.h>
#include <LinkMsgSender.h>

/*
	Error checking rules: (for if you don't want to check every return code)
	
	Calling EndMessage() is optional, implied by Flush() or StartMessage().

	If you are sending just one message you only need to test Flush() == B_OK

	If you are buffering multiple messages without calling Flush() you must
		check EndMessage() == B_OK, or the last Attach() for each message. Check
		Flush() at the end.

	If you are reading, check the last Read() or ReadString() you perform.

*/

// ToDo: put this into the private namespace
//namespace BPrivate {

//class LinkMsgReader;
//class LinkMsgSender;

class BPortLink {
	public:
		BPortLink(port_id send = -1, port_id reply = -1);
		virtual ~BPortLink();

		// send methods

		void SetSendPort(port_id port);
		port_id	SendPort();

		status_t StartMessage(int32 code, size_t minSize = 0);
		void CancelMessage();
		status_t EndMessage();

		status_t Flush(bigtime_t timeout = B_INFINITE_TIMEOUT, bool needsReply = false);
		status_t Attach(const void *data, ssize_t size);
		status_t AttachString(const char *string, int32 length = -1);
		status_t AttachRegion(const BRegion &region);
		status_t AttachShape(BShape &shape);
		template <class Type> status_t Attach(const Type& data);

		// receive methods

		void SetReplyPort(port_id port);
		port_id	ReplyPort();

		status_t GetNextReply(int32 &code, bigtime_t timeout = B_INFINITE_TIMEOUT);
		status_t Read(void *data, ssize_t size);
		status_t ReadString(char **string);
		status_t ReadRegion(BRegion *region);
		status_t ReadShape(BShape *shape);
		template <class Type> status_t Read(Type *data);

		// convenience methods

		status_t FlushWithReply(int32 &code);

	protected:
		LinkMsgReader *fReader;
		LinkMsgSender *fSender;
};

// sender inline functions

inline void
BPortLink::SetSendPort(port_id port)
{
	fSender->SetPort(port);
}

inline port_id
BPortLink::SendPort()
{
	return fSender->Port();
}

inline status_t
BPortLink::StartMessage(int32 code, size_t minSize)
{
	return fSender->StartMessage(code, minSize);
}

inline status_t
BPortLink::EndMessage()
{
	return fSender->EndMessage();
}

inline void
BPortLink::CancelMessage()
{
	fSender->CancelMessage();
}

inline status_t
BPortLink::Flush(bigtime_t timeout, bool needsReply)
{
	return fSender->Flush(timeout, needsReply);
}

inline status_t
BPortLink::Attach(const void *data, ssize_t size)
{
	return fSender->Attach(data, size);
}

inline status_t
BPortLink::AttachString(const char *string, int32 length)
{
	return fSender->AttachString(string, length);
}

template<class Type> status_t
BPortLink::Attach(const Type &data)
{
	return Attach(&data, sizeof(Type));
}

// #pragma mark - receiver inline functions

inline void
BPortLink::SetReplyPort(port_id port)
{
	fReader->SetPort(port);
}

inline port_id
BPortLink::ReplyPort()
{
	return fReader->Port();
}

inline status_t
BPortLink::GetNextReply(int32 &code, bigtime_t timeout)
{
	return fReader->GetNextMessage(code, timeout);
}

inline status_t
BPortLink::Read(void *data, ssize_t size)
{
	return fReader->Read(data, size);
}

inline status_t
BPortLink::ReadString(char **string)
{
	return fReader->ReadString(string);
}

template <class Type> status_t
BPortLink::Read(Type *data)
{
	return Read(data, sizeof(Type));
}

//}	// namespace BPrivate

#endif	/* _PORTLINK_H */
