/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef NETWORK_STATUS_WINDOW_H
#define NETWORK_STATUS_WINDOW_H


#include <Window.h>


class NetworkStatusWindow : public BWindow {
	public:
		NetworkStatusWindow();
		virtual ~NetworkStatusWindow();

		virtual bool QuitRequested();
};

#endif	// NETWORK_STATUS_WINDOW_H
