/*
 * Copyright (c) 2005-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#ifndef RESWIN_H
#define RESWIN_H

#include <Window.h>

class ResWindow : public BWindow
{
public:
				ResWindow(const BRect &rect,
								const entry_ref *ref=NULL);
				~ResWindow(void);
	bool		QuitRequested(void);
};

#endif
