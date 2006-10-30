/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 */
#ifndef APR_WINDOW_H
#define APR_WINDOW_H

#include <Application.h>
#include <Window.h>
#include <Message.h>

class APRWindow : public BWindow 
{
public:
			APRWindow(BRect frame); 
	bool	QuitRequested(void);
};

#endif
