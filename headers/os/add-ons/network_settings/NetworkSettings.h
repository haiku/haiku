/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
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


namespace BNetworkKit {


class BNetworkSettings {
public:
	static	const uint32		kMsgInterfaceSettingsUpdated = 'SUif';
	static	const uint32		kMsgServiceSettingsUpdated = 'SUsv';

public:
								BNetworkSettings();
								~BNetworkSettings();

			status_t			GetNextInterface(uint32& cookie,
									BMessage& interface);
			status_t			AddInterface(const BMessage& interface);
			status_t			RemoveInterface(const char* name);

			int32				CountNetworks() const;
			status_t			GetNextNetwork(uint32& cookie,
									BMessage& network) const;
			status_t			AddNetwork(const BMessage& network);
			status_t			RemoveNetwork(const char* name);

			status_t			GetNextService(uint32& cookie,
									BMessage& service);
			const BMessage&		Services() const;
			status_t			AddService(const BMessage& service);
			status_t			RemoveService(const char* name);

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

			status_t			_ConvertNetworkToSettings(BMessage& message);
			status_t			_ConvertNetworkFromSettings(BMessage& message);
			status_t			_RemoveItem(BMessage& container,
									const char* itemField,const char* nameField,
									const char* name, const char* store);

private:
			BMessenger			fListener;
			BMessage			fInterfaces;
			BMessage			fNetworks;
			BMessage			fServices;
};


}	// namespace BNetworkKit


#endif	// SETTINGS_H
