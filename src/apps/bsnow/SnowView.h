#ifndef _SNOW_VIEW_H
#define _SNOW_VIEW_H


#include <Bitmap.h>
#include <Dragger.h>
#include <List.h>
#include <MessageRunner.h>
#include <OS.h>
#include <View.h>

#include "Flakes.h"


#define APP_SIG "application/x-vnd.mmu_man.BSnow"
#define SNOW_VIEW_RECT 0,0,200,40
#define NUM_FLAKES 200
#define INTERVAL 200000
#define MSG_PULSE_ME 'PulS'
#define MSG_DRAW_ME 'DraW'
#define WORKSPACES_COUNT 32
#define WEIGHT_SPAN 50
#define WEIGHT_GRAN 10
#define WIND_SPAN 25
#define WIND_MAX_DURATION 10000000
#define FALLEN_HEIGHT 30
#define INVALIDATOR_THREAD_NAME "You're Neo? I'm the Snow Maker!"


typedef struct flake {
	BPoint pos;
	BPoint opos;
	float weight;
} flake;


class SnowView : public BView
{
public:
						SnowView();
						SnowView(BMessage *archive);
						~SnowView();
	static  BArchivable	*Instantiate(BMessage *data);
	virtual	status_t	Archive(BMessage *data, bool deep = true) const;
			void		AttachedToWindow();
			void		DetachedFromWindow();
			void		MessageReceived(BMessage *msg);
			void		Draw(BRect ur);
			void		Pulse();

	virtual	void		MouseDown(BPoint where);
	virtual	void		MouseUp(BPoint where);
	virtual	void		MouseMoved(BPoint where, uint32 code,
							const BMessage *a_message);
	virtual	void		KeyDown(const char *bytes, int32 numBytes);
	virtual	void		KeyUp(const char *bytes, int32 numBytes);

	static	int32		SnowMakerThread(void *p_this);
			void		Calc();
			void		InvalFlakes();

private:
			BMessageRunner *fMsgRunner;
			BDragger	*fDragger;
			long		fNumFlakes;
			flake		*fFlakes[WORKSPACES_COUNT];
			BList		*fFallenFlakes[WORKSPACES_COUNT];
			uint32		fCurrentWorkspace;
			uint32		fCachedWsWidth;
			uint32		fCachedWsHeight;
			float		fWind;
			bigtime_t	fWindDuration;
			bool		fAttached;
			BBitmap		*fFlakeBitmaps[NUM_PATTERNS];
			thread_id	fInvalidator;
			BView		*fCachedParent;
			BBitmap		*fFallenBmp;
			BView		*fFallenView;
			BRegion		*fFallenReg;
			bool		fShowClickMe;
};

#endif
