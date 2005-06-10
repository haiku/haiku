/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef ResScroll_h
#define ResScroll_h

#include <interface/ScrollBar.h>

class ObjectView;

class ResScroll : public BScrollBar {
public:
		ObjectView *	objectView;
		
						ResScroll(BRect r, const char *name,
							ObjectView *target, orientation posture);
virtual	void			ValueChanged(float value);

};

#endif
