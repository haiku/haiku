/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/
#ifndef ICONS_SAVER_H
#define ICONS_SAVER_H

#include <List.h>
#include <ScreenSaver.h>


class IconDisplay;


class IconsSaver: public BScreenSaver {
public:
								IconsSaver(BMessage* archive, image_id);
	virtual						~IconsSaver();

	virtual	status_t			StartSaver(BView *view, bool preview);
	virtual	void				StopSaver();

	virtual	void				Draw(BView *view, int32 frame);

	virtual	void				StartConfig(BView* view);

private:
	BList						fVectorIcons;
	int32						fVectorIconsCount;
	IconDisplay*				fIcons;

	BBitmap*					fBackBitmap;
	BView*						fBackView;

	uint16						fMinSize, fMaxSize;
};

#endif
