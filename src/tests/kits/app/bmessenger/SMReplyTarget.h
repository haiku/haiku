// SMReplyTarget.h

#ifndef SM_REPLY_TARGET_H
#define SM_REPLY_TARGET_H

#include <Messenger.h>

class SMHandler;
class SMLooper;

class SMReplyTarget {
public:
	SMReplyTarget(bool preferred = false);
	virtual ~SMReplyTarget();

	virtual BHandler *Handler();
	virtual BMessenger Messenger();

	virtual bool ReplySuccess();

private:
	SMHandler	*fHandler;
	SMLooper	*fLooper;
};

#endif	// SM_REPLY_TARGET_H
