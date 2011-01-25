/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include "JobStateListener.h"
#include "pkgman.h"


using BPackageKit::BJob;


void
JobStateListener::JobStarted(BJob* job)
{
	printf("%s ...\n", job->Title().String());
}


void
JobStateListener::JobSucceeded(BJob* job)
{
}


void
JobStateListener::JobFailed(BJob* job)
{
	BString error = job->ErrorString();
	if (error.Length() > 0) {
		error.ReplaceAll("\n", "\n*** ");
		fprintf(stderr, "%s", error.String());
	}
	DIE(job->Result(), "failed!");
}


void
JobStateListener::JobAborted(BJob* job)
{
	DIE(job->Result(), "aborted");
}
