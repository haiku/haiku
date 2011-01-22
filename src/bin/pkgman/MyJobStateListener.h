/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef MY_JOB_STATE_LISTENER_H
#define MY_JOB_STATE_LISTENER_H


#include <package/Job.h>


struct MyJobStateListener : public Haiku::Package::JobStateListener {
	virtual	void JobStarted(Haiku::Package::Job* job);
	virtual	void JobSucceeded(Haiku::Package::Job* job);
	virtual	void JobFailed(Haiku::Package::Job* job);
	virtual	void JobAborted(Haiku::Package::Job* job);
};


#endif	// MY_JOB_STATE_LISTENER_H
