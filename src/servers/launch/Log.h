/*
 * Copyright 2017-2018, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LOG_H
#define LOG_H


#include <String.h>

#include <locks.h>
#include <util/DoublyLinkedList.h>


class BMessage;

class BaseJob;
class Event;
class Job;


enum LogItemType {
	kJobInitialized,
	kJobIgnored,
	kJobSkipped,
	kJobLaunched,
	kJobTerminated,
	kJobEnabled,
	kJobStopped,
	kEvent,
	kExternalEvent,
	kExternalEventRegistered,
	kExternalEventUnregistered,
};


class LogItem : public DoublyLinkedListLinkImpl<LogItem> {
public:
public:
								LogItem();
	virtual						~LogItem();

			bigtime_t			When()
									{ return fWhen; }
			BString				Message() const;

	virtual	LogItemType			Type() const = 0;
	virtual status_t			GetMessage(BString& target) const = 0;
	virtual status_t			GetParameter(BMessage& parameter) const = 0;
	virtual	bool				Matches(const char* jobName,
									const char* eventName) = 0;

private:
			bigtime_t			fWhen;
};


typedef DoublyLinkedList<LogItem> LogItemList;


class Log {
public:
								Log();

			void				Add(LogItem* item);

			LogItemList::Iterator
								Iterator()
									{ return fItems.GetIterator(); }

			mutex&				Lock()
									{ return fLock; }

			void				JobInitialized(Job* job);
			void				JobIgnored(Job* job, status_t status);

			void				JobSkipped(Job* job);
			void				JobLaunched(Job* job, status_t status);
			void				JobTerminated(Job* job, status_t status);

			void				JobEnabled(Job* job, bool enabled);
			void				JobStopped(BaseJob* job, bool force);

			void				EventTriggered(BaseJob* job, Event* event);

			void				ExternalEventTriggered(const char* name);
			void				ExternalEventRegistered(const char* name);
			void				ExternalEventUnregistered(const char* name);

private:
			mutex				fLock;
			LogItemList			fItems;
			size_t				fCount;
};


#endif // LOG_H
