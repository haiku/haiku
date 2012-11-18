/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 *		Axel Dörfler, axeld@pinc-software.de
 */

//!	BAlert displays a modal alert window.

#include <new>
#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Autolock.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Button.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <IconUtils.h>
#include <Invoker.h>
#include <Looper.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Path.h>
#include <Resources.h>
#include <Screen.h>
#include <TextView.h>
#include <View.h>

#include <binary_compatibility/Interface.h>


//#define DEBUG_ALERT
#ifdef DEBUG_ALERT
#	define FTRACE(x) fprintf(x)
#else
#	define FTRACE(x) /* nothing */
#endif

// Default size of the Alert window.
#define DEFAULT_RECT	BRect(0, 0, 310, 75)
#define max(LHS, RHS)	((LHS) > (RHS) ? (LHS) : (RHS))

static const unsigned int kAlertButtonMsg = 'ALTB';
static const int kSemTimeOut = 50000;

static const int kLeftOffset = 10;
static const int kTopOffset = 6;
static const int kBottomOffset = 8;
static const int kRightOffset = 8;

static const int kButtonSpacing = 6;
static const int kButtonOffsetSpacing = 62;
static const int kButtonUsualWidth = 75;

static const int kWindowIconOffset = 27;
static const int kWindowMinOffset = 12;
static const int kWindowMinWidth = 310;
static const int kWindowOffsetMinWidth = 335;

static const int kIconStripeWidth = 30;

static const int kTextButtonOffset = 10;

static inline int32
icon_layout_scale()
{
#ifdef __HAIKU__
	return max_c(1, ((int32)be_plain_font->Size() + 15) / 16);
#endif
	return 1;
}


class TAlertView : public BView {
	public:
		TAlertView(BRect frame);
		TAlertView(BMessage* archive);
		~TAlertView();

		static TAlertView*	Instantiate(BMessage* archive);
		virtual status_t	Archive(BMessage* archive, bool deep = true) const;

		virtual void	Draw(BRect updateRect);

		// These functions (or something analogous) are missing from libbe.so's
		// dump.  I can only assume that the bitmap is a public var in the
		// original implementation -- or BAlert is a friend of TAlertView.
		// Neither one is necessary, since I can just add these.
		void			SetBitmap(BBitmap* Icon)	{ fIconBitmap = Icon; }
		BBitmap*		Bitmap()					{ return fIconBitmap; }

	private:
		BBitmap*	fIconBitmap;
};

class _BAlertFilter_ : public BMessageFilter {
	public:
		_BAlertFilter_(BAlert* Alert);
		~_BAlertFilter_();

		virtual filter_result Filter(BMessage* msg, BHandler** target);

	private:
		BAlert*	fAlert;
};


//	#pragma mark - BAlert


BAlert::BAlert(const char *title, const char *text, const char *button1,
		const char *button2, const char *button3, button_width width,
		alert_type type)
	: BWindow(DEFAULT_RECT, title, B_MODAL_WINDOW,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	_InitObject(text, button1, button2, button3, width, B_EVEN_SPACING, type);
}


BAlert::BAlert(const char *title, const char *text, const char *button1,
		const char *button2, const char *button3, button_width width,
		button_spacing spacing, alert_type type)
	: BWindow(DEFAULT_RECT, title, B_MODAL_WINDOW,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	_InitObject(text, button1, button2, button3, width, spacing, type);
}


BAlert::~BAlert()
{
	// Probably not necessary, but it makes me feel better.
	if (fAlertSem >= B_OK)
		delete_sem(fAlertSem);
}


BAlert::BAlert(BMessage* data)
	: BWindow(data)
{
	fInvoker = NULL;
	fAlertSem = -1;
	fAlertValue = -1;

	fTextView = (BTextView*)FindView("_tv_");

	fButtons[0] = (BButton*)FindView("_b0_");
	fButtons[1] = (BButton*)FindView("_b1_");
	fButtons[2] = (BButton*)FindView("_b2_");

	if (fButtons[2])
		SetDefaultButton(fButtons[2]);
	else if (fButtons[1])
		SetDefaultButton(fButtons[1]);
	else if (fButtons[0])
		SetDefaultButton(fButtons[0]);

	TAlertView* view = (TAlertView*)FindView("_master_");
	if (view)
		view->SetBitmap(_InitIcon());

	// Get keys
	char key;
	for (int32 i = 0; i < 3; ++i) {
		if (data->FindInt8("_but_key", i, (int8*)&key) == B_OK)
			fKeys[i] = key;
	}

	int32 temp;
	// Get alert type
	if (data->FindInt32("_atype", &temp) == B_OK)
		fMsgType = (alert_type)temp;

	// Get button width
	if (data->FindInt32("_but_width", &temp) == B_OK)
		fButtonWidth = (button_width)temp;

	AddCommonFilter(new(std::nothrow) _BAlertFilter_(this));
}


