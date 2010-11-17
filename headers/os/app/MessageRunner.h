/*
 * Copyright 2001-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MESSAGE_RUNNER_H
#define _MESSAGE_RUNNER_H


#include <Messenger.h>


class BMessageRunner {
public:
								BMessageRunner(BMessenger target,
									const BMessage* message, bigtime_t interval,
									int32 count = -1);
								BMessageRunner(BMessenger target,
									const BMessage& message, bigtime_t interval,
									int32 count = -1);
								BMessageRunner(BMessenger target,
									const BMessage* message, bigtime_t interval,
									int32 count, BMessenger replyTo);
								BMessageRunner(BMessenger target,
									const BMessage& message, bigtime_t interval,
									int32 count, BMessenger replyTo);
	virtual						~BMessageRunner();

			status_t			InitCheck() const;

			status_t			SetInterval(bigtime_t interval);
			status_t			SetCount(int32 count);
			status_t			GetInfo(bigtime_t* interval,
									int32* count) const;

	static	status_t			StartSending(BMessenger target,
									const BMessage* message, bigtime_t interval,
									int32 count);
	static	status_t			StartSending(BMessenger target,
									const BMessage* message, bigtime_t interval,
									int32 count, BMessenger replyTo);

private:
								BMessageRunner(const BMessageRunner &);
								BMessageRunner &operator=(
									const BMessageRunner &);

	static	int32				_RegisterRunner(BMessenger target,
									const BMessage* message, bigtime_t interval,
									int32 count, bool detach,
									BMessenger replyTo);

			void				_InitData(BMessenger target,
									const BMessage* message, bigtime_t interval,
									int32 count, BMessenger replyTo);
			status_t			_SetParams(bool resetInterval,
									bigtime_t interval, bool resetCount,
									int32 count);

	virtual	void				_ReservedMessageRunner1();
	virtual	void				_ReservedMessageRunner2();
	virtual	void				_ReservedMessageRunner3();
	virtual	void				_ReservedMessageRunner4();
	virtual	void				_ReservedMessageRunner5();
	virtual	void				_ReservedMessageRunner6();

private:
			int32				fToken;
			uint32				_reserved[6];
};


#endif	// _MESSAGE_RUNNER_H
