/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TARGET_HOST_INTERFACE_H
#define TARGET_HOST_INTERFACE_H

#include <OS.h>
#include <String.h>

#include <Referenceable.h>


class DebuggerInterface;
class TargetHost;


class TargetHostInterface : public BReferenceable {
public:
	virtual						~TargetHostInterface();

	virtual	status_t			Init() = 0;
	virtual	void				Close() = 0;

			const BString&		Name() const { return fName; }
			void				SetName(const BString& name);

	virtual	bool				Connected() const = 0;

	virtual	TargetHost*			GetTargetHost() = 0;

	virtual	status_t			Attach(team_id id, thread_id threadID,
									DebuggerInterface*& _interface) = 0;

	virtual	status_t			CreateTeam(int commandLineArgc,
									const char* const* arguments,
									DebuggerInterface*& _interface) = 0;

private:
			BString				fName;
};


#endif	// TARGET_HOST_INTERFACE_H
