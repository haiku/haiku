/******************************************************************************
/
/	File:			MessageRunner.h
/
/	Description:	
/
/	Copyright 1998, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _MESSAGE_RUNNER_H
#define _MESSAGE_RUNNER_H

#include <BeBuild.h>
#include <Messenger.h>

/*----------------------------------------------------------------------*/
/*----- BMessageRunner class --------------------------------------------*/

class	BMessageRunner {
public:
					BMessageRunner(BMessenger target, const BMessage *msg,
						bigtime_t interval, int32 count = -1);	
					BMessageRunner(BMessenger target, const BMessage *msg,
						bigtime_t interval, int32 count, BMessenger reply_to);	
virtual				~BMessageRunner();

		status_t	InitCheck() const;

		status_t	SetInterval(bigtime_t interval);
		status_t	SetCount(int32 count);
		status_t	GetInfo(bigtime_t *interval, int32 *count) const;

/*----- Private or reserved -----------------------------------------*/

private:

virtual	void		_ReservedMessageRunner1();
virtual	void		_ReservedMessageRunner2();
virtual	void		_ReservedMessageRunner3();
virtual	void		_ReservedMessageRunner4();
virtual	void		_ReservedMessageRunner5();
virtual	void		_ReservedMessageRunner6();

					BMessageRunner(const BMessageRunner &);
		BMessageRunner &operator=(const BMessageRunner &);

		void		InitData(BMessenger target, const BMessage *msg,
						bigtime_t interval, int32 count, BMessenger reply_to);	
		status_t	SetParams(bool reset_i, bigtime_t interval,
								bool reset_c, int32 count);

		int32		fToken;
		uint32		_reserved[6];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _MESSAGE_QUEUE_H */


