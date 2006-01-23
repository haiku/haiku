//------------------------------------------------------------------------------
//	MessagePrivate.h
//
//------------------------------------------------------------------------------

#ifdef USING_MESSAGE4
#	include <MessagePrivate4.h>
#else

#ifndef MESSAGEPRIVATE_H
#define MESSAGEPRIVATE_H

#include <Message.h>
#include <Messenger.h>
#include <MessengerPrivate.h>
#include <TokenSpace.h>

class BMessage::Private {
	public:
		Private(BMessage* msg) : fMessage(msg) {;}
		Private(BMessage& msg) : fMessage(&msg) {;}

		inline void SetTarget(int32 token)
		{
			fMessage->fTarget = token;
			fMessage->fPreferred = token == B_PREFERRED_TOKEN;
		}

		inline void SetReply(team_id team, port_id port, int32 token)
		{
			fMessage->fReplyTo.port = port;
			fMessage->fReplyTo.target = token;
			fMessage->fReplyTo.team = team;
			fMessage->fReplyTo.preferred = token == B_PREFERRED_TOKEN;
		}

		inline void SetReply(BMessenger messenger)
		{
			BMessenger::Private mp(messenger);
			fMessage->fReplyTo.port = mp.Port();
			fMessage->fReplyTo.target = mp.Token();
			fMessage->fReplyTo.team = mp.Team();
			fMessage->fReplyTo.preferred = mp.IsPreferredTarget();
		}

		inline int32 GetTarget()
		{
			return fMessage->fTarget;
		}

		inline bool UsePreferredTarget()
		{
			return fMessage->fPreferred;
		}

		static inline status_t SendFlattenedMessage(void *data, int32 size,
			port_id port, int32 token, bigtime_t timeout)
		{
			return BMessage::_SendFlattenedMessage(data, size, port, token,
				timeout);
		}

		static inline void StaticInit()
		{
			BMessage::_StaticInit();
		}

		static inline void StaticCleanup()
		{
			BMessage::_StaticCleanup();
		}

		static inline void StaticCacheCleanup()
		{
			BMessage::_StaticCacheCleanup();
		}

	private:
		BMessage*	fMessage;
};

#endif	// MESSAGEPRIVATE_H
#endif	// USING_MESSAGE4
