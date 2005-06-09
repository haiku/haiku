/*******************************************************************************
/
/	File:			SoundPrivate.h
/
/   Description:	Implementation headers for SoundConsumer and SoundProducer.
/
/	Copyright 1998-1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if ! defined( _SoundPrivate_h )
#define _SoundPrivate_h

//	The following are implementation details that we don't
//	want to expose to the world at large.

#include "SoundUtils.h"

//	This structure is the body of a request that we use to
//	implement SetHooks().
struct set_hooks_q {
	port_id reply;
	void * cookie;
	SoundProcessFunc process;
	SoundNotifyFunc notify;
};

//	All incoming buffers and Media Kit requests arrive at a
//	media node in the form of messages (which are generally
//	dispatched for you by your superclasses' HandleMessage
//	implementations). Each message has a 'type' which is
//	analagous to a BMessage's 'what' field. We'll define our
//	own private message types for our SoundConsumer and
//	SoundProducer to use. The BeOS reserves a range,
//	0x60000000 to 0x7fffffff, for us to use.
enum {
	MSG_QUIT_NOW = 0x60000000L,
	MSG_CHANGE_HOOKS
};

#endif /* _SoundPrivate_h */
