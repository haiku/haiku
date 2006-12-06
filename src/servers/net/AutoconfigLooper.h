/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
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


class AutoconfigLooper : public BLooper {
	public:
		AutoconfigLooper(BMessenger target, const char* device);
		virtual ~AutoconfigLooper();

		virtual void MessageReceived(BMessage* message);

		BMessenger Target() const { return fTarget; }

	private:
		void _ReadyToRun();

		BMessenger	fTarget;
		BString		fDevice;
};

#endif	// AUTOCONFIG_LOOPER_H
