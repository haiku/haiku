/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGE_WINDOW_H
#define PACKAGE_WINDOW_H


#include <Window.h>


struct entry_ref;

const uint32 P_WINDOW_QUIT		=	'PiWq';


class PackageWindow : public BWindow {
public:
								PackageWindow(const entry_ref* ref);
	virtual						~PackageWindow();
		
	virtual	void				Quit();
};


#endif // PACKAGE_WINDOW_H

