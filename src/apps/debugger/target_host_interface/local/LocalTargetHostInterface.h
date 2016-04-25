/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef LOCAL_TARGET_HOST_INTERFACE_H
#define LOCAL_TARGET_HOST_INTERFACE_H

#include "TargetHostInterface.h"


class LocalTargetHostInterface : public TargetHostInterface {
public:
								LocalTargetHostInterface();
	virtual						~LocalTargetHostInterface();

	virtual	status_t			Init(Settings* settings);
	virtual	void				Close();

	virtual	bool				IsLocal() const;
	virtual	bool				Connected() const;

	virtual	TargetHost*			GetTargetHost();

	virtual	status_t			Attach(team_id id, thread_id threadID,
									DebuggerInterface*& _interface) const;
	virtual	status_t			CreateTeam(int commandLineArgc,
									const char* const* arguments,
									team_id& _teamID) const;
	virtual	status_t			LoadCore(const char* coreFilePath,
									DebuggerInterface*& _interface,
									thread_id& _thread) const;

	virtual	status_t			FindTeamByThread(thread_id thread,
									team_id& _teamID) const;


private:
	static	status_t			_PortLoop(void* arg);
			status_t			_HandleTeamEvent(team_id team, int32 opcode,
									bool& addToWaiters);

private:
			TargetHost*			fTargetHost;
			port_id				fDataPort;
			thread_id			fPortWorker;
};

#endif	// LOCAL_TARGET_HOST_INTERFACE_H
