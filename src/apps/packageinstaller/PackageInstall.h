/*
 * Copyright (c) 2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGE_INSTALL_H
#define PACKAGE_INSTALL_H

#include <Locker.h>

class PackageView;
class PackageScript;

enum {
	P_MSG_I_FINISHED = 'pifi',
	P_MSG_I_ABORT = 'piab',
	P_MSG_I_ERROR = 'pier'
};

class PackageInstall {
	public:
		PackageInstall(PackageView *parent);
		~PackageInstall();

		status_t Start();
		void Stop();
		void Install();

	private:
		uint32 _Install();

		PackageView *fParent;
		thread_id fThreadId;
		BLocker fIdLocker;

		PackageScript *fCurrentScript;
		BLocker fCurrentScriptLocker; // XXX: will we need this?
		int32 fItemExistsPolicy;
};

#endif
