/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_H
#define TEAM_H

#include <Locker.h>

#include "Image.h"
#include "ImageInfo.h"
#include "Thread.h"
#include "ThreadInfo.h"


// team event types
enum {
	TEAM_EVENT_THREAD_ADDED,
	TEAM_EVENT_THREAD_REMOVED,
	TEAM_EVENT_IMAGE_ADDED,
	TEAM_EVENT_IMAGE_REMOVED,

	TEAM_EVENT_THREAD_STATE_CHANGED,
	TEAM_EVENT_THREAD_CPU_STATE_CHANGED,
	TEAM_EVENT_THREAD_STACK_TRACE_CHANGED,

	TEAM_EVENT_IMAGE_DEBUG_INFO_CHANGED
};


class FunctionInstance;
class LocatableFile;
class Statement;
class TeamDebugInfo;


class Team : public BLocker {
public:
			class Event;
			class ThreadEvent;
			class ImageEvent;
			class Listener;

public:
								Team(team_id teamID, TeamDebugInfo* debugInfo);
								~Team();

			status_t			Init();

			team_id				ID() const			{ return fID; }
			TeamDebugInfo*		DebugInfo() const	{ return fDebugInfo; }

			const char*			Name() const	{ return fName.String(); }
			void				SetName(const BString& name);

			void				AddThread(Thread* thread);
			status_t			AddThread(const ThreadInfo& threadInfo,
									Thread** _thread = NULL);
			void				RemoveThread(Thread* thread);
			bool				RemoveThread(thread_id threadID);
			Thread*				ThreadByID(thread_id threadID) const;
			const ThreadList&	Threads() const;

			status_t			AddImage(const ImageInfo& imageInfo,
									LocatableFile* imageFile,
									Image** _image = NULL);
			void				RemoveImage(Image* image);
			bool				RemoveImage(image_id imageID);
			Image*				ImageByID(image_id imageID) const;
			Image*				ImageByAddress(target_addr_t address) const;
			const ImageList&	Images() const;

			status_t			GetStatementAtAddress(target_addr_t address,
									FunctionInstance*& _function,
									Statement*& _statement);
									// returns a reference to the statement,
									// not to the functions instance, though,
									// caller must lock

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

			// service methods for Thread
			void				NotifyThreadStateChanged(Thread* thread);
			void				NotifyThreadCpuStateChanged(Thread* thread);
			void				NotifyThreadStackTraceChanged(Thread* thread);

			// service methods for Image
			void				NotifyImageDebugInfoChanged(Image* image);

private:
			typedef DoublyLinkedList<Listener> ListenerList;

private:
			void				_NotifyThreadAdded(Thread* thread);
			void				_NotifyThreadRemoved(Thread* thread);
			void				_NotifyImageAdded(Image* image);
			void				_NotifyImageRemoved(Image* image);

private:
			team_id				fID;
			TeamDebugInfo*		fDebugInfo;
			BString				fName;
			ThreadList			fThreads;
			ImageList			fImages;
			ListenerList		fListeners;
};


class Team::Event {
public:
								Event(uint32 type, Team* team);

			uint32				EventType() const	{ return fEventType; }
			Team*				GetTeam() const		{ return fTeam; }

protected:
			uint32				fEventType;
			Team*				fTeam;
};


class Team::ThreadEvent : public Event {
public:
								ThreadEvent(uint32 type, Thread* thread);

			Thread*				GetThread() const	{ return fThread; }

protected:
			Thread*				fThread;
};


class Team::ImageEvent : public Event {
public:
								ImageEvent(uint32 type, Image* image);

			Image*				GetImage() const	{ return fImage; }

protected:
			Image*				fImage;
};


class Team::Listener : public DoublyLinkedListLinkImpl<Team::Listener> {
public:
	virtual						~Listener();

	virtual	void				ThreadAdded(const Team::ThreadEvent& event);
	virtual	void				ThreadRemoved(const Team::ThreadEvent& event);

	virtual	void				ImageAdded(const Team::ImageEvent& event);
	virtual	void				ImageRemoved(const Team::ImageEvent& event);

	virtual	void				ThreadStateChanged(
									const Team::ThreadEvent& event);
	virtual	void				ThreadCpuStateChanged(
									const Team::ThreadEvent& event);
	virtual	void				ThreadStackTraceChanged(
									const Team::ThreadEvent& event);

	virtual	void				ImageDebugInfoChanged(
									const Team::ImageEvent& event);
};


#endif	// TEAM_H
