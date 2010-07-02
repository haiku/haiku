/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 */
#ifndef APR_MAIN_H
#define APR_MAIN_H

#include <Application.h>
#include <Catalog.h>

class APRWindow;

class APRApplication : public BApplication 
{
public:
	APRApplication(void);

private:
	
	APRWindow *fWindow;
};

#endif
