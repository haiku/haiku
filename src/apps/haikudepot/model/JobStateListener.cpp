/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "JobStateListener.h"


using BSupportKit::BJob;


JobStateListener::JobStateListener()
{
}


void
JobStateListener::JobStarted(BJob* job)
{
}


void
JobStateListener::JobSucceeded(BJob* job)
{
	// TODO: notify user
}


void
JobStateListener::JobFailed(BJob* job)
{
	// TODO: notify user
}


void
JobStateListener::JobAborted(BJob* job)
{
	// TODO: notify user
}
