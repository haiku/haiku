// IFSSaver.cpp


#include <math.h>
#include <stdio.h>

#include <Catalog.h>
#include <CheckBox.h>
#include <Slider.h>
#include <TextView.h>

#include "IFSSaver.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screensaver IFS"


enum {
	MSG_TOGGLE_ADDITIVE		= 'tgad',
	MSG_SET_SPEED			= 'stsp',
};

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver*
instantiate_screen_saver(BMessage *message, image_id image)
{
	return new IFSSaver(message, image);
}

// constructor
IFSSaver::IFSSaver(BMessage *message, image_id id)
	: BScreenSaver(message, id),
	  BHandler("IFS Saver"),
	  fIFS(NULL),
	  fIsPreview(false),
	  fBounds(0.0, 0.0, -1.0, -1.0),
	  fLastDrawnFrame(0),
	  fAdditive(false),
	  fSpeed(6)
{
	fDirectInfo.bits = NULL;
	fDirectInfo.bytesPerRow = 0;

	if (message) {
		if (message->FindBool("IFS additive", &fAdditive) < B_OK)
			fAdditive = false;
		if (message->FindInt32("IFS speed", &fSpeed) < B_OK)
			fSpeed = 6;
	}
}

// destructor
IFSSaver::~IFSSaver()
{
	if (Looper()) {
		Looper()->RemoveHandler(this);
	}
	_Cleanup();
}

// StartConfig
void
IFSSaver::StartConfig(BView *view)
{
	BRect bounds = view->Bounds();
	bounds.InsetBy(10.0, 10.0);
	BRect frame(0.0, 0.0, bounds.Width(), 20.0);

	// the additive check box
	fAdditiveCB = new BCheckBox(frame, "additive setting",
								B_TRANSLATE("Render dots additive"),
								new BMessage(MSG_TOGGLE_ADDITIVE),
								B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);

	fAdditiveCB->SetValue(fAdditive);

	fAdditiveCB->ResizeToPreferred();
	bounds.bottom -= fAdditiveCB->Bounds().Height() * 2.0;
	fAdditiveCB->MoveTo(bounds.LeftBottom());

	view->AddChild(fAdditiveCB);

	// the additive check box
	fSpeedS = new BSlider(frame, "speed setting",
						  B_TRANSLATE("Morphing speed:"),
						  new BMessage(MSG_SET_SPEED),
						  1, 12, B_BLOCK_THUMB,
						  B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);

	fSpeedS->SetValue(fSpeed);
	fSpeedS->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fSpeedS->SetHashMarkCount(12);

	fSpeedS->ResizeToPreferred();
	bounds.bottom -= fSpeedS->Bounds().Height() + 15.0;
	fSpeedS->MoveTo(bounds.LeftBottom());

	view->AddChild(fSpeedS);

	// the info text view
	BRect textRect = bounds;
	textRect.OffsetTo(0.0, 0.0);
	BTextView* textView = new BTextView(bounds, B_EMPTY_STRING, textRect,
										B_FOLLOW_ALL, B_WILL_DRAW);
	textView->SetViewColor(view->ViewColor());
	
	BString aboutScreenSaver(B_TRANSLATE("%screenSaverName%\n\n"
					 ""B_UTF8_COPYRIGHT" 1997 Massimino Pascal\n\n"
					 "xscreensaver port by Stephan Aßmus\n"
					 "<stippi@yellowbites.com>"));
	BString screenSaverName(B_TRANSLATE("Iterated Function System"));

	aboutScreenSaver.ReplaceFirst("%screenSaverName%", screenSaverName);
	textView->Insert(aboutScreenSaver);

	textView->SetStylable(true);
	textView->SetFontAndColor(0, screenSaverName.Length(), be_bold_font);
//	textView->SetFontAndColor(25, 255, be_plain_font);

	textView->MakeEditable(false);

	view->AddChild(textView);

	// make sure we receive messages from the views we added
	if (BWindow* window = view->Window())
		window->AddHandler(this);

	fAdditiveCB->SetTarget(this);
	fSpeedS->SetTarget(this);
}

