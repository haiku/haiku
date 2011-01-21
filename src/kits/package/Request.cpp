/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/Request.h>

#include <package/Context.h>
#include <package/Job.h>
#include <package/JobQueue.h>


namespace Haiku {

namespace Package {


Request::Request(const Context& context)
	:
	fContext(context)
{
}


Request::~Request()
{
}


const Context&
Request::GetContext() const
{
	return fContext;
}


status_t
Request::QueueJob(Job* job, JobQueue& jobQueue) const
{
	JobStateListener* defaultListener = fContext.DefaultJobStateListener();
	if (defaultListener != NULL)
		job->AddStateListener(defaultListener);

	return jobQueue.AddJob(job);
}


}	// namespace Package

}	// namespace Haiku
