/*
 * Copyright 2003-2009, Haiku.
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
#include <Locker.h>
#include <Looper.h>
#include <Node.h>


class BMessageRunner;
class ScreenSaverFilter;


class ScreenSaverController : public BLooper {
public:
								ScreenSaverController(
									ScreenSaverFilter* filter);
	virtual	void				MessageReceived(BMessage* msg);

private:
			ScreenSaverFilter*	fFilter;
};


class ScreenSaverFilter : public BInputServerFilter, BLocker {
public:
								ScreenSaverFilter();
	virtual						~ScreenSaverFilter();

	virtual	filter_result		Filter(BMessage* message, BList* outList);

			void				Suspend(BMessage* message);

			void				CheckTime();
			void				CheckCornerInvoke();

			void				ReloadSettings();
			void				SetIsRunning(bool isRunning);

private:
			uint32				_SnoozeTime() {return fSnoozeTime;}
			void				_WatchSettings();
			void				_UpdateRectangles();
			BRect				_ScreenCorner(screen_corner pos,
									uint32 cornerSize);

			void				_Invoke();

			ScreenSaverSettings	fSettings;
			bigtime_t			fLastEventTime;
			bigtime_t			fBlankTime;
			bigtime_t			fSnoozeTime;
			screen_corner		fBlankCorner;
			screen_corner		fNeverBlankCorner;
			screen_corner		fCurrentCorner;
			BRect				fBlankRect;
			BRect				fNeverBlankRect;
			uint32				fFrameNum;

			ScreenSaverController* fController;
			node_ref			fNodeRef;
			BMessageRunner*		fRunner;
			BMessageRunner*		fCornerRunner;
			bool				fWatchingDirectory;
			bool				fWatchingFile;

			bool				fIsRunning;
};

#endif	/* SCREEN_SAVER_FILTER_H */
