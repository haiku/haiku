/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 */
#ifndef APPEARANCE_H
#define APPEARANCE_H

#include <Application.h>
#include <Catalog.h>

class AppearanceWindow;

class AppearanceApplication : public BApplication 
{
public:
	AppearanceApplication(void);

private:
	
	AppearanceWindow *fWindow;
};

#endif /* APPEARANCE_H */
