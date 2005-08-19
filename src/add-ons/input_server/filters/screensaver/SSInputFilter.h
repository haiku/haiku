/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <InputServerFilter.h>
#include <Rect.h>
#include <Region.h>
#include <Messenger.h>
#include <MessageFilter.h>
#include <MessageRunner.h>
#include <Node.h>
#include "ScreenSaverPrefs.h"

const int CORNER_SIZE = 10;
const int32 SS_CHECK_TIME = 'SSCT';

class SSInputFilter;

class SSController : public BLooper
{
public:
        SSController(SSInputFilter *filter);
        void MessageReceived(BMessage *msg);
private:
        SSInputFilter* fFilter;
};

class SSInputFilter : public BInputServerFilter
{
public:
	SSInputFilter();
	virtual ~SSInputFilter();
	virtual filter_result Filter(BMessage *msg, BList *outList);
	void CheckTime();
	uint32 GetSnoozeTime(void) {return fSnoozeTime;}
	void ReloadSettings();
	void SetEnabled(bool enabled) { fEnabled = enabled; }
private:
	bigtime_t fLastEventTime, fBlankTime, fSnoozeTime, fRtc;
	arrowDirection fBlank, fKeep, fCurrent; 
	bool fEnabled;
	BRect fTopLeft, fTopRight, fBottomLeft, fBottomRight;
	ScreenSaverPrefs fPref;
	uint32 fFrameNum; // Used so that we don't update the screen coord's so often
				// Ideally, we would get a message when the screen changes. R5 doesn't do this.
	SSController *fSSController;
	node_ref fPrefNodeRef;
	BMessageRunner *fRunner;

	void UpdateRectangles();
	void Cornered(arrowDirection pos);
	void Invoke();
	void Banish();
};