BArchivable*
BAlert::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data, "BAlert"))
		return NULL;

	return new(std::nothrow) BAlert(data);
}


status_t
BAlert::Archive(BMessage* data, bool deep) const
{
	status_t ret = BWindow::Archive(data, deep);

	// Stow the text
	if (ret == B_OK)
		ret = data->AddString("_text", fTextView->Text());

	// Stow the alert type
	if (ret == B_OK)
		ret = data->AddInt32("_atype", fMsgType);

	// Stow the button width
	if (ret == B_OK)
		ret = data->AddInt32("_but_width", fButtonWidth);

	// Stow the shortcut keys
	if (fKeys[0] || fKeys[1] || fKeys[2]) {
		// If we have any to save, we must save something for everyone so it
		// doesn't get confusing on the unarchive.
		if (ret == B_OK)
			ret = data->AddInt8("_but_key", fKeys[0]);
		if (ret == B_OK)
			ret = data->AddInt8("_but_key", fKeys[1]);
		if (ret == B_OK)
			ret = data->AddInt8("_but_key", fKeys[2]);
	}

	return ret;
}


void
BAlert::SetShortcut(int32 index, char key)
{
	if (index >= 0 && index < 3)
		fKeys[index] = key;
}


char
BAlert::Shortcut(int32 index) const
{
	if (index >= 0 && index < 3)
		return fKeys[index];

	return 0;
}


int32
BAlert::Go()
{
	fAlertSem = create_sem(0, "AlertSem");
	if (fAlertSem < B_OK) {
		Quit();
		return -1;
	}

	// Get the originating window, if it exists
	BWindow* window =
		dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));

	Show();

	// Heavily modified from TextEntryAlert code; the original didn't let the
	// blocked window ever draw.
	if (window) {
		status_t err;
		for (;;) {
			do {
				err = acquire_sem_etc(fAlertSem, 1, B_RELATIVE_TIMEOUT,
									  kSemTimeOut);
				// We've (probably) had our time slice taken away from us
			} while (err == B_INTERRUPTED);

			if (err == B_BAD_SEM_ID) {
				// Semaphore was finally nuked in MessageReceived
				break;
			}
			window->UpdateIfNeeded();
		}
	} else {
		// No window to update, so just hang out until we're done.
		while (acquire_sem(fAlertSem) == B_INTERRUPTED) {
		}
	}

	// Have to cache the value since we delete on Quit()
	int32 value = fAlertValue;
	if (Lock())
		Quit();

	return value;
}


status_t
BAlert::Go(BInvoker* invoker)
{
	fInvoker = invoker;
	Show();
	return B_OK;
}


void
BAlert::MessageReceived(BMessage* msg)
{
	if (msg->what != kAlertButtonMsg)
		return BWindow::MessageReceived(msg);

	int32 which;
	if (msg->FindInt32("which", &which) == B_OK) {
		if (fAlertSem < B_OK) {
			// Semaphore hasn't been created; we're running asynchronous
			if (fInvoker) {
				BMessage* out = fInvoker->Message();
				if (out && (out->ReplaceInt32("which", which) == B_OK
							|| out->AddInt32("which", which) == B_OK))
					fInvoker->Invoke();
			}
			PostMessage(B_QUIT_REQUESTED);
		} else {
			// Created semaphore means were running synchronously
			fAlertValue = which;

			// TextAlertVar does release_sem() below, and then sets the
			// member var.  That doesn't make much sense to me, since we
			// want to be able to clean up at some point.  Better to just
			// nuke the semaphore now; we don't need it any more and this
			// lets synchronous Go() continue just as well.
			delete_sem(fAlertSem);
			fAlertSem = -1;
		}
	}
}


