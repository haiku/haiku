/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#ifndef RESWIN_H
#define RESWIN_H

#include <Window.h>

struct entry_ref;
class ResView;

class ResWindow : public BWindow {
public:
								ResWindow(const BRect& rect,
									const entry_ref* ref = NULL);
	virtual						~ResWindow();

	virtual	bool				QuitRequested();
private:
	ResView		*fView;
};

#endif // RESWIN_H
