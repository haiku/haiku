/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TARGET_HOST_H
#define TARGET_HOST_H

#include <Locker.h>
#include <OS.h>
#include <String.h>

#include <ObjectList.h>
#include <Referenceable.h>
#include <util/DoublyLinkedList.h>


class TeamInfo;


class TargetHost : public BReferenceable {
public:
			class Listener;
public:
								TargetHost(const BString& name);
	virtual						~TargetHost();

			bool				Lock()			{ return fLock.Lock(); }
			void				Unlock()		{ fLock.Unlock(); }

			const BString&		Name() const	{ return fName; }

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

			int32				CountTeams() const;
			status_t			AddTeam(const team_info& info);
			void				RemoveTeam(team_id team);
			void				UpdateTeam(const team_info& info);
			TeamInfo*			TeamInfoAt(int32 index) const;
			TeamInfo*			TeamInfoByID(team_id team) const;

private:
			typedef DoublyLinkedList<Listener> ListenerList;
			typedef BObjectList<TeamInfo> TeamInfoList;

private:
	static	int 				_CompareTeams(const TeamInfo* a,
									const TeamInfo* b);
	static	int					_FindTeamByKey(const team_id* id,
									const TeamInfo* info);

			void				_NotifyTeamAdded(TeamInfo* info);
			void				_NotifyTeamRemoved(team_id team);
			void				_NotifyTeamRenamed(TeamInfo* info);

private:
			BString				fName;
			BLocker				fLock;
			ListenerList		fListeners;
			TeamInfoList		fTeams;
};


class TargetHost::Listener
	: public DoublyLinkedListLinkImpl<TargetHost::Listener> {
public:
	virtual						~Listener();

	virtual	void				TeamAdded(TeamInfo* info);
	virtual	void				TeamRemoved(team_id team);
	virtual	void				TeamRenamed(TeamInfo* info);
};

#endif	// TARGET_HOST_H
