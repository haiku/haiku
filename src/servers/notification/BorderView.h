/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BORDER_VIEW_H
#define _BORDER_VIEW_H

#include <View.h>
#include <String.h>

class BorderView : public BView {
public:
					BorderView(BRect, const char*);
	virtual			~BorderView();

	virtual	void	Draw(BRect updateRect);

	virtual	void	GetPreferredSize(float*, float*);

	virtual	void	FrameResized(float, float);

			float	BorderSize();

private:
			BString	fTitle;
};

#endif	// _BORDER_VIEW_H
