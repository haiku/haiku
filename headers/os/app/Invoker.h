/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen (xlr8@tref.nl)
 */
#ifndef _INVOKER_H
#define	_INVOKER_H


#include <BeBuild.h>
#include <Messenger.h>


class BHandler;
class BLooper;
class BMessage;


class BInvoker {
public:
								BInvoker();
								BInvoker(BMessage* message,
									const BHandler* handler,
									const BLooper* looper = NULL);
								BInvoker(BMessage* message, BMessenger target);
	virtual						~BInvoker();

	virtual	status_t			SetMessage(BMessage* message);
			BMessage*			Message() const;
			uint32				Command() const;

	virtual	status_t			SetTarget(const BHandler* handler,
									const BLooper* looper = NULL);
	virtual	status_t			SetTarget(BMessenger messenger);
			bool				IsTargetLocal() const;
			BHandler*			Target(BLooper** _looper = NULL) const;
			BMessenger			Messenger() const;

	virtual	status_t			SetHandlerForReply(BHandler* handler);
			BHandler*			HandlerForReply() const;

	virtual	status_t			Invoke(BMessage* message = NULL);
			status_t			InvokeNotify(BMessage* message,
									uint32 kind = B_CONTROL_INVOKED);
			status_t			SetTimeout(bigtime_t timeout);
			bigtime_t			Timeout() const;

protected:
			uint32				InvokeKind(bool* _notify = NULL);
			void				BeginInvokeNotify(
									uint32 kind = B_CONTROL_INVOKED);
			void				EndInvokeNotify();

private:
	virtual	void				_ReservedInvoker1();
	virtual	void				_ReservedInvoker2();
	virtual	void				_ReservedInvoker3();

								BInvoker(const BInvoker&);
			BInvoker&			operator=(const BInvoker&);

			BMessage*			fMessage;
			BMessenger			fMessenger;
			BHandler*			fReplyTo;
			bigtime_t			fTimeout;
			uint32				fNotifyKind;
			uint32				_reserved[1];
};


#endif	// _INVOKER_H
