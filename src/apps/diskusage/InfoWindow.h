/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */
#ifndef INFO_WINDOW_H
#define INFO_WINDOW_H


#include <Window.h>


struct FileInfo;

class LeftView: public BView {
public:
						LeftView(BRect frame, BBitmap* icon);
	virtual				~LeftView();

	virtual	void		Draw(BRect updateRect);

private:
			BBitmap*	fIcon;
};


class InfoWin: public BWindow {
public:
						InfoWin(BPoint location, FileInfo* info,
							BWindow* parent);
	virtual				~InfoWin();
};

#endif // INFO_WINDOW_H