void
BAlert::FrameResized(float newWidth, float newHeight)
{
	BWindow::FrameResized(newWidth, newHeight);
}


BButton*
BAlert::ButtonAt(int32 index) const
{
	if (index >= 0 && index < 3)
		return fButtons[index];

	return NULL;
}


BTextView*
BAlert::TextView() const
{
	return fTextView;
}


BHandler*
BAlert::ResolveSpecifier(BMessage* msg, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	return BWindow::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
BAlert::GetSupportedSuites(BMessage* data)
{
	return BWindow::GetSupportedSuites(data);
}


void
BAlert::DispatchMessage(BMessage* msg, BHandler* handler)
{
	BWindow::DispatchMessage(msg, handler);
}


void
BAlert::Quit()
{
	BWindow::Quit();
}


bool
BAlert::QuitRequested()
{
	return BWindow::QuitRequested();
}


BPoint
BAlert::AlertPosition(float width, float height)
{
	BPoint result(100, 100);

	BWindow* window =
		dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));

	BScreen screen(window);
 	BRect screenFrame(0, 0, 640, 480);
 	if (screen.IsValid())
 		screenFrame = screen.Frame();

	// Horizontally, we're smack in the middle
	result.x = screenFrame.left + (screenFrame.Width() / 2.0) - (width / 2.0);

	// This is probably sooo wrong, but it looks right on 1024 x 768
	result.y = screenFrame.top + (screenFrame.Height() / 4.0) - ceil(height / 3.0);

	return result;
}


status_t
BAlert::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BAlert::SetLayout(data->layout);
			return B_OK;
}
	}

	return BWindow::Perform(code, _data);
}


void BAlert::_ReservedAlert1() {}
void BAlert::_ReservedAlert2() {}
void BAlert::_ReservedAlert3() {}


