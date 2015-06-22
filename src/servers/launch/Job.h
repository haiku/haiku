/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef JOB_H
#define JOB_H


#include "BaseJob.h"

#include <map>
#include <set>

#include <OS.h>
#include <StringList.h>


using namespace BSupportKit;
class BMessage;

class Finder;
class Target;


typedef std::map<BString, BMessage> PortMap;


class Job : public BaseJob {
public:
								Job(const char* name);
								Job(const Job& other);
	virtual						~Job();

			bool				IsEnabled() const;
			void				SetEnabled(bool enable);

			bool				IsService() const;
			void				SetService(bool service);

			bool				CreateDefaultPort() const;
			void				SetCreateDefaultPort(bool createPort);

			void				AddPort(BMessage& data);

			bool				LaunchInSafeMode() const;
			void				SetLaunchInSafeMode(bool launch);

			const BStringList&	Arguments() const;
			BStringList&		Arguments();
			void				AddArgument(const char* argument);

			::Target*			Target() const;
			void				SetTarget(::Target* target);

			const BStringList&	Requirements() const;
			BStringList&		Requirements();
			void				AddRequirement(const char* requirement);

			status_t			Init(const Finder& jobs,
									std::set<BString>& dependencies);
			status_t			InitCheck() const;

			team_id				Team() const;

			const PortMap&		Ports() const;
			port_id				Port(const char* name = NULL) const;

			status_t			Launch();
			bool				IsLaunched() const;

protected:
	virtual	status_t			Execute();

private:
			Job&				operator=(const Job& other);
			void				_DeletePorts();
			status_t			_AddRequirement(BJob* dependency);

private:
			BStringList			fArguments;
			BStringList			fRequirements;
			bool				fEnabled;
			bool				fService;
			bool				fCreateDefaultPort;
			bool				fLaunchInSafeMode;
			PortMap				fPortMap;
			status_t			fInitStatus;
			team_id				fTeam;
			::Target*			fTarget;
			::Condition*		fCondition;
};


class Finder {
public:
	virtual	Job*				FindJob(const char* name) const = 0;
	virtual	Target*				FindTarget(const char* name) const = 0;
};


#endif // JOB_H
