/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SCREEN_SAVER_FILTER_H
#define SCREEN_SAVER_FILTER_H


#include "ScreenSaverSettings.h"

#include <InputServerFilter.h>
#include <Looper.h>
#include <Node.h>


static const uint32 kMsgSuspendScreenSaver = 'susp';

class BMessageRunner;
class ScreenSaverFilter;

class ScreenSaverController : public BLooper {
	public:
        ScreenSaverController(ScreenSaverFilter *filter);
        void MessageReceived(BMessage *msg);

	private:
        ScreenSaverFilter* fFilter;
};

class ScreenSaverFilter : public BInputServerFilter {
	public:
		ScreenSaverFilter();
		virtual ~ScreenSaverFilter();

		virtual filter_result Filter(BMessage *message, BList *outList);

		void Suspend(BMessage *message);

		void CheckTime();
		void CheckCornerInvoke();

		uint32 SnoozeTime() {return fSnoozeTime;}
		void ReloadSettings();
		void SetEnabled(bool enabled) { fEnabled = enabled; }

	private:
		void _WatchSettings();
		void _UpdateRectangles();
		BRect _ScreenCorner(screen_corner pos, uint32 cornerSize);

		void _Invoke();
		void _Banish();

		ScreenSaverSettings fSettings;
		bigtime_t		fLastEventTime, fBlankTime, fSnoozeTime;
		screen_corner	fBlankCorner, fNeverBlankCorner, fCurrentCorner;
		BRect			fBlankRect, fNeverBlankRect;
		bool			fEnabled;
		uint32			fFrameNum;

		ScreenSaverController* fController;
		node_ref		fNodeRef;
		BMessageRunner*	fRunner;
		BMessageRunner*	fCornerRunner;
		bool			fWatchingDirectory, fWatchingFile;
};

#endif	/* SCREEN_SAVER_FILTER_H */
