/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef RESSCROLL_H
#define RESSCROLL_H

#include <interface/ScrollBar.h>

class ObjectView;

class ResScroll : public BScrollBar {
	public:
			ObjectView *	objectView;
			
							ResScroll(BRect r, const char *name,
								ObjectView *target, orientation posture);
			virtual	void	ValueChanged(float value);
};

#endif
