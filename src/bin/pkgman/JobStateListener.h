/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef JOB_STATE_LISTENER_H
#define JOB_STATE_LISTENER_H


#include <package/Job.h>


class JobStateListener : public BPackageKit::BJobStateListener {
public:
			enum {
				EXIT_ON_ERROR	= 0x01,
				EXIT_ON_ABORT	= 0x02,
			};


public:
								JobStateListener(
									uint32 flags = EXIT_ON_ERROR
										| EXIT_ON_ABORT);

	virtual	void				JobStarted(BPackageKit::BJob* job);
	virtual	void				JobSucceeded(BPackageKit::BJob* job);
	virtual	void				JobFailed(BPackageKit::BJob* job);
	virtual	void				JobAborted(BPackageKit::BJob* job);

private:
			uint32				fFlags;
};


#endif	// JOB_STATE_LISTENER_H
