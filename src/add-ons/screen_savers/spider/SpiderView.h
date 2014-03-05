/*
 * Copyright 2007-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 */
#ifndef SPIDER_VIEW_H
#define SPIDER_VIEW_H


#include <View.h>


class BRect;
class BMenuField;
class BMessage;
class BSlider;
class SpiderSaver;


class SpiderView : public BView {
public:
								SpiderView(BRect frame, SpiderSaver* saver,
									uint32 queueNumber,
									uint32 maxPolyPoints,
									uint32 maxQueueDepth,
									uint32 color);
	virtual						~SpiderView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			SpiderSaver*		fSaver;

			BSlider*			fQueueNumberSlider;
			BSlider*			fPolyNumberSlider;
			BSlider*			fQueueDepthSlider;
			BMenuField*			fColorMenuField;
};


#endif	// SPIDER_VIEW_H
