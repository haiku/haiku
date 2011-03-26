/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		Fredrik	Modéen
 */
#ifndef ShortcutsApp_h
#define ShortcutsApp_h


#include <Application.h>


class ShortcutsApp : public BApplication {
public:
							ShortcutsApp();
							~ShortcutsApp();
	virtual	void			ReadyToRun();
};


#endif

