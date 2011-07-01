/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_H
#define TEAM_H


#include <Locker.h>

#include <ObjectList.h>

#include "Image.h"
#include "ImageInfo.h"
#include "TargetAddressRange.h"
#include "Thread.h"
#include "ThreadInfo.h"
#include "UserBreakpoint.h"


// team event types
enum {
	TEAM_EVENT_THREAD_ADDED,
	TEAM_EVENT_THREAD_REMOVED,
	TEAM_EVENT_IMAGE_ADDED,
	TEAM_EVENT_IMAGE_REMOVED,

	TEAM_EVENT_THREAD_STATE_CHANGED,
	TEAM_EVENT_THREAD_CPU_STATE_CHANGED,
	TEAM_EVENT_THREAD_STACK_TRACE_CHANGED,

	TEAM_EVENT_IMAGE_DEBUG_INFO_CHANGED,

	TEAM_EVENT_BREAKPOINT_ADDED,
	TEAM_EVENT_BREAKPOINT_REMOVED,
	TEAM_EVENT_USER_BREAKPOINT_CHANGED
};


class Architecture;
class Breakpoint;
class Function;
class FunctionID;
class FunctionInstance;
class LocatableFile;
class SourceCode;
class SourceLocation;
class Statement;
class TeamDebugInfo;
class TeamMemory;
class TeamTypeInformation;
class UserBreakpoint;


class Team {
public:
			class Event;
			class ThreadEvent;
			class ImageEvent;
			class BreakpointEvent;
			class UserBreakpointEvent;
			class Listener;

public:
								Team(team_id teamID, TeamMemory* teamMemory,
									Architecture* architecture,
									TeamDebugInfo* debugInfo,
									TeamTypeInformation* typeInformation);
								~Team();

			status_t			Init();

			bool				Lock()		{ return fLock.Lock(); }
			void				Unlock()	{ fLock.Unlock(); }

			team_id				ID() const			{ return fID; }
			TeamMemory*			GetTeamMemory() const
									{ return fTeamMemory; }
			Architecture*		GetArchitecture() const
									{ return fArchitecture; }
			TeamDebugInfo*		DebugInfo() const	{ return fDebugInfo; }
			TeamTypeInformation*
								GetTeamTypeInformation() const
									{ return fTypeInformation; }

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

			bool				AddBreakpoint(Breakpoint* breakpoint);
									// takes over reference (also on error)
			void				RemoveBreakpoint(Breakpoint* breakpoint);
									// releases its own reference
			int32				CountBreakpoints() const;
			Breakpoint*			BreakpointAt(int32 index) const;
			Breakpoint*			BreakpointAtAddress(
									target_addr_t address) const;
			void				GetBreakpointsInAddressRange(
									TargetAddressRange range,
									BObjectList<UserBreakpoint>& breakpoints)
										const;
			void				GetBreakpointsForSourceCode(
									SourceCode* sourceCode,
									BObjectList<UserBreakpoint>& breakpoints)
										const;

			void				AddUserBreakpoint(
									UserBreakpoint* userBreakpoint);
			void				RemoveUserBreakpoint(
									UserBreakpoint* userBreakpoint);
			const UserBreakpointList& UserBreakpoints() const
									{ return fUserBreakpoints; }

			status_t			GetStatementAtAddress(target_addr_t address,
									FunctionInstance*& _function,
									Statement*& _statement);
									// returns a reference to the statement,
									// not to the functions instance, though,
									// caller must lock
			status_t			GetStatementAtSourceLocation(
									SourceCode* sourceCode,
									const SourceLocation& location,
									Statement*& _statement);
									// returns a reference to the statement
									// (any matching statement!),
									// caller must lock,

			Function*			FunctionByID(FunctionID* functionID) const;

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

			// service methods for Thread
			void				NotifyThreadStateChanged(Thread* thread);
			void				NotifyThreadCpuStateChanged(Thread* thread);
			void				NotifyThreadStackTraceChanged(Thread* thread);

			// service methods for Image
			void				NotifyImageDebugInfoChanged(Image* image);

			// breakpoint related service methods
			void				NotifyUserBreakpointChanged(
									UserBreakpoint* breakpoint);

private:
			struct BreakpointByAddressPredicate;

			typedef BObjectList<Breakpoint> BreakpointList;
			typedef DoublyLinkedList<Listener> ListenerList;

private:
			void				_NotifyThreadAdded(Thread* thread);
			void				_NotifyThreadRemoved(Thread* thread);
			void				_NotifyImageAdded(Image* image);
			void				_NotifyImageRemoved(Image* image);
			void				_NotifyBreakpointAdded(Breakpoint* breakpoint);
			void				_NotifyBreakpointRemoved(
									Breakpoint* breakpoint);

private:
			BLocker				fLock;
			team_id				fID;
			TeamMemory*			fTeamMemory;
			TeamTypeInformation*
								fTypeInformation;
			Architecture*		fArchitecture;
			TeamDebugInfo*		fDebugInfo;
			BString				fName;
			ThreadList			fThreads;
			ImageList			fImages;
			BreakpointList		fBreakpoints;
			UserBreakpointList	fUserBreakpoints;
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


class Team::BreakpointEvent : public Event {
public:
								BreakpointEvent(uint32 type, Team* team,
									Breakpoint* breakpoint);

			Breakpoint*			GetBreakpoint() const	{ return fBreakpoint; }

protected:
			Breakpoint*			fBreakpoint;
};


class Team::UserBreakpointEvent : public Event {
public:
								UserBreakpointEvent(uint32 type, Team* team,
									UserBreakpoint* breakpoint);

			UserBreakpoint*		GetBreakpoint() const	{ return fBreakpoint; }

protected:
			UserBreakpoint*		fBreakpoint;
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

	virtual	void				BreakpointAdded(
									const Team::BreakpointEvent& event);
	virtual	void				BreakpointRemoved(
									const Team::BreakpointEvent& event);
	virtual	void				UserBreakpointChanged(
									const Team::UserBreakpointEvent& event);
};


#endif	// TEAM_H
