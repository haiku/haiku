/*
 * Copyright 2011-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_H
#define SETTINGS_H


#include <Message.h>
#include <NetworkAddress.h>
#include <Path.h>


class Settings {
public:
								Settings(const char* accountName,
									const BMessage& archive);
								~Settings();

			BNetworkAddress		ServerAddress() const;

			BString				Server() const;
			uint16				Port() const;
			bool				UseSSL() const;

			BString				Username() const;
			BString				Password() const;

			BPath				Destination() const;

			int32				MaxConnections() const;
			bool				IdleMode() const;
			int32				BodyFetchLimit() const;
			bool				DeleteRemoteWhenLocal() const;

private:
			const BMessage		fMessage;
			BString				fAccountName;
};


#endif	// SETTINGS_H
