/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold (bonefish@users.sf.net)
 */
#ifndef _MESSAGE_RUNNER_H
#define _MESSAGE_RUNNER_H


#include <Messenger.h>


class BMessageRunner {
	public:
		BMessageRunner(BMessenger target, const BMessage *message,
					   bigtime_t interval, int32 count = -1);
		BMessageRunner(BMessenger target, const BMessage *message,
					   bigtime_t interval, int32 count, BMessenger replyTo);
		BMessageRunner(BMessenger target, const BMessage *message,
					   bigtime_t interval, int32 count, bool detach);
		BMessageRunner(BMessenger target, const BMessage *message,
					   bigtime_t interval, int32 count, bool detach,
					   BMessenger replyTo);
		virtual ~BMessageRunner();

		status_t InitCheck() const;

		status_t SetInterval(bigtime_t interval);
		status_t SetCount(int32 count);
		status_t GetInfo(bigtime_t *interval, int32 *count) const;

	private:
		BMessageRunner(const BMessageRunner &);
		BMessageRunner &operator=(const BMessageRunner &);

		void _InitData(BMessenger target, const BMessage *message, bigtime_t interval,
					int32 count, bool detach, BMessenger replyTo);
		status_t _SetParams(bool resetInterval, bigtime_t interval, bool resetCount,
					int32 count);

		virtual void _ReservedMessageRunner1();
		virtual void _ReservedMessageRunner2();
		virtual void _ReservedMessageRunner3();
		virtual void _ReservedMessageRunner4();
		virtual void _ReservedMessageRunner5();
		virtual void _ReservedMessageRunner6();

		int32	fToken;
		bool	fDetached;

		bool	_unused0;
		bool	_unused1;
		bool	_unused2;
		uint32	_reserved[5];
};

#endif	// _MESSAGE_RUNNER_H
