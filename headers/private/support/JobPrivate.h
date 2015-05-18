/*
 * Copyright 2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _JOB_PRIVATE_H_
#define _JOB_PRIVATE_H_


#include <Job.h>


namespace BSupportKit {


class BJob::Private {
public:
	Private(BJob& job)
		:
		fJob(job)
	{
	}

	void SetTicketNumber(uint32 ticketNumber)
	{
		fJob._SetTicketNumber(ticketNumber);
	}

	void ClearTicketNumber()
	{
		fJob._ClearTicketNumber();
	}

private:
			BJob&				fJob;
};


}	// namespace BSupportKit


#endif // _JOB_PRIVATE_H_
