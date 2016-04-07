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

	virtual	bool				Connected() const;

	virtual	TargetHost*			GetTargetHost();

	virtual	status_t			Attach(team_id id, thread_id threadID,
									DebuggerInterface*& _interface);

	virtual	status_t			CreateTeam(int commandLineArgc,
									const char* const* arguments,
									DebuggerInterface*& _interface);

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
