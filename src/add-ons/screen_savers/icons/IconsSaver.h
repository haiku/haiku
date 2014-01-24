/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Vincent Duvert, vincent.duvert@free.fr
 *		John Scipione, jscipione@gmail.com
 */
#ifndef ICONS_SAVER_H
#define ICONS_SAVER_H


#include <ObjectList.h>
#include <ScreenSaver.h>


struct vector_icon;


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
			void				_GetVectorIcons();

	BObjectList<vector_icon>	fVectorIcons;
			IconDisplay*		fIcons;

			BBitmap*			fBackBitmap;
			BView*				fBackView;

			uint16				fMinSize;
			uint16				fMaxSize;
};


#endif	// ICONS_SAVER_H
