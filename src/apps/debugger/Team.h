/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_H
#define TEAM_H

#include <Locker.h>

#include "Image.h"
#include "Thread.h"


// team event types
enum {
	TEAM_EVENT_THREAD_ADDED,
	TEAM_EVENT_THREAD_REMOVED,
	TEAM_EVENT_IMAGE_ADDED,
	TEAM_EVENT_IMAGE_REMOVED
};


class Team : public BLocker {
public:
			class Event;
			class ThreadEvent;
			class ImageEvent;
			class Listener;

public:
								Team(team_id teamID);
								~Team();

			status_t			Init();

			team_id				ID() const		{ return fID; }

			const char*			Name() const	{ return fName.String(); }
			void				SetName(const BString& name);

			void				AddThread(Thread* thread);
			status_t			AddThread(const thread_info& threadInfo,
									Thread** _thread = NULL);
			status_t			AddThread(thread_id threadID,
									Thread** _thread = NULL);
			void				RemoveThread(Thread* thread);
			bool				RemoveThread(thread_id threadID);
			Thread*				ThreadByID(thread_id threadID) const;
			const ThreadList&	Threads() const;

			void				AddImage(Image* image);
			status_t			AddImage(const image_info& imageInfo,
									Image** _image = NULL);
			void				RemoveImage(Image* image);
			bool				RemoveImage(image_id imageID);
			Image*				ImageByID(image_id imageID) const;
			const ImageList&	Images() const;

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

private:
			typedef DoublyLinkedList<Listener> ListenerList;

private:
			void				_NotifyThreadAdded(Thread* thread);
			void				_NotifyThreadRemoved(Thread* thread);
			void				_NotifyImageAdded(Image* image);
			void				_NotifyImageRemoved(Image* image);

private:
			team_id				fID;
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
};


#endif	// TEAM_H
