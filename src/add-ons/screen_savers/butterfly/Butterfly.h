/*
 * Copyright 2010-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Geoffry Song, goffrie@gmail.com
 */
#ifndef _BUTTERFLY_H_
#define _BUTTERFLY_H_


#include <ScreenSaver.h>


class Butterfly : public BScreenSaver {
public:
						Butterfly(BMessage* archive, image_id imageId);

	virtual	void		StartConfig(BView* view);
	virtual status_t	StartSaver(BView* view, bool preview);
	virtual void		Draw(BView* view, int32 frame);

private:
			float		fT;
			// previously calculated points
			BPoint		fLast[3];
			// transformation from graph coordinates to view coordinates
			float		fScale;
			BPoint		fTrans;
			// bounding box of drawn figure
			BRect		fBounds;

	inline	rgb_color	_HueToColor(float hue);
	inline	BPoint		_Iterate();
};

#endif  // _BUTTERFLY_H_