void
BAlert::_InitObject(const char* text, const char* button0, const char* button1,
	const char* button2, button_width buttonWidth, button_spacing spacing,
	alert_type type)
{
	fInvoker = NULL;
	fAlertSem = -1;
	fAlertValue = -1;
	fButtons[0] = fButtons[1] = fButtons[2] = NULL;
	fTextView = NULL;
	fKeys[0] = fKeys[1] = fKeys[2] = 0;
	fMsgType = type;
	fButtonWidth = buttonWidth;

	// Set up the "_master_" view
	TAlertView* view = new(std::nothrow) TAlertView(Bounds());
	if (view == NULL)
		return;

	AddChild(view);
	view->SetBitmap(_InitIcon());

	// Must have at least one button
	if (button0 == NULL) {
		debugger("BAlerts must have at least one button.");
		button0 = "";
	}

	// Set up the buttons

	int32 buttonCount = 1;
	view->AddChild(fButtons[0] = _CreateButton(0, button0));

	if (button1 != NULL) {
		view->AddChild(fButtons[1] = _CreateButton(1, button1));
		buttonCount++;
	}

	if (button2 != NULL) {
		view->AddChild(fButtons[2] = _CreateButton(2, button2));
		buttonCount++;
	}

	// Find the widest button only if the widest value needs to be known.

	if (fButtonWidth == B_WIDTH_FROM_WIDEST) {
		float maxWidth = 0;
		for (int i = 0; i < buttonCount; i++) {
			float width = fButtons[i]->Bounds().Width();
			if (width > maxWidth)
				maxWidth = width;
		}

		// resize buttons
		for (int i = 0; i < buttonCount; i++) {
			fButtons[i]->ResizeTo(maxWidth, fButtons[i]->Bounds().Height());
		}
	}

	float defaultButtonFrameWidth = -fButtons[buttonCount - 1]->Bounds().Width() / 2.0f;
	SetDefaultButton(fButtons[buttonCount - 1]);
	defaultButtonFrameWidth += fButtons[buttonCount - 1]->Bounds().Width() / 2.0f;

	// Layout buttons

	float fontFactor = be_plain_font->Size() / 11.0f;

	for (int i = buttonCount - 1; i >= 0; --i) {
		float x = -fButtons[i]->Bounds().Width();
		if (i + 1 == buttonCount)
			x += Bounds().right - kRightOffset + defaultButtonFrameWidth;
		else
			x += fButtons[i + 1]->Frame().left - kButtonSpacing;

		if (buttonCount > 1 && i == 0 && spacing == B_OFFSET_SPACING)
			x -= kButtonOffsetSpacing * fontFactor;

		fButtons[i]->MoveTo(x, fButtons[i]->Frame().top);
	}

	// Adjust the window's width, if necessary

	int32 iconLayoutScale = icon_layout_scale();
	float totalWidth = kRightOffset + fButtons[buttonCount - 1]->Frame().right
		- defaultButtonFrameWidth - fButtons[0]->Frame().left;
	if (view->Bitmap()) {
		totalWidth += (kIconStripeWidth + kWindowIconOffset) * iconLayoutScale;
	} else
		totalWidth += kWindowMinOffset;

	float width = (spacing == B_OFFSET_SPACING
		? kWindowOffsetMinWidth : kWindowMinWidth) * fontFactor;

	ResizeTo(max_c(totalWidth, width), Bounds().Height());

	// Set up the text view

	BRect textViewRect(kLeftOffset, kTopOffset,
		Bounds().right - kRightOffset,
		fButtons[0]->Frame().top - kTextButtonOffset);
	if (view->Bitmap())
		textViewRect.left = (kWindowIconOffset
			+ kIconStripeWidth) * iconLayoutScale - 2;

	fTextView = new(std::nothrow) BTextView(textViewRect, "_tv_",
		textViewRect.OffsetByCopy(B_ORIGIN),
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	if (fTextView == NULL)
		return;

	fTextView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	fTextView->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
	fTextView->SetText(text, strlen(text));
	fTextView->MakeEditable(false);
	fTextView->MakeSelectable(false);
	fTextView->SetWordWrap(true);
	view->AddChild(fTextView);

	// Now resize the TextView vertically so that all the text is visible
	float textHeight = fTextView->TextHeight(0, fTextView->CountLines());
	textViewRect.OffsetTo(0, 0);
	textHeight -= textViewRect.Height();
	ResizeBy(0, textHeight);
	fTextView->ResizeBy(0, textHeight);
	textViewRect.bottom += textHeight;
	fTextView->SetTextRect(textViewRect);

	AddCommonFilter(new(std::nothrow) _BAlertFilter_(this));

	MoveTo(AlertPosition(Frame().Width(), Frame().Height()));
}


BBitmap*
BAlert::_InitIcon()
{
	// Save the desired alert type and set it to "empty" until
	// loading the icon was successful
	alert_type alertType = fMsgType;
	fMsgType = B_EMPTY_ALERT;

	// After a bit of a search, I found the icons in app_server. =P
	BBitmap* icon = NULL;
	BPath path;
	status_t status = find_directory(B_BEOS_SERVERS_DIRECTORY, &path);
	if (status < B_OK) {
		FTRACE((stderr, "BAlert::_InitIcon() - find_directory failed: %s\n",
			strerror(status)));
		return NULL;
	}

	path.Append("app_server");
	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status < B_OK) {
		FTRACE((stderr, "BAlert::_InitIcon() - BFile init failed: %s\n",
			strerror(status)));
		return NULL;
	}

	BResources resources;
	status = resources.SetTo(&file);
	if (status < B_OK) {
		FTRACE((stderr, "BAlert::_InitIcon() - BResources init failed: %s\n",
			strerror(status)));
		return NULL;
	}

	// Which icon are we trying to load?
	const char* iconName = "";	// Don't want any seg faults
	switch (alertType) {
		case B_INFO_ALERT:
			iconName = "info";
			break;
		case B_IDEA_ALERT:
			iconName = "idea";
			break;
		case B_WARNING_ALERT:
			iconName = "warn";
			break;
		case B_STOP_ALERT:
			iconName = "stop";
			break;

		default:
			// Alert type is either invalid or B_EMPTY_ALERT;
			// either way, we're not going to load an icon
			return NULL;
	}

	int32 iconSize = 32 * icon_layout_scale();
	// Allocate the icon bitmap
	icon = new(std::nothrow) BBitmap(BRect(0, 0, iconSize - 1, iconSize - 1),
		0, B_RGBA32);
	if (icon == NULL || icon->InitCheck() < B_OK) {
		FTRACE((stderr, "BAlert::_InitIcon() - No memory for bitmap\n"));
		delete icon;
		return NULL;
	}

	// Load the raw icon data
	size_t size = 0;
	const uint8* rawIcon;

#ifdef __HAIKU__
	// Try to load vector icon
	rawIcon = (const uint8*)resources.LoadResource(B_VECTOR_ICON_TYPE,
		iconName, &size);
	if (rawIcon != NULL
		&& BIconUtils::GetVectorIcon(rawIcon, size, icon) == B_OK) {
		// We have an icon, restore the saved alert type
		fMsgType = alertType;
		return icon;
	}
#endif

	// Fall back to bitmap icon
	rawIcon = (const uint8*)resources.LoadResource(B_LARGE_ICON_TYPE,
		iconName, &size);
	if (rawIcon == NULL) {
		FTRACE((stderr, "BAlert::_InitIcon() - Icon resource not found\n"));
		delete icon;
		return NULL;
	}

	// Handle color space conversion
#ifdef __HAIKU__
	if (icon->ColorSpace() != B_CMAP8) {
		BIconUtils::ConvertFromCMAP8(rawIcon, iconSize, iconSize,
			iconSize, icon);
	}
#else
	icon->SetBits(rawIcon, iconSize, 0, B_CMAP8);
#endif

	// We have an icon, restore the saved alert type
	fMsgType = alertType;

	return icon;
}


