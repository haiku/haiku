/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef JOB_STATE_LISTENER_H
#define JOB_STATE_LISTENER_H


#include <Job.h>


class JobStateListener : public BSupportKit::BJobStateListener {
public:
								JobStateListener();

	virtual	void				JobStarted(BSupportKit::BJob* job);
	virtual	void				JobSucceeded(BSupportKit::BJob* job);
	virtual	void				JobFailed(BSupportKit::BJob* job);
	virtual	void				JobAborted(BSupportKit::BJob* job);
};


#endif	// JOB_STATE_LISTENER_H
