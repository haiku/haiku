/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#ifndef	_TRACKER_BACKGROUND_H
#define	_TRACKER_BACKGROUND_H

#include <SupportDefs.h>

/*----------------------------------------------------------------*/
/*-----  Tracker background attribute name  ----------------------*/

#define B_BACKGROUND_INFO		"be:bgndimginfo"

/*----------------------------------------------------------------*/
/*-----  Tracker background BMessage entries  --------------------*/

#define B_BACKGROUND_IMAGE			"be:bgndimginfopath"		// string path
#define B_BACKGROUND_MODE			"be:bgndimginfomode"		// int32, the enum below
#define B_BACKGROUND_ORIGIN			"be:bgndimginfooffset"		// BPoint
#define B_BACKGROUND_TEXT_OUTLINE	"be:bgndimginfoerasetext"	// bool
	// NOTE: the actual attribute name is kept for backwards compatible settings
#define B_BACKGROUND_WORKSPACES		"be:bgndimginfoworkspaces"	// uint32

/*----------------------------------------------------------------*/
/*-----  Background mode values  ---------------------------------*/

enum {
	B_BACKGROUND_MODE_USE_ORIGIN,
	B_BACKGROUND_MODE_CENTERED,		// only works on Desktop
	B_BACKGROUND_MODE_SCALED,		// only works on Desktop
	B_BACKGROUND_MODE_TILED
};

/*----------------------------------------------------------------*/
/*----------------------------------------------------------------*/

const int32 B_RESTORE_BACKGROUND_IMAGE = 'Tbgr';	// force a Tracker window to
													// use a new background image

#endif // _TRACKER_BACKGROUND_H
