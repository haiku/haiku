/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_H
#define THREAD_H

#include <OS.h>
#include <String.h>

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>


class Team;


enum {
	THREAD_STATE_UNKNOWN,
	THREAD_STATE_RUNNING,
	THREAD_STATE_STOPPED
};


class Thread : public Referenceable, public DoublyLinkedListLinkImpl<Thread> {
public:
								Thread(Team* team, thread_id threadID);
								~Thread();

			status_t			Init();

			Team*				GetTeam() const	{ return fTeam; }
			thread_id			ID() const		{ return fID; }

			const char*			Name() const	{ return fName.String(); }
			void				SetName(const BString& name);

			uint32				State() const	{ return fState; }
			void				SetState(uint32 state);

private:
			Team*				fTeam;
			thread_id			fID;
			BString				fName;
			uint32				fState;
};


typedef DoublyLinkedList<Thread> ThreadList;


#endif	// THREAD_H
