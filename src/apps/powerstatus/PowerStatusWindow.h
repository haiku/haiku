/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef POWER_STATUS_WINDOW_H
#define POWER_STATUS_WINDOW_H


#include <Window.h>


class PowerStatusWindow : public BWindow {
	public:
		PowerStatusWindow();
		virtual ~PowerStatusWindow();

		virtual bool QuitRequested();
};

#endif	// POWER_STATUS_WINDOW_H
