/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_DEBUG_MODEL_H
#define TEAM_DEBUG_MODEL_H

#include "Team.h"


// team debug model event types
//enum {
//	TEAM_EVENT_THREAD_ADDED
//};


class Architecture;
class DebuggerInterface;


class TeamDebugModel {
public:
			class Event;
			class Listener;

public:
								TeamDebugModel(Team* team,
									DebuggerInterface* debuggerInterface,
									Architecture* architecture);
								~TeamDebugModel();

			status_t			Init();

			bool				Lock()		{ return fTeam->Lock(); }
			void				Unlock()	{ fTeam->Unlock(); }

			Team*				GetTeam() const	{ return fTeam; }
			DebuggerInterface*	GetDebuggerInterface() const
									{ return fDebuggerInterface; }
			Architecture*		GetArchitecture() const
									{ return fArchitecture; }

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

private:
			typedef DoublyLinkedList<Listener> ListenerList;

private:
			Team*				fTeam;
			DebuggerInterface*	fDebuggerInterface;
			Architecture*		fArchitecture;
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



class TeamDebugModel::Listener
	: public DoublyLinkedListLinkImpl<TeamDebugModel::Listener> {
public:
	virtual						~Listener();

//	virtual	void				ThreadAdded(const Team::ThreadEvent& event);
};


#endif	// TEAM_DEBUG_MODEL_H
