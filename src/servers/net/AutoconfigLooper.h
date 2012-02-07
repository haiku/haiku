/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef AUTOCONFIG_LOOPER_H
#define AUTOCONFIG_LOOPER_H


#include <Looper.h>
#include <Messenger.h>
#include <String.h>
#include <netinet6/in6.h>

class AutoconfigClient;

class AutoconfigLooper : public BLooper {
public:
								AutoconfigLooper(BMessenger target,
									const char* device);
	virtual						~AutoconfigLooper();

	virtual	void				MessageReceived(BMessage* message);

			BMessenger			Target() const { return fTarget; }

private:
			void				_RemoveClient();
			void				_ConfigureIPv4();
			void				_ReadyToRun();

			BMessenger			fTarget;
			BString				fDevice;
			AutoconfigClient*	fCurrentClient;
};

#endif	// AUTOCONFIG_LOOPER_H
