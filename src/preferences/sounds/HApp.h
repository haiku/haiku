/*
 * Copyright 2003-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Atsushi Takamatsu
 *		Jérôme Duval
 *		Oliver Ruiz Dorantes
 */
#ifndef HAPP_H
#define HAPP_H


#include <Application.h>
#include <Catalog.h>


class HApp : public BApplication {
public:
								HApp();
	virtual						~HApp();
	virtual	void				AboutRequested();
};


#endif	// HAPP_H
