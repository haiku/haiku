//------------------------------------------------------------------------------
//	MessagePrivate.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEPRIVATE_H
#define MESSAGEPRIVATE_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Message.h>
#include <Messenger.h>
#include <MessengerPrivate.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

extern "C" void		_msg_cache_cleanup_();
extern "C" int		_init_message_();
extern "C" int		_delete_message_();

class BMessage::Private
{
	public:
		Private(BMessage* msg) : fMessage(msg) {;}
		Private(BMessage& msg) : fMessage(&msg) {;}

		inline void SetTarget(int32 token, bool preferred)
		{
			fMessage->fTarget = token;
			fMessage->fPreferred = preferred;
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
			port_id port, int32 token, bool preferred, bigtime_t timeout)
		{
			return BMessage::_SendFlattenedMessage(data, size, port, token,
				preferred, timeout);
		}

	private:
		BMessage*	fMessage;
};

#endif	// MESSAGEPRIVATE_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

