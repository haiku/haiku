//------------------------------------------------------------------------------
//	MessagePrivate.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEPRIVATE_H
#define MESSAGEPRIVATE_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

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
			fMessage->fReplyTo.port = messenger.fPort;
			fMessage->fReplyTo.target = messenger.fHandlerToken;
			fMessage->fReplyTo.team = messenger.fTeam;
			fMessage->fReplyTo.preferred = messenger.fPreferredTarget;
		}
		inline int32 GetTarget()
		{
			return fMessage->fTarget;
		}
		inline bool UsePreferredTarget()
		{
			return fMessage->fPreferred;
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

