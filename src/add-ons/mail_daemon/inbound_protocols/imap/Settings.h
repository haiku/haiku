/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_H
#define SETTINGS_H


#include <Message.h>
#include <NetworkAddress.h>


class Settings {
public:
								Settings(const BMessage& archive);
								~Settings();

			BNetworkAddress		ServerAddress() const;

			BString				Server() const;
			uint16				Port() const;
			bool				UseSSL() const;

			BString				Username() const;
			BString				Password() const;

private:
			const BMessage		fMessage;
};


#endif	// SETTINGS_H
