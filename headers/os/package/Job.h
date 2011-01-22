/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__JOB_H_
#define _HAIKU__PACKAGE__JOB_H_


#include <ObjectList.h>
#include <String.h>


namespace Haiku {

namespace Package {


class Context;
class Job;

struct JobStateListener {
	virtual						~JobStateListener();

								// these default implementations do nothing
	virtual	void				JobStarted(Job* job);
	virtual	void				JobSucceeded(Job* job);
	virtual	void				JobFailed(Job* job);
	virtual	void				JobAborted(Job* job);
};


enum JobState {
	JOB_STATE_WAITING_TO_RUN,
	JOB_STATE_RUNNING,
	JOB_STATE_SUCCEEDED,
	JOB_STATE_FAILED,
	JOB_STATE_ABORTED,
};


class Job {
public:
								Job(const Context& context,
									const BString& title);
	virtual						~Job();

			status_t			InitCheck() const;

	virtual	status_t			Run();

			const BString&		Title() const;
			JobState			State() const;
			status_t			Result() const;

			status_t			AddStateListener(JobStateListener* listener);
			status_t			RemoveStateListener(
									JobStateListener* listener);

			status_t			AddDependency(Job* job);
			status_t			RemoveDependency(Job* job);
			int32				CountDependencies() const;

			Job*				DependantJobAt(int32 index) const;
protected:
	virtual	status_t			Execute() = 0;
	virtual	void				Cleanup(status_t jobResult);

			void				NotifyStateListeners();

			const Context&		fContext;
private:
			status_t			fInitStatus;
			BString				fTitle;

			JobState			fState;
			status_t			fResult;

	typedef	BObjectList<Job>	JobList;
			JobList				fDependencies;
			JobList				fDependantJobs;

	typedef	BObjectList<JobStateListener>	StateListenerList;
			StateListenerList	fStateListeners;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__JOB_H_
