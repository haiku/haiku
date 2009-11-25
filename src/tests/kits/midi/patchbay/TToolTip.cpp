//--------------------------------------------------------------------
//	
//	TToolTip.cpp
//
//	Written by: Robert Polic
//	
//--------------------------------------------------------------------

#include <Screen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Roster.h>

#include "TToolTip.h"

#define kHOR_MARGIN					  4		// hor. gap between frame and tip
#define kVER_MARGIN					  3		// ver. gap between frame and tip
#define kTIP_HOR_OFFSET				 10		// tip position right of cursor
#define kTIP_VER_OFFSET				 16		// tip position below cursor
#define kSLOP						  4		// mouse slop before tip hides

#define kTOOL_TIP_DELAY_TIME	 500000		// default delay time before tip shows (.5 secs.)
#define kTOOL_TIP_HOLD_TIME		3000000		// default hold time of time (3 secs.)

#define kDRAW_WINDOW_FRAME

const rgb_color kVIEW_COLOR	= {255, 203, 0, 255};	// view background color (light yellow)
const rgb_color kLIGHT_VIEW_COLOR = {255, 255, 80, 255}; // top left frame highlight
const rgb_color kDARK_VIEW_COLOR = {175, 123, 0, 255}; // bottom right frame highlight
const rgb_color kTEXT_COLOR = {0, 0, 0, 255};		// text color (black)


//====================================================================

TToolTip::TToolTip(tool_tip_settings *settings)
	   :BWindow(BRect(0, 0, 10, 10), "tool_tip", B_NO_BORDER_WINDOW_LOOK,
	   			B_FLOATING_ALL_WINDOW_FEEL, B_AVOID_FRONT)
{
	// setup the tooltip view
	AddChild(fView = new TToolTipView(settings));
	// start the message loop thread
	Run();
}

//--------------------------------------------------------------------

void TToolTip::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		// forward interesting messages to the view
		case B_SOME_APP_ACTIVATED:
		case eToolTipStart:
		case eToolTipStop:
			PostMessage(msg, fView);
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

void TToolTip::GetSettings(tool_tip_settings *settings)
{
	fView->GetSettings(settings);
}

//--------------------------------------------------------------------

void TToolTip::SetSettings(tool_tip_settings *settings)
{
	fView->SetSettings(settings);
}


//====================================================================

TToolTipView::TToolTipView(tool_tip_settings *settings)
			 :BView(BRect(0, 0, 10, 10), "tool_tip", B_FOLLOW_ALL, B_WILL_DRAW)
{
	// initialize tooltip settings
	if (settings)
		// we should probably sanity-check user defined settings (but we won't)
		fTip.settings = *settings;
	else {
		// use defaults if no settings are passed
		fTip.settings.enabled = true;
		fTip.settings.one_time_only = false;
		fTip.settings.delay = kTOOL_TIP_DELAY_TIME;
		fTip.settings.hold = kTOOL_TIP_HOLD_TIME;
		fTip.settings.font = be_plain_font;
	}

	// initialize the tip
	fString = (char *)malloc(1);
	fString[0] = 0;

	// initialize the view
	SetFont(&fTip.settings.font);
	SetViewColor(kVIEW_COLOR);
}

//--------------------------------------------------------------------

TToolTipView::~TToolTipView()
{
	status_t	status;

	// kill tool_tip thread
	fTip.quit = true;
	wait_for_thread(fThread, &status);

	// free tip
	free(fString);
}

//--------------------------------------------------------------------

void TToolTipView::AllAttached()
{
	// initialize internal settings
	fTip.app_active = true;
	fTip.quit = false;
	fTip.stopped = true;

	fTip.tool_tip_view = this;
	fTip.tool_tip_window = Window();

	// start tool_tip thread
	resume_thread(fThread = spawn_thread((status_t (*)(void *)) ToolTipThread,
				"tip_thread", B_DISPLAY_PRIORITY, &fTip));
}

//--------------------------------------------------------------------

void TToolTipView::Draw(BRect /* where */)
{
	char		*src_strings[1];
	char		*tmp_string;
	char		*truncated_strings[1];
	BFont		font;
	BRect		r = Bounds();
	font_height	finfo;

	// draw border around window
#ifdef kDRAW_WINDOW_FRAME
	SetHighColor(0, 0, 0, 255);
	StrokeRect(r);
	r.InsetBy(1, 1);
#endif
	SetHighColor(kLIGHT_VIEW_COLOR);
	StrokeLine(BPoint(r.left, r.bottom), BPoint(r.left, r.top));
	StrokeLine(BPoint(r.left + 1, r.top), BPoint(r.right - 1, r.top));
	SetHighColor(kDARK_VIEW_COLOR);
	StrokeLine(BPoint(r.right, r.top), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.right - 1, r.bottom), BPoint(r.left + 1, r.bottom));

	// set pen position
	GetFont(&font);
	font.GetHeight(&finfo);
	MovePenTo(kHOR_MARGIN + 1, kVER_MARGIN + finfo.ascent);

	// truncate string if needed
	src_strings[0] = fString;
	tmp_string = (char *)malloc(strlen(fString) + 16);
	truncated_strings[0] = tmp_string;
	font.GetTruncatedStrings((const char **)src_strings, 1, B_TRUNCATE_END,
		Bounds().Width() - (2 * kHOR_MARGIN) + 1, truncated_strings);

	// draw string
	SetLowColor(kVIEW_COLOR);
	SetHighColor(kTEXT_COLOR);
	DrawString(tmp_string);
	free(tmp_string);
}

