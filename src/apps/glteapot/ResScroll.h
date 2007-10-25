/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef RESSCROLL_H
#define RESSCROLL_H

#include <ScrollBar.h>

class ObjectView;

class ResScroll : public BScrollBar {
public:
								ResScroll(BRect r, const char* name,
									ObjectView* target, orientation posture);

	virtual	void				ValueChanged(float value);

private:
			ObjectView*			fObjectView;
};

#endif // RESSCROLL_H
