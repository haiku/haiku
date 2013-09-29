/*
 * Copyright 2006-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SETTINGS_H
#define SETTINGS_H


#include <Message.h>
#include <Messenger.h>
#include <Path.h>


class Settings {
public:
								Settings();
								~Settings();

			status_t			GetNextInterface(uint32& cookie,
									BMessage& interface);

			int32				CountNetworks() const;
			status_t			GetNextNetwork(uint32& cookie,
									BMessage& network) const;
			status_t			AddNetwork(const BMessage& network);
			status_t			RemoveNetwork(const char* name);

			status_t			GetNextService(uint32& cookie,
									BMessage& service);
			const BMessage&		Services() const;

			status_t			StartMonitoring(const BMessenger& target);
			status_t			StopMonitoring(const BMessenger& target);

			status_t			Update(BMessage* message);

private:
			status_t			_Load(const char* name = NULL,
									uint32* _type = NULL);
			status_t			_Save(const char* name = NULL);
			BPath				_Path(BPath& parent, const char* name);
			status_t			_GetPath(const char* name, BPath& path);

			status_t			_StartWatching(const char* name,
									const BMessenger& target);

			bool				_IsWatching(const BMessenger& target) const
									{ return fListener == target; }
			bool				_IsWatching() const
									{ return fListener.IsValid(); }

private:
			BMessenger			fListener;
			BMessage			fInterfaces;
			BMessage			fNetworks;
			BMessage			fServices;
};


static const uint32 kMsgInterfaceSettingsUpdated = 'SUif';
static const uint32 kMsgServiceSettingsUpdated = 'SUsv';


#endif	// SETTINGS_H
