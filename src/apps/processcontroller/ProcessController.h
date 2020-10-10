/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Copyright 2006-2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PCVIEW_H_
#define _PCVIEW_H_


#include "AutoIcon.h"

#include <View.h>


class BMessageRunner;
class ThreadBarMenu;


class ProcessController : public BView {
	public:
						ProcessController(BRect frame, bool temp = false);
						ProcessController(BMessage *data);
						ProcessController(float width, float height);
		virtual			~ProcessController();

		virtual	void	MessageReceived(BMessage *message);
		virtual	void	AttachedToWindow();
		virtual	void	MouseDown(BPoint where);
		virtual	void	Draw(BRect updateRect);
				void	DoDraw (bool force);
		static	ProcessController* Instantiate(BMessage* data);
		virtual	status_t Archive(BMessage *data, bool deep = true) const;

		void			AboutRequested();
		void			Update();
		void			DefaultColors();

		// TODO: move those into private, and have getter methods
		AutoIcon		fProcessControllerIcon;
		AutoIcon		fProcessorIcon;
		AutoIcon		fTrackerIcon;
		AutoIcon		fDeskbarIcon;
		AutoIcon		fTerminalIcon;

	private:
		void			Init();
		void			_HandleDebugRequest(team_id team, thread_id thread);

		const int32		kCPUCount;
		bool			fTemp;
		float			fMemoryUsage;
		float*			fLastBarHeight;
		float			fLastMemoryHeight;
		double*			fCPUTimes;
		bigtime_t*		fPrevActive;
		bigtime_t		fPrevTime;
		BMessageRunner*	fMessageRunner;
		rgb_color		frame_color, active_color, idle_color, memory_color, swap_color;
};

extern	ProcessController*	gPCView;
extern	uint32				gCPUcount;
extern	rgb_color			gIdleColor;
extern	rgb_color			gIdleColorSelected;
extern	rgb_color			gKernelColor;
extern	rgb_color			gKernelColorSelected;
extern	rgb_color			gUserColor;
extern	rgb_color			gUserColorSelected;
extern	rgb_color			gFrameColor;
extern	rgb_color			gFrameColorSelected;
extern	rgb_color			gMenuBackColor;
extern	rgb_color			gMenuBackColorSelected;
extern	rgb_color			gWhiteSelected;
extern	ThreadBarMenu*		gCurrentThreadBarMenu;
extern	thread_id			gPopupThreadID;
extern	const char*			kDeskbarItemName;
extern	bool				gInDeskbar;

#define kBarWidth 100
#define kMargin	12

#endif // _PCVIEW_H_