//--------------------------------------------------------------------

void TToolTipView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_SOME_APP_ACTIVATED:
			msg->FindBool("active", &fTip.app_active);
			break;

		case eToolTipStart:
			{
				const char	*str;

				// extract parameters
				msg->FindPoint("start", &fTip.start);
				msg->FindRect("bounds", &fTip.bounds);
				msg->FindString("string", &str);
				free(fString);
				fString = (char *)malloc(strlen(str) + 1);
				strcpy(fString, str);

				// force window to fit new parameters
				AdjustWindow();

				// flag thread to reset
				fTip.reset = true;
			}
			break;

		case eToolTipStop:
			// flag thread to stop
			fTip.stop = true;
			break;
	}
}

//--------------------------------------------------------------------

void TToolTipView::GetSettings(tool_tip_settings *settings)
{
	// return current settings
	*settings = fTip.settings;
}

//--------------------------------------------------------------------

void TToolTipView::SetSettings(tool_tip_settings *settings)
{
	bool	invalidate = fTip.settings.font != settings->font;

	// we should probably sanity-check user defined settings (but we won't)
	fTip.settings = *settings;

	// if the font changed, adjust window to fit
	if (invalidate) {
		Window()->Lock();
		SetFont(&fTip.settings.font);
		AdjustWindow();
		Window()->Unlock();
	}
}

//--------------------------------------------------------------------

void TToolTipView::AdjustWindow()
{
	float		width;
	float		height;
	float		x;
	float		y;
	BScreen		s(B_MAIN_SCREEN_ID);
	BRect		screen = s.Frame();
	BWindow		*wind = Window();
	font_height	finfo;

	screen.InsetBy(2, 2);	// we want a 2-pixel clearance
	fTip.settings.font.GetHeight(&finfo);
	width = fTip.settings.font.StringWidth(fString) + (kHOR_MARGIN * 2);  // string width
	height = (finfo.ascent + finfo.descent + finfo.leading) + (kVER_MARGIN * 2);  // string height

	// calculate new position and size of window
	x = fTip.start.x + kTIP_HOR_OFFSET;
	if ((x + width) > screen.right)
		x = screen.right - width;
	y = fTip.start.y + kTIP_VER_OFFSET;
	if ((y + height) > screen.bottom) {
		y = screen.bottom - height;
		if ((fTip.start.y >= (y - kSLOP)) && (fTip.start.y <= (y + height)))
			y = fTip.start.y - kTIP_VER_OFFSET - height;
	}
	if (x < screen.left) {
		width -= screen.left - x;
		x = screen.left;
	}
	if (y < screen.top) {
		height -= screen.top - y;
		y = screen.top;
	}

	wind->MoveTo((int)x, (int)y);
	wind->ResizeTo((int)width, (int)height);

	// force an update
	Invalidate(Bounds());
}

//--------------------------------------------------------------------

status_t TToolTipView::ToolTipThread(tool_tip *tip)
{
	uint32	buttons;
	BPoint	where;
	BScreen	s(B_MAIN_SCREEN_ID);
	BRect	screen = s.Frame();

	screen.InsetBy(2, 2);
	while (!tip->quit) {
		if (tip->tool_tip_window->LockWithTimeout(0) == B_NO_ERROR) {
			tip->tool_tip_view->GetMouse(&where, &buttons);
			tip->tool_tip_view->ConvertToScreen(&where);

			tip->stopped = tip->stop;
			if (tip->reset) {
				if (tip->showing)
					tip->tool_tip_window->Hide();
				tip->stop = false;
				tip->stopped = false;
				tip->reset = false;
				tip->shown = false;
				tip->showing = false;
				tip->start_time = system_time() + tip->settings.delay;
			}
			else if (tip->showing) {
				if ((tip->stop) ||
					(!tip->settings.enabled) ||
					(!tip->app_active) ||
					(!tip->bounds.Contains(where)) ||
					(tip->expire_time < system_time()) ||
					(abs((int)tip->start.x - (int)where.x) > kSLOP) ||
					(abs((int)tip->start.y - (int)where.y) > kSLOP) ||
					(buttons)) {
					tip->tool_tip_window->Hide();
					tip->shown = tip->settings.one_time_only;
					tip->showing = false;
					tip->tip_timed_out = (tip->expire_time < system_time());
					tip->start_time = system_time() + tip->settings.delay;
				}
			}
			else if ((tip->settings.enabled) &&
					 (!tip->stopped) &&
					 (tip->app_active) &&
					 (!tip->shown) &&
					 (!tip->tip_timed_out) &&
					 (!buttons) &&
					 (tip->bounds.Contains(where)) &&
					 (tip->start_time < system_time())) {
				tip->start = where;
				tip->tool_tip_view->AdjustWindow();
				tip->tool_tip_window->Show();
				tip->tool_tip_window->Activate(false);
				tip->showing = true;
				tip->expire_time = system_time() + tip->settings.hold;
				tip->start = where;
			}
			else if ((abs((int)tip->start.x - (int)where.x) > kSLOP) ||
					 (abs((int)tip->start.y - (int)where.y) > kSLOP)) {
				tip->start = where;
				tip->start_time = system_time() + tip->settings.delay;
				tip->tip_timed_out = false;
			}
			if (buttons)
				tip->start_time = system_time() + tip->settings.delay;
			tip->tool_tip_window->Unlock();
		}
		snooze(50000);
	}
	return B_NO_ERROR;
}