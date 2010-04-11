/*
 * Copyright 2001-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Pahtz <pahtz@yahoo.com.au>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _SERVER_LINK_H
#define _SERVER_LINK_H


#include <OS.h>
#include <LinkReceiver.h>
#include <LinkSender.h>

class BShape;
class BString;
class BGradient;

/*
 * Error checking rules: (for if you don't want to check every return code)
 * - Calling EndMessage() is optional, implied by Flush() or StartMessage().
 * - If you are sending just one message you only need to test Flush() == B_OK
 * - If you are buffering multiple messages without calling Flush() you must
 *   check EndMessage() == B_OK, or the last Attach() for each message.
 *   Check Flush() at the end.
 * - If you are reading, check the last Read() or ReadString() you perform.
 */

namespace BPrivate {

class ServerLink {
public:
								ServerLink();
	virtual						~ServerLink();

			void				SetTo(port_id sender, port_id receiver);

	// send methods

			void				SetSenderPort(port_id port);
			port_id				SenderPort();

			void				SetTargetTeam(team_id team);
			team_id				TargetTeam();

			status_t			StartMessage(int32 code, size_t minSize = 0);
			void				CancelMessage();
			status_t			EndMessage();

			status_t			Flush(bigtime_t timeout = B_INFINITE_TIMEOUT,
									bool needsReply = false);
			status_t			Attach(const void* data, ssize_t size);
			status_t			AttachString(const char* string,
									int32 length = -1);
			status_t			AttachRegion(const BRegion& region);
			status_t			AttachShape(BShape& shape);
			status_t			AttachGradient(const BGradient& gradient);

			template <class Type>
			status_t			Attach(const Type& data);

	// receive methods

			void				SetReceiverPort(port_id port);
			port_id				ReceiverPort();

			status_t			GetNextMessage(int32& code,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
			bool				NeedsReply() const;
			status_t			Read(void* data, ssize_t size);
			status_t			ReadString(char* buffer, size_t bufferSize);
			status_t			ReadString(BString& string,
									size_t* _length = NULL);
			status_t			ReadString(char** _string,
									size_t* _length = NULL);
			status_t			ReadRegion(BRegion* region);
			status_t			ReadShape(BShape* shape);
			status_t			ReadGradient(BGradient** _gradient);
			
			template <class Type>
			status_t			Read(Type* data);

	// convenience methods

			status_t			FlushWithReply(int32& code);
			LinkSender&			Sender() { return *fSender; }
			LinkReceiver&		Receiver() { return *fReceiver; }

protected:
			LinkSender*			fSender;
			LinkReceiver*		fReceiver;
};


// #pragma mark - sender inline functions


inline void
ServerLink::SetSenderPort(port_id port)
{
	fSender->SetPort(port);
}


inline port_id
ServerLink::SenderPort()
{
	return fSender->Port();
}


inline void
ServerLink::SetTargetTeam(team_id team)
{
	fSender->SetTargetTeam(team);
}


inline team_id
ServerLink::TargetTeam()
{
	return fSender->TargetTeam();
}


inline status_t
ServerLink::StartMessage(int32 code, size_t minSize)
{
	return fSender->StartMessage(code, minSize);
}


inline status_t
ServerLink::EndMessage()
{
	return fSender->EndMessage();
}


inline void
ServerLink::CancelMessage()
{
	fSender->CancelMessage();
}


inline status_t
ServerLink::Flush(bigtime_t timeout, bool needsReply)
{
	return fSender->Flush(timeout, needsReply);
}


inline status_t
ServerLink::Attach(const void* data, ssize_t size)
{
	return fSender->Attach(data, size);
}


inline status_t
ServerLink::AttachString(const char* string, int32 length)
{
	return fSender->AttachString(string, length);
}


template<class Type> status_t
ServerLink::Attach(const Type& data)
{
	return Attach(&data, sizeof(Type));
}


// #pragma mark - receiver inline functions


inline void
ServerLink::SetReceiverPort(port_id port)
{
	fReceiver->SetPort(port);
}


inline port_id
ServerLink::ReceiverPort()
{
	return fReceiver->Port();
}


inline status_t
ServerLink::GetNextMessage(int32& code, bigtime_t timeout)
{
	return fReceiver->GetNextMessage(code, timeout);
}


inline bool
ServerLink::NeedsReply() const
{
	return fReceiver->NeedsReply();
}


inline status_t
ServerLink::Read(void* data, ssize_t size)
{
	return fReceiver->Read(data, size);
}


inline status_t
ServerLink::ReadString(char* buffer, size_t bufferSize)
{
	return fReceiver->ReadString(buffer, bufferSize);
}


inline status_t
ServerLink::ReadString(BString& string, size_t* _length)
{
	return fReceiver->ReadString(string, _length);
}


inline status_t
ServerLink::ReadString(char** _string, size_t* _length)
{
	return fReceiver->ReadString(_string, _length);
}


template <class Type> status_t
ServerLink::Read(Type* data)
{
	return Read(data, sizeof(Type));
}


}	// namespace BPrivate

#endif	/* _SERVER_LINK_H */