// StartSaver
status_t
IFSSaver::StartSaver(BView *v, bool preview)
{
	display_mode mode;
	BScreen screen(B_MAIN_SCREEN_ID);
	screen.GetMode(&mode);
	float totalSize = mode.timing.h_total * mode.timing.v_total;
	float fps = mode.timing.pixel_clock * 1000.0 / totalSize;

//printf("ticks per frame: %lldµs\n", (int64)floor(1000000.0 / fps + 0.5));
	SetTickSize((int64)floor(1000000.0 / fps + 0.5));

	fIsPreview = preview;
	fBounds = v->Bounds();

	_Init(fBounds);
	fIFS->SetAdditive(fIsPreview || fAdditive);
	fIFS->SetSpeed(fSpeed);

	return B_OK;
}

// StopSaver
void
IFSSaver::StopSaver()
{
	_Cleanup();
}

// DirectConnected
void
IFSSaver::DirectConnected(direct_buffer_info* info)
{
//printf("IFSSaver::DirectConnected()\n");
	int32 request = info->buffer_state & B_DIRECT_MODE_MASK;
	switch (request) {
		case B_DIRECT_START:
//printf("B_DIRECT_START\n");
//printf("  bits_per_pixel: %ld\n", info->bits_per_pixel);
//printf("   bytes_per_row: %ld\n", info->bytes_per_row);
//printf("    pixel_format: %d\n", info->pixel_format);
//printf("   window_bounds: (%ld, %ld, %ld, %ld)\n",
//	info->window_bounds.left, info->window_bounds.top,
//	info->window_bounds.right, info->window_bounds.bottom);
			fDirectInfo.bits = info->bits;
			fDirectInfo.bytesPerRow = info->bytes_per_row;
			fDirectInfo.bits_per_pixel = info->bits_per_pixel;
			fDirectInfo.format = info->pixel_format;
			fDirectInfo.bounds = info->window_bounds;
			break;
		case B_DIRECT_STOP:
			fDirectInfo.bits = NULL;
			break;
		default:
			break;
	}
//printf("bits: %p\n", fDirectInfo.bits);
}

// Draw
void
IFSSaver::Draw(BView *view, int32 frame)
{
//printf("IFSSaver::Draw(%ld) (%ldx%ld)\n", frame, view->Bounds().IntegerWidth() + 1, view->Bounds().IntegerHeight() + 1);
	if (frame == 0) {
		fLastDrawnFrame = -1;
		view->SetHighColor(0, 0, 0);
		view->FillRect(view->Bounds());
	}
	int32 frames = frame - fLastDrawnFrame;
	if ((fIsPreview || fDirectInfo.bits == NULL) && fLocker.Lock()) {
		fIFS->Draw(view, NULL, frames);

		fLastDrawnFrame = frame;
		fLocker.Unlock();
	}
}

// DirectDraw
void
IFSSaver::DirectDraw(int32 frame)
{
//bigtime_t now = system_time();
//printf("IFSSaver::DirectDraw(%ld)\n", frame);
	if (frame == 0)
		fLastDrawnFrame = -1;
	int32 frames = frame - fLastDrawnFrame;
	if (fDirectInfo.bits) {
		fIFS->Draw(NULL, &fDirectInfo, frames);
//printf("DirectDraw(): %lldµs\n", system_time() - now);
		fLastDrawnFrame = frame;
	}
}

// SaveState
status_t
IFSSaver::SaveState(BMessage* into) const
{
	status_t ret = B_BAD_VALUE;
	if (into) {
		ret = into->AddBool("IFS additive", fAdditive);
		if (ret >= B_OK)
			ret = into->AddInt32("IFS speed", fSpeed);
	}
	return ret;
}

// MessageReceived
void
IFSSaver::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_TOGGLE_ADDITIVE:
			if (fLocker.Lock() && fIFS) {
				fAdditive = fAdditiveCB->Value() == B_CONTROL_ON;
				fIFS->SetAdditive(fAdditive || fIsPreview);
				fLocker.Unlock();
			}
			break;
		case MSG_SET_SPEED:
			if (fLocker.Lock() && fIFS) {
				fSpeed = fSpeedS->Value();
				fIFS->SetSpeed(fSpeed);
				fLocker.Unlock();
			}
			break;
		default:
			BHandler::MessageReceived(message);
	}
}

// _Init
void
IFSSaver::_Init(BRect bounds)
{
	if (fLocker.Lock()) {
		delete fIFS;
		fIFS = new IFS(bounds);
		fLocker.Unlock();
	}
}

// _Cleanup
void
IFSSaver::_Cleanup()
{
	if (fLocker.Lock()) {
		delete fIFS;
		fIFS = NULL;
		fLocker.Unlock();
	}
}