BButton*
BAlert::_CreateButton(int32 which, const char* label)
{
	BMessage* message = new BMessage(kAlertButtonMsg);
	if (message == NULL)
		return NULL;

	message->AddInt32("which", which);

	BRect rect;
	rect.top = Bounds().bottom - kBottomOffset;
	rect.bottom = rect.top;

	char name[32];
	snprintf(name, sizeof(name), "_b%" B_PRId32 "_", which);

	BButton* button = new(std::nothrow) BButton(rect, name, label, message,
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	if (button == NULL)
		return NULL;

	float width, height;
	button->GetPreferredSize(&width, &height);

	if (fButtonWidth == B_WIDTH_AS_USUAL) {
		float fontFactor = be_plain_font->Size() / 11.0f;
		width = max_c(width, kButtonUsualWidth * fontFactor);
	}

	button->ResizeTo(width, height);
	button->MoveBy(0.0f, -height);
	return button;
}


//	#pragma mark - TAlertView


TAlertView::TAlertView(BRect frame)
	:	BView(frame, "TAlertView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
		fIconBitmap(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


TAlertView::TAlertView(BMessage* archive)
	:	BView(archive),
		fIconBitmap(NULL)
{
}


TAlertView::~TAlertView()
{
	delete fIconBitmap;
}


TAlertView*
TAlertView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "TAlertView"))
		return NULL;

	return new(std::nothrow) TAlertView(archive);
}


status_t
TAlertView::Archive(BMessage* archive, bool deep) const
{
	return BView::Archive(archive, deep);
}


void
TAlertView::Draw(BRect updateRect)
{
	if (!fIconBitmap)
		return;

	// Here's the fun stuff
	BRect stripeRect = Bounds();
	int32 iconLayoutScale = icon_layout_scale();
	stripeRect.right = kIconStripeWidth * iconLayoutScale;
	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRect(stripeRect);

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmapAsync(fIconBitmap, BPoint(18 * iconLayoutScale,
		6 * iconLayoutScale));

}


//------------------------------------------------------------------------------
//	#pragma mark - _BAlertFilter_


_BAlertFilter_::_BAlertFilter_(BAlert* alert)
	: BMessageFilter(B_KEY_DOWN),
	fAlert(alert)
{
}


_BAlertFilter_::~_BAlertFilter_()
{
}


filter_result
_BAlertFilter_::Filter(BMessage* msg, BHandler** target)
{
	if (msg->what == B_KEY_DOWN) {
		char byte;
		if (msg->FindInt8("byte", (int8*)&byte) == B_OK) {
			for (int i = 0; i < 3; ++i) {
				if (byte == fAlert->Shortcut(i) && fAlert->ButtonAt(i)) {
					char space = ' ';
					fAlert->ButtonAt(i)->KeyDown(&space, 1);

					return B_SKIP_MESSAGE;
				}
			}
		}
	}

	return B_DISPATCH_MESSAGE;
}

