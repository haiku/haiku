/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */
#ifndef SCREEN_SAVER_FILTER_H
#define SCREEN_SAVER_FILTER_H


#include <InputServerFilter.h>
#include <Rect.h>
#include <Region.h>
#include <Messenger.h>
#include <MessageFilter.h>
#include <MessageRunner.h>
#include <Node.h>
#include "ScreenSaverPrefs.h"


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

		void CheckTime();
		uint32 SnoozeTime() {return fSnoozeTime;}
		void ReloadSettings();
		void SetEnabled(bool enabled) { fEnabled = enabled; }

	private:
		bigtime_t fLastEventTime, fBlankTime, fSnoozeTime;
		arrowDirection fBlank, fKeep, fCurrent; 
		bool fEnabled;
		BRect fTopLeft, fTopRight, fBottomLeft, fBottomRight;
		ScreenSaverPrefs fPrefs;
		uint32 fFrameNum;
			// Used so that we don't update the screen coord's so often
			// Ideally, we would get a message when the screen changes.
			// R5 doesn't do this.

		ScreenSaverController *fController;
		node_ref fPrefsNodeRef, fPrefsDirNodeRef;
		BMessageRunner *fRunner;
		bool fWatchingDirectory, fWatchingFile;
	
		void WatchPreferences();
		void UpdateRectangles();
		void Cornered(arrowDirection pos);
		void Invoke();
		void Banish();
};

#endif	/* SCREEN_SAVER_FILTER_H */
