/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__REQUEST_H_
#define _HAIKU__PACKAGE__REQUEST_H_


#include <SupportDefs.h>


namespace Haiku {

namespace Package {


class Context;
class Job;
class JobQueue;


class Request {
public:
								Request(const Context& context);
	virtual						~Request();

	virtual	status_t			CreateJobsToRun(JobQueue& jobQueue) = 0;

			const Context&		GetContext() const;

protected:
			status_t			QueueJob(Job* job, JobQueue& jobQueue) const;
private:
			const Context&		fContext;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__REQUEST_H_
