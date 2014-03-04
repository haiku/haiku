/*
 * Copyright 2007-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUG_NOW_H
#define DEBUG_NOW_H


#include <Point.h>
#include <ScreenSaver.h>


// Inspired by the classic BeOS BuyNow screensaver, of course

class BMessage;
class BView;

class DebugNow : public BScreenSaver {
public:
								DebugNow(BMessage* archive, image_id);

			void				Draw(BView* view, int32 frame);
			void				StartConfig(BView *view);
			status_t			StartSaver(BView *view, bool preview);

private:
			const char* 		fLine1;
			const char*			fLine2;
			BPoint				fLine1Start;
			BPoint				fLine2Start;
			escapement_delta	fDelta;
};


#endif	// DEBUG_NOW_H
