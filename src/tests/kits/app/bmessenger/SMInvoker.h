// SMInvoker.h

#ifndef SM_INVOKER_H
#define SM_INVOKER_H

#include <Messenger.h>

class SMInvoker {
public:
	SMInvoker();
	virtual ~SMInvoker();

	virtual status_t Invoke(BMessenger &target, BHandler *replyHandler,
							BMessenger &replyMessenger) = 0;

	bool ReplySuccess();
	bool DirectReply();

protected:
	BMessage *fReplyMessage;
};

// Invoker for SendMessage(uint32, BHandler *)
class SMInvoker1 : public SMInvoker {
public:
	SMInvoker1(bool useReplyTo);

	virtual status_t Invoke(BMessenger &target, BHandler *replyHandler,
							BMessenger &replyMessenger);

private:
	bool	fUseReplyTo;
};

// Invoker for SendMessage(BMessage *, BHandler *, bigtime_t)
class SMInvoker2 : public SMInvoker {
public:
	SMInvoker2(bool useMessage, bool useReplyTo, bigtime_t timeout);

	virtual status_t Invoke(BMessenger &target, BHandler *replyHandler,
							BMessenger &replyMessenger);

private:
	bool		fUseMessage;
	bool		fUseReplyTo;
	bigtime_t	fTimeout;
};

// Invoker for SendMessage(BMessage *, BMessenger, bigtime_t)
class SMInvoker3 : public SMInvoker {
public:
	SMInvoker3(bool useMessage, bool useReplyTo, bigtime_t timeout);

	virtual status_t Invoke(BMessenger &target, BHandler *replyHandler,
							BMessenger &replyMessenger);

private:
	bool		fUseMessage;
	bool		fUseReplyTo;
	bigtime_t	fTimeout;
};

// Invoker for SendMessage(uint32, BMessage *)
class SMInvoker4 : public SMInvoker {
public:
	SMInvoker4(bool useReply);

	virtual status_t Invoke(BMessenger &target, BHandler *replyHandler,
							BMessenger &replyMessenger);

private:
	bool		fUseReply;
};

// Invoker for SendMessage(BMessage *, BMessage *, bigtime_t, bigtime_t)
class SMInvoker5 : public SMInvoker {
public:
	SMInvoker5(bool useMessage, bool useReply, bigtime_t deliveryTimeout,
			   bigtime_t replyTimeout);

	virtual status_t Invoke(BMessenger &target, BHandler *replyHandler,
							BMessenger &replyMessenger);

private:
	bool		fUseMessage;
	bool		fUseReply;
	bigtime_t	fDeliveryTimeout;
	bigtime_t	fReplyTimeout;
};

#endif	// SM_INVOKER_H
