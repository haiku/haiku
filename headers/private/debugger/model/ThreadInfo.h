/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_INFO_H
#define THREAD_INFO_H

#include <OS.h>
#include <String.h>


class ThreadInfo {
public:
								ThreadInfo();
								ThreadInfo(const ThreadInfo& other);
								ThreadInfo(team_id team, thread_id thread,
									const BString& name);

			void				SetTo(team_id team, thread_id thread,
									const BString& name);

			team_id				TeamID() const		{ return fTeam; }
			thread_id			ThreadID() const	{ return fThread; }
			const char*			Name() const	{ return fName.String(); }

private:
			team_id				fTeam;
			thread_id			fThread;
			BString				fName;
};


#endif	// THREAD_INFO_H
