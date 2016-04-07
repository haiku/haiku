/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TARGET_HOST_INTERFACE_H
#define TARGET_HOST_INTERFACE_H

#include <OS.h>
#include <String.h>

#include <ObjectList.h>
#include <Referenceable.h>


class DebuggerInterface;
class Settings;
class TargetHost;
class TeamDebugger;


class TargetHostInterface : public BReferenceable {
public:
								TargetHostInterface();
	virtual						~TargetHostInterface();

			const BString&		Name() const { return fName; }
			void				SetName(const BString& name);


			int32				CountTeamDebuggers() const;
			TeamDebugger*		TeamDebuggerAt(int32 index) const;
			TeamDebugger*		FindTeamDebugger(team_id team) const;
			status_t			AddTeamDebugger(TeamDebugger* debugger);
			void				RemoveTeamDebugger(TeamDebugger* debugger);

	virtual	status_t			Init(Settings* settings) = 0;
	virtual	void				Close() = 0;

	virtual	bool				Connected() const = 0;

	virtual	TargetHost*			GetTargetHost() = 0;

	virtual	status_t			Attach(team_id id, thread_id threadID,
									DebuggerInterface*& _interface) = 0;

	virtual	status_t			CreateTeam(int commandLineArgc,
									const char* const* arguments,
									DebuggerInterface*& _interface) = 0;

private:
	static	int					_CompareDebuggers(const TeamDebugger* a,
									const TeamDebugger* b);
	static	int					_FindDebuggerByKey(const team_id* team,
									const TeamDebugger* debugger);
private:
			typedef BObjectList<TeamDebugger> TeamDebuggerList;

private:
			BString				fName;
			TeamDebuggerList	fTeamDebuggers;
};


#endif	// TARGET_HOST_INTERFACE_H
