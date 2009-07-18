/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_DEBUG_MODEL_H
#define TEAM_DEBUG_MODEL_H

#include <ObjectList.h>

#include "Breakpoint.h"
#include "TargetAddressRange.h"
#include "Team.h"


// team debug model event types
enum {
	TEAM_DEBUG_MODEL_EVENT_BREAKPOINT_ADDED,
	TEAM_DEBUG_MODEL_EVENT_BREAKPOINT_REMOVED,
	TEAM_DEBUG_MODEL_EVENT_USER_BREAKPOINT_CHANGED
};


class Architecture;
class Breakpoint;
class FunctionID;
class SourceCode;
class TeamMemory;
class UserBreakpoint;


class TeamDebugModel {
public:
			class Event;
			class BreakpointEvent;
			class Listener;

public:
								TeamDebugModel(Team* team,
									TeamMemory* teamMemory,
									Architecture* architecture);
								~TeamDebugModel();

			status_t			Init();

			bool				Lock()		{ return fTeam->Lock(); }
			void				Unlock()	{ fTeam->Unlock(); }

			Team*				GetTeam() const	{ return fTeam; }
			TeamMemory*			GetTeamMemory() const
									{ return fTeamMemory; }
			Architecture*		GetArchitecture() const
									{ return fArchitecture; }

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

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

			void				NotifyUserBreakpointChanged(
									Breakpoint* breakpoint);

private:
			struct BreakpointByAddressPredicate;

			typedef BObjectList<Breakpoint> BreakpointList;
			typedef DoublyLinkedList<Listener> ListenerList;

private:
			void				_NotifyBreakpointAdded(Breakpoint* breakpoint);
			void				_NotifyBreakpointRemoved(
									Breakpoint* breakpoint);

private:
			Team*				fTeam;
			TeamMemory*			fTeamMemory;
			Architecture*		fArchitecture;
			BreakpointList		fBreakpoints;
			ListenerList		fListeners;
};


class TeamDebugModel::Event {
public:
								Event(uint32 type, TeamDebugModel* model);

			uint32				EventType() const	{ return fEventType; }
			TeamDebugModel*		Model() const		{ return fModel; }

protected:
			uint32				fEventType;
			TeamDebugModel*		fModel;
};


class TeamDebugModel::BreakpointEvent : public Event {
public:
								BreakpointEvent(uint32 type,
									TeamDebugModel* model,
									Breakpoint* breakpoint);

			Breakpoint*			GetBreakpoint() const	{ return fBreakpoint; }

protected:
			Breakpoint*			fBreakpoint;
};


class TeamDebugModel::Listener
	: public DoublyLinkedListLinkImpl<TeamDebugModel::Listener> {
public:
	virtual						~Listener();

	virtual	void				BreakpointAdded(
									const TeamDebugModel::BreakpointEvent&
										event);
	virtual	void				BreakpointRemoved(
									const TeamDebugModel::BreakpointEvent&
										event);
	virtual	void				UserBreakpointChanged(
									const TeamDebugModel::BreakpointEvent&
										event);
};


#endif	// TEAM_DEBUG_MODEL_H
