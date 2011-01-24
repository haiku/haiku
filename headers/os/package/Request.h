/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__REQUEST_H_
#define _HAIKU__PACKAGE__REQUEST_H_


#include <SupportDefs.h>

#include <package/JobQueue.h>


namespace Haiku {

namespace Package {


class Context;
using Private::JobQueue;


class Request : protected JobStateListener {
public:
								Request(const Context& context);
	virtual						~Request();

	virtual	status_t			CreateInitialJobs() = 0;

			Job*				PopRunnableJob();

protected:
			status_t			QueueJob(Job* job);

			const Context&		fContext;

private:
			JobQueue			fJobQueue;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__REQUEST_H_
