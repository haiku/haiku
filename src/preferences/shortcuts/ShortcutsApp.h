/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		Fredrik Modéen
 */
#ifndef SHORTCUTS_APP_H
#define SHORTCUTS_APP_H


#include <Application.h>


class ShortcutsApp : public BApplication {
public:
							ShortcutsApp();
							~ShortcutsApp();
	virtual	void			ReadyToRun();
};


#endif	// SHORTCUTS_APP_H
