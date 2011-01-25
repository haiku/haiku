/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef JOB_STATE_LISTENER_H
#define JOB_STATE_LISTENER_H


#include <package/Job.h>


struct JobStateListener : public BPackageKit::BJobStateListener {
	virtual	void JobStarted(BPackageKit::BJob* job);
	virtual	void JobSucceeded(BPackageKit::BJob* job);
	virtual	void JobFailed(BPackageKit::BJob* job);
	virtual	void JobAborted(BPackageKit::BJob* job);
};


#endif	// JOB_STATE_LISTENER_H
