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


namespace Haiku {

namespace Package {


Request::Request(const Context& context)
	:
	fContext(context),
	fJobQueue()
{
}


Request::~Request()
{
}


Job*
Request::PopRunnableJob()
{
	return fJobQueue.Pop();
}


status_t
Request::QueueJob(Job* job)
{
	job->AddStateListener(this);

	JobStateListener* listener = fContext.GetJobStateListener();
	if (listener != NULL)
		job->AddStateListener(listener);

	return fJobQueue.AddJob(job);
}


}	// namespace Package

}	// namespace Haiku
