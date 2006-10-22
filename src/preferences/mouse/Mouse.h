/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 *		Jérôme Duval
 *		Axel Dörfler (axeld@pinc-software.de)
 */
#ifndef MOUSE_H
#define MOUSE_H


#include <Application.h>


class MouseApplication : public BApplication {
	public:
		MouseApplication();

		virtual void AboutRequested();
};

#endif	/* MOUSE_H */
