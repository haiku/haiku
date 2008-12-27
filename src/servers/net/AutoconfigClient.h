/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef AUTOCONFIG_CLIENT_H
#define AUTOCONFIG_CLIENT_H


#include <Handler.h>
#include <Messenger.h>
#include <String.h>


class AutoconfigClient : public BHandler {
public:
								AutoconfigClient(const char* name,
									BMessenger target, const char* device);
	virtual						~AutoconfigClient();

	virtual	status_t			Initialize();

			const BMessenger&	Target() const { return fTarget; }
			const char*			Device() const { return fDevice.String(); }

private:
			BMessenger			fTarget;
			BString				fDevice;
};

#endif	// AUTOCONFIG_CLIENT_H
