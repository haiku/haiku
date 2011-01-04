/*
 * Copyright 2010-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Geoffry Song, goffrie@gmail.com
 */
#ifndef _LEAVES_H_
#define _LEAVES_H_


#include <vector>

#include <ScreenSaver.h>


class BSlider;

class Leaves : public BScreenSaver, public BHandler {
public:
						Leaves(BMessage* archive, image_id id);
	virtual	void		Draw(BView* view, int32 frame);
	virtual	void		StartConfig(BView* view);
	virtual	status_t	StartSaver(BView* view, bool preview);

	virtual	status_t	SaveState(BMessage* into) const;

	virtual	void		MessageReceived(BMessage* message);

private:
			BSlider*	fDropRateSlider;
			BSlider*	fLeafSizeSlider;
			BSlider*	fSizeVariationSlider;
			int32		fDropRate;
			int32		fLeafSize;
			int32		fSizeVariation;

	inline	BPoint		_RandomPoint(const BRect& bound);
};

#endif	// _LEAVES_H_

