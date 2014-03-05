/*
 * Copyright 2007-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 */
#ifndef SPIDER_SAVER_H
#define SPIDER_SAVER_H


#include <Locker.h>
#include <ScreenSaver.h>
#include <View.h>


enum {
	RED							= 1,
	GREEN						= 2,
	BLUE						= 3,
	YELLOW						= 4,
	PURPLE						= 5,
	CYAN						= 6,
	GRAY						= 7,
};


#define MIN_POLY_POINTS 3
#define MAX_POLY_POINTS 10
#define MIN_QUEUE_DEPTH 40
#define MAX_QUEUE_DEPTH 160
#define MAX_QUEUE_NUMBER 40



class BSlider;
class BMenuField;
class Polygon;
class PolygonQueue;
class SpiderView;


class SpiderSaver : public BScreenSaver {
public:
								SpiderSaver(BMessage *message,
											image_id image);
	virtual						~SpiderSaver();

								// BScreenSaver
	virtual	void				StartConfig(BView *view);
	virtual	status_t			StartSaver(BView *view, bool preview);
	virtual	void				StopSaver();
	virtual	void				Draw(BView* view, int32 frame);
	virtual	status_t			SaveState(BMessage* into) const;

								// SpiderSaver
			void				SetQueueNumber(uint32 number);
			void				SetQueueDepth(uint32 maxDepth);
			void				SetPolyPoints(uint32 maxPoints);
			void				SetColor(uint32 color);

private:
			void				_Init(BRect bounds);
			void				_Cleanup();
			void				_AllocBackBitmap(float width, float height);
			void				_FreeBackBitmap();
			void				_DrawInto(BView *view);
			void				_DrawPolygon(Polygon* polygon, BView *view);

			BBitmap*			fBackBitmap;
			BView*				fBackView;

			PolygonQueue**		fQueues;
			uint32				fQueueNumber;
			uint32				fMaxPolyPoints;
			uint32				fMaxQueueDepth;
			uint32				fColor;

			bool				fPreview;
			BRect				fBounds;

			BLocker				fLocker;
};


#endif	//	SPIDER_SAVER_H
