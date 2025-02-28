/*
 * Copyright 2017-2018, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Log.h"

#include <OS.h>

#include "Events.h"
#include "Job.h"


const size_t kMaxItems = 10000;


class AbstractJobLogItem : public LogItem {
public:
								AbstractJobLogItem(BaseJob* job);
	virtual						~AbstractJobLogItem();

	virtual status_t			GetParameter(BMessage& parameter) const;
	virtual	bool				Matches(const char* jobName,
									const char* eventName);

protected:
			BaseJob*			fJob;
};


class JobInitializedLogItem : public AbstractJobLogItem {
public:
								JobInitializedLogItem(Job* job);
	virtual						~JobInitializedLogItem();

	virtual	LogItemType			Type() const;
	virtual status_t			GetMessage(BString& target) const;
};


class JobIgnoredLogItem : public LogItem {
public:
								JobIgnoredLogItem(Job* job, status_t error);
	virtual						~JobIgnoredLogItem();

	virtual	LogItemType			Type() const;
	virtual status_t			GetMessage(BString& target) const;
	virtual status_t			GetParameter(BMessage& parameter) const;
	virtual	bool				Matches(const char* jobName,
									const char* eventName);

private:
			BString				fJobName;
			status_t			fError;
};


class JobSkippedLogItem : public LogItem {
public:
								JobSkippedLogItem(Job* job);
	virtual						~JobSkippedLogItem();

	virtual	LogItemType			Type() const;
	virtual status_t			GetMessage(BString& target) const;
	virtual status_t			GetParameter(BMessage& parameter) const;
	virtual	bool				Matches(const char* jobName,
									const char* eventName);

private:
			BString				fJobName;
};


class JobLaunchedLogItem : public AbstractJobLogItem {
public:
								JobLaunchedLogItem(Job* job, status_t status);
	virtual						~JobLaunchedLogItem();

	virtual	LogItemType			Type() const;
	virtual status_t			GetMessage(BString& target) const;
	virtual status_t			GetParameter(BMessage& parameter) const;

private:
			status_t			fStatus;
};


class JobTerminatedLogItem : public AbstractJobLogItem {
public:
								JobTerminatedLogItem(Job* job, status_t status);
	virtual						~JobTerminatedLogItem();

	virtual	LogItemType			Type() const;
	virtual status_t			GetMessage(BString& target) const;
	virtual status_t			GetParameter(BMessage& parameter) const;

private:
			status_t			fStatus;
};


class JobEnabledLogItem : public AbstractJobLogItem {
public:
								JobEnabledLogItem(Job* job, bool enabled);
	virtual						~JobEnabledLogItem();

	virtual	LogItemType			Type() const;
	virtual status_t			GetMessage(BString& target) const;
	virtual status_t			GetParameter(BMessage& parameter) const;

private:
			bool				fEnabled;
};


class JobStoppedLogItem : public AbstractJobLogItem {
public:
								JobStoppedLogItem(BaseJob* job, bool force);
	virtual						~JobStoppedLogItem();

	virtual	LogItemType			Type() const;
	virtual status_t			GetMessage(BString& target) const;
	virtual status_t			GetParameter(BMessage& parameter) const;

private:
			bool				fForce;
};


class EventLogItem : public AbstractJobLogItem {
public:
								EventLogItem(BaseJob* job, Event* event);
	virtual						~EventLogItem();

	virtual	LogItemType			Type() const;
	virtual status_t			GetMessage(BString& target) const;
	virtual status_t			GetParameter(BMessage& parameter) const;
	virtual	bool				Matches(const char* jobName,
									const char* eventName);

private:
			Event*				fEvent;
};


class AbstractExternalEventLogItem : public LogItem {
public:
								AbstractExternalEventLogItem(const char* name);
	virtual						~AbstractExternalEventLogItem();

	virtual status_t			GetParameter(BMessage& parameter) const;
	virtual	bool				Matches(const char* jobName,
									const char* eventName);

protected:
			BString				fEventName;
};


class ExternalEventLogItem : public AbstractExternalEventLogItem {
public:
								ExternalEventLogItem(const char* name);
	virtual						~ExternalEventLogItem();

	virtual	LogItemType			Type() const;
	virtual status_t			GetMessage(BString& target) const;
};


class ExternalEventRegisteredLogItem : public AbstractExternalEventLogItem {
public:
								ExternalEventRegisteredLogItem(
									const char* name);
	virtual						~ExternalEventRegisteredLogItem();

	virtual	LogItemType			Type() const;
	virtual status_t			GetMessage(BString& target) const;
};


class ExternalEventUnregisteredLogItem : public AbstractExternalEventLogItem {
public:
								ExternalEventUnregisteredLogItem(
									const char* name);
	virtual						~ExternalEventUnregisteredLogItem();

	virtual	LogItemType			Type() const;
	virtual status_t			GetMessage(BString& target) const;
};


// #pragma mark -


LogItem::LogItem()
	:
	fWhen(system_time())
{
}


LogItem::~LogItem()
{
}


BString
LogItem::Message() const
{
	BString message;
	GetMessage(message);
	return message;
}


// #pragma mark - Log


Log::Log()
	:
	fCount(0)
{
	mutex_init(&fLock, "log lock");
}


void
Log::Add(LogItem* item)
{
	MutexLocker locker(fLock);
	if (fCount == kMaxItems)
		fItems.Remove(fItems.First());
	else
		fCount++;

	fItems.Add(item);
}


void
Log::JobInitialized(Job* job)
{
	LogItem* item = new(std::nothrow) JobInitializedLogItem(job);
	if (item != NULL)
		Add(item);
	else
		debug_printf("Initialized job \"%s\"\n", job->Name());
}


void
Log::JobIgnored(Job* job, status_t status)
{
	LogItem* item = new(std::nothrow) JobIgnoredLogItem(job, status);
	if (item != NULL)
		Add(item);
	else {
		debug_printf("Ignored job \"%s\": %s\n", job->Name(),
			strerror(status));
	}
}


void
Log::JobSkipped(Job* job)
{
	LogItem* item = new(std::nothrow) JobSkippedLogItem(job);
	if (item != NULL)
		Add(item);
	else {
		debug_printf("Skipped job \"%s\"\n", job->Name());
	}
}


void
Log::JobLaunched(Job* job, status_t status)
{
	LogItem* item = new(std::nothrow) JobLaunchedLogItem(job, status);
	if (item != NULL)
		Add(item);
	else {
		debug_printf("Launched job \"%s\": %s\n", job->Name(),
			strerror(status));
	}
}


void
Log::JobTerminated(Job* job, status_t status)
{
	LogItem* item = new(std::nothrow) JobTerminatedLogItem(job, status);
	if (item != NULL)
		Add(item);
	else {
		debug_printf("Terminated job \"%s\": %s\n", job->Name(),
			strerror(status));
	}
}


void
Log::JobEnabled(Job* job, bool enabled)
{
	LogItem* item = new(std::nothrow) JobEnabledLogItem(job, enabled);
	if (item != NULL)
		Add(item);
	else
		debug_printf("Enabled job \"%s\": %d\n", job->Name(), enabled);
}


void
Log::JobStopped(BaseJob* job, bool force)
{
	LogItem* item = new(std::nothrow) JobStoppedLogItem(job, force);
	if (item != NULL)
		Add(item);
	else
		debug_printf("Stopped job \"%s\"\n", job->Name());
}


void
Log::EventTriggered(BaseJob* job, Event* event)
{
	LogItem* item = new(std::nothrow) EventLogItem(job, event);
	if (item != NULL)
		Add(item);
	else {
		debug_printf("Event triggered for \"%s\": %s\n", job->Name(),
			event->ToString().String());
	}
}


void
Log::ExternalEventTriggered(const char* name)
{
	LogItem* item = new(std::nothrow) ExternalEventLogItem(name);
	if (item != NULL)
		Add(item);
	else
		debug_printf("External event triggered: %s\n", name);
}


void
Log::ExternalEventRegistered(const char* name)
{
	LogItem* item = new(std::nothrow) ExternalEventRegisteredLogItem(name);
	if (item != NULL)
		Add(item);
	else
		debug_printf("External event registered: %s\n", name);
}


void
Log::ExternalEventUnregistered(const char* name)
{
	LogItem* item = new(std::nothrow) ExternalEventUnregisteredLogItem(name);
	if (item != NULL)
		Add(item);
	else
		debug_printf("External event unregistered: %s\n", name);
}


// #pragma mark - AbstractJobLogItem


AbstractJobLogItem::AbstractJobLogItem(BaseJob* job)
	:
	fJob(job)
{
}


AbstractJobLogItem::~AbstractJobLogItem()
{
}


status_t
AbstractJobLogItem::GetParameter(BMessage& parameter) const
{
	return parameter.AddString("job", fJob->Name());
}


bool
AbstractJobLogItem::Matches(const char* jobName, const char* eventName)
{
	if (jobName == NULL && eventName == NULL)
		return true;

	if (jobName != NULL && strcmp(fJob->Name(), jobName) == 0)
		return true;

	return false;
}


// #pragma mark - JobInitializedLogItem


JobInitializedLogItem::JobInitializedLogItem(Job* job)
	:
	AbstractJobLogItem(job)
{
}


JobInitializedLogItem::~JobInitializedLogItem()
{
}


LogItemType
JobInitializedLogItem::Type() const
{
	return kJobInitialized;
}


status_t
JobInitializedLogItem::GetMessage(BString& target) const
{
	target.SetToFormat("Job \"%s\" initialized.", fJob->Name());
	return B_OK;
}


// #pragma mark - JobIgnoredLogItem


JobIgnoredLogItem::JobIgnoredLogItem(Job* job, status_t error)
	:
	fJobName(job->Name()),
	fError(error)
{
}


JobIgnoredLogItem::~JobIgnoredLogItem()
{
}


LogItemType
JobIgnoredLogItem::Type() const
{
	return kJobIgnored;
}


status_t
JobIgnoredLogItem::GetMessage(BString& target) const
{
	target.SetToFormat("Ignored job \"%s\" due %s", fJobName.String(),
		strerror(fError));
	return B_OK;
}


status_t
JobIgnoredLogItem::GetParameter(BMessage& parameter) const
{
	status_t status = parameter.AddString("job", fJobName);
	if (status == B_OK)
		status = parameter.AddInt32("error", fError);
	return status;
}


bool
JobIgnoredLogItem::Matches(const char* jobName, const char* eventName)
{
	if (jobName == NULL && eventName == NULL)
		return true;

	if (jobName != NULL && fJobName == jobName)
		return true;

	return false;
}


// #pragma mark - JobSkippedLogItem


JobSkippedLogItem::JobSkippedLogItem(Job* job)
	:
	fJobName(job->Name())
{
}


JobSkippedLogItem::~JobSkippedLogItem()
{
}


LogItemType
JobSkippedLogItem::Type() const
{
	return kJobSkipped;
}


status_t
JobSkippedLogItem::GetMessage(BString& target) const
{
	target.SetToFormat("Skipped job \"%s\"", fJobName.String());
	return B_OK;
}


status_t
JobSkippedLogItem::GetParameter(BMessage& parameter) const
{
	return parameter.AddString("job", fJobName);
}


bool
JobSkippedLogItem::Matches(const char* jobName, const char* eventName)
{
	if (jobName == NULL && eventName == NULL)
		return true;

	if (jobName != NULL && fJobName == jobName)
		return true;

	return false;
}


// #pragma mark - JobLaunchedLogItem


JobLaunchedLogItem::JobLaunchedLogItem(Job* job, status_t status)
	:
	AbstractJobLogItem(job),
	fStatus(status)
{
}


JobLaunchedLogItem::~JobLaunchedLogItem()
{
}


LogItemType
JobLaunchedLogItem::Type() const
{
	return kJobLaunched;
}


status_t
JobLaunchedLogItem::GetMessage(BString& target) const
{
	target.SetToFormat("Job \"%s\" launched: %s", fJob->Name(),
		strerror(fStatus));
	return B_OK;
}


status_t
JobLaunchedLogItem::GetParameter(BMessage& parameter) const
{
	status_t status = AbstractJobLogItem::GetParameter(parameter);
	if (status == B_OK)
		status = parameter.AddInt32("status", fStatus);
	return status;
}


// #pragma mark - JobTerminatedLogItem


JobTerminatedLogItem::JobTerminatedLogItem(Job* job, status_t status)
	:
	AbstractJobLogItem(job),
	fStatus(status)
{
}


JobTerminatedLogItem::~JobTerminatedLogItem()
{
}


LogItemType
JobTerminatedLogItem::Type() const
{
	return kJobTerminated;
}


status_t
JobTerminatedLogItem::GetMessage(BString& target) const
{
	target.SetToFormat("Job \"%s\" terminated: %s", fJob->Name(),
		strerror(fStatus));
	return B_OK;
}


status_t
JobTerminatedLogItem::GetParameter(BMessage& parameter) const
{
	status_t status = AbstractJobLogItem::GetParameter(parameter);
	if (status == B_OK)
		status = parameter.AddInt32("status", fStatus);
	return status;
}


// #pragma mark - JobEnabledLogItem


JobEnabledLogItem::JobEnabledLogItem(Job* job, bool enabled)
	:
	AbstractJobLogItem(job),
	fEnabled(enabled)
{
}


JobEnabledLogItem::~JobEnabledLogItem()
{
}


LogItemType
JobEnabledLogItem::Type() const
{
	return kJobEnabled;
}


status_t
JobEnabledLogItem::GetMessage(BString& target) const
{
	target.SetToFormat("Job \"%s\" %sabled", fJob->Name(),
		fEnabled ? "en" : "dis");
	return B_OK;
}


status_t
JobEnabledLogItem::GetParameter(BMessage& parameter) const
{
	status_t status = AbstractJobLogItem::GetParameter(parameter);
	if (status == B_OK)
		status = parameter.AddBool("enabled", fEnabled);
	return status;
}


// #pragma mark - JobStoppedLogItem


JobStoppedLogItem::JobStoppedLogItem(BaseJob* job, bool force)
	:
	AbstractJobLogItem(job),
	fForce(force)
{
}


JobStoppedLogItem::~JobStoppedLogItem()
{
}


LogItemType
JobStoppedLogItem::Type() const
{
	return kJobStopped;
}


status_t
JobStoppedLogItem::GetMessage(BString& target) const
{
	target.SetToFormat("Job \"%s\" %sstopped", fJob->Name(),
		fForce ? "force " : "");
	return B_OK;
}


status_t
JobStoppedLogItem::GetParameter(BMessage& parameter) const
{
	status_t status = AbstractJobLogItem::GetParameter(parameter);
	if (status == B_OK)
		status = parameter.AddBool("force", fForce);
	return status;
}


// #pragma mark - EventLogItem


EventLogItem::EventLogItem(BaseJob* job, Event* event)
	:
	AbstractJobLogItem(job),
	fEvent(event)
{
}


EventLogItem::~EventLogItem()
{
}


LogItemType
EventLogItem::Type() const
{
	return kEvent;
}


status_t
EventLogItem::GetMessage(BString& target) const
{
	target.SetToFormat("Event triggered \"%s\": \"%s\"", fJob->Name(),
		fEvent->ToString().String());
	return B_OK;
}


status_t
EventLogItem::GetParameter(BMessage& parameter) const
{
	status_t status = AbstractJobLogItem::GetParameter(parameter);
	if (status == B_OK)
		status = parameter.AddString("event", fEvent->ToString());
	return status;
}


bool
EventLogItem::Matches(const char* jobName, const char* eventName)
{
	if (eventName != NULL && strstr(fEvent->ToString(), eventName) == NULL)
		return false;

	return AbstractJobLogItem::Matches(jobName, NULL);
}


// #pragma mark - ExternalEventLogItem


AbstractExternalEventLogItem::AbstractExternalEventLogItem(const char* name)
	:
	fEventName(name)
{
}


AbstractExternalEventLogItem::~AbstractExternalEventLogItem()
{
}


status_t
AbstractExternalEventLogItem::GetParameter(BMessage& parameter) const
{
	return parameter.AddString("event", fEventName);
}


bool
AbstractExternalEventLogItem::Matches(const char* jobName,
	const char* eventName)
{
	if (jobName == NULL && eventName == NULL)
		return true;

	if (eventName != NULL && strstr(fEventName.String(), eventName) != NULL)
		return true;

	return false;
}


// #pragma mark - ExternalEventLogItem


ExternalEventLogItem::ExternalEventLogItem(const char* name)
	:
	AbstractExternalEventLogItem(name)
{
}


ExternalEventLogItem::~ExternalEventLogItem()
{
}


LogItemType
ExternalEventLogItem::Type() const
{
	return kExternalEvent;
}


status_t
ExternalEventLogItem::GetMessage(BString& target) const
{
	target.SetToFormat("External event triggered: \"%s\"",
		fEventName.String());
	return B_OK;
}


// #pragma mark - ExternalEventRegisteredLogItem


ExternalEventRegisteredLogItem::ExternalEventRegisteredLogItem(const char* name)
	:
	AbstractExternalEventLogItem(name)
{
}


ExternalEventRegisteredLogItem::~ExternalEventRegisteredLogItem()
{
}


LogItemType
ExternalEventRegisteredLogItem::Type() const
{
	return kExternalEventRegistered;
}


status_t
ExternalEventRegisteredLogItem::GetMessage(BString& target) const
{
	target.SetToFormat("External event registered: \"%s\"",
		fEventName.String());
	return B_OK;
}


// #pragma mark - ExternalEventUnregisteredLogItem


ExternalEventUnregisteredLogItem::ExternalEventUnregisteredLogItem(
	const char* name)
	:
	AbstractExternalEventLogItem(name)
{
}


ExternalEventUnregisteredLogItem::~ExternalEventUnregisteredLogItem()
{
}


LogItemType
ExternalEventUnregisteredLogItem::Type() const
{
	return kExternalEventUnregistered;
}


status_t
ExternalEventUnregisteredLogItem::GetMessage(BString& target) const
{
	target.SetToFormat("External event unregistered: \"%s\"",
		fEventName.String());
	return B_OK;
}
