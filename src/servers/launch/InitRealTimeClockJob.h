/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef INIT_REAL_TIME_CLOCK_JOB_H
#define INIT_REAL_TIME_CLOCK_JOB_H


#include <Job.h>
#include <Path.h>


class InitRealTimeClockJob : public BSupportKit::BJob {
public:
								InitRealTimeClockJob();

protected:
	virtual	status_t			Execute();

private:
			void				_SetRealTimeClockIsGMT(BPath path) const;
			void				_SetTimeZoneOffset(BPath path) const;
};


#endif // INIT_REAL_TIME_CLOCK_JOB_H
