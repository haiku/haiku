// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MediaAlert.h
//  Author:      Jérôme Duval
//  Description: Media Preferences
//  Created :    June 25, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
#ifndef	_MEDIAALERT_H
#define	_MEDIAALERT_H

#include <Window.h>
#include <Bitmap.h>
#include <TextView.h>


class MediaAlert : public BWindow
{
public:
								MediaAlert(BRect rect, const char* title,
									const char* text);
	virtual						~MediaAlert();
			BTextView*			TextView() const;
private:
			BBitmap*			InitIcon();
			BTextView*			fTextView;
};

#endif	// _MEDIAALERT_H

