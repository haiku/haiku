/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include "MyJobStateListener.h"
#include "pkgman.h"


using Haiku::Package::Job;


void
MyJobStateListener::JobStarted(Job* job)
{
	printf("%s ...\n", job->Title().String());
}


void
MyJobStateListener::JobSucceeded(Job* job)
{
}


void
MyJobStateListener::JobFailed(Job* job)
{
	BString error = job->ErrorString();
	if (error.Length() > 0) {
		error.ReplaceAll("\n", "\n*** ");
		fprintf(stderr, "%s", error.String());
	}
	DIE(job->Result(), "failed!");
}


void
MyJobStateListener::JobAborted(Job* job)
{
	DIE(job->Result(), "aborted");
}
