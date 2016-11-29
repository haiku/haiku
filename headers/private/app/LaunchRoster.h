/*
 * Copyright 2015-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LAUNCH_ROSTER_H
#define _LAUNCH_ROSTER_H


#include <Messenger.h>


// Flags for RegisterEvent()
enum {
	B_STICKY_EVENT	= 0x01
};


class BLaunchRoster {
public:
								BLaunchRoster();
								~BLaunchRoster();

			status_t			InitCheck() const;

			status_t			GetData(BMessage& data);
			status_t			GetData(const char* signature, BMessage& data);
			port_id				GetPort(const char* name = NULL);
			port_id				GetPort(const char* signature,
									const char* name);

			status_t			Target(const char* name, const BMessage& data,
									const char* baseName = NULL);
			status_t			Target(const char* name,
									const BMessage* data = NULL,
									const char* baseName = NULL);
			status_t			StopTarget(const char* name,
									bool force = false);

			status_t			Start(const char* name);
			status_t			Stop(const char* name, bool force = false);
			status_t			SetEnabled(const char* name, bool enabled);

			status_t			StartSession(const char* login);

			status_t			RegisterEvent(const BMessenger& source,
									const char* name, uint32 flags);
			status_t			UnregisterEvent(const BMessenger& source,
									const char* name);
			status_t			NotifyEvent(const BMessenger& source,
									const char* name);
			status_t			ResetStickyEvent(const BMessenger& source,
									const char* name);

			status_t			GetTargets(BStringList& targets);
			status_t			GetTargetInfo(const char* name, BMessage& info);
			status_t			GetJobs(const char* target, BStringList& jobs);
			status_t			GetJobInfo(const char* name, BMessage& info);

	class Private;

private:
	friend class Private;

			void				_InitMessenger();
			status_t			_SendRequest(BMessage& request);
			status_t			_SendRequest(BMessage& request,
									BMessage& reply);
			status_t			_UpdateEvent(uint32 what,
									const BMessenger& source, const char* name,
									uint32 flags = 0);
			status_t			_GetInfo(uint32 what, const char* name,
									BMessage& info);

private:
			BMessenger			fMessenger;
			uint32				_reserved[5];
};


#endif	// _LAUNCH_ROSTER_H
