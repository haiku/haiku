/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGEWINDOW_H
#define PACKAGEWINDOW_H

#include "PackageView.h"
#include <Window.h>
#include <Entry.h>


const uint32 P_WINDOW_QUIT		=	'PiWq';


class PackageWindow : public BWindow {
	public:
		PackageWindow(const entry_ref *refs);
		virtual ~PackageWindow();
		
		virtual void Quit();
		
	private:
		PackageView *fBackground;
};


#endif

