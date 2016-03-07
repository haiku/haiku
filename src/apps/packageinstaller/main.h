/*
 * Copyright (c) 2007-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGEINSTALLER_MAIN_H
#define PACKAGEINSTALLER_MAIN_H

#include <Application.h>


const uint32 P_WINDOW_QUIT		=	'PiWq';



class PackageInstaller : public BApplication {
public:
								PackageInstaller();
	virtual						~PackageInstaller();

	virtual void				RefsReceived(BMessage* message);
	virtual void				ArgvReceived(int32 argc, char** argv);
	virtual void				ReadyToRun();

	virtual void				MessageReceived(BMessage* message);

private:
			void				_NewWindow(const entry_ref* ref);

private:
			uint32				fWindowCount;
};

#endif // PACKAGEINSTALLER_MAIN_H
