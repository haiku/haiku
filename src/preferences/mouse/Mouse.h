/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval,
 *		Axel Dörfler (axeld@pinc-software.de)
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 */
#ifndef MOUSE_H
#define MOUSE_H


#include <Application.h>
#include <Catalog.h>
#include <Locale.h>


class MouseApplication : public BApplication {
public:
		MouseApplication();

		virtual void AboutRequested();
};

#endif	/* MOUSE_H */
