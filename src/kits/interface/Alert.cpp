//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Alert.cpp
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	BAlert displays a modal alert window.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <string.h>

// System Includes -------------------------------------------------------------
#include <Invoker.h>
#include <Looper.h>
#include <Message.h>
#include <MessageFilter.h>

#include <Alert.h>
#include <Bitmap.h>
#include <Button.h>
// TODO: Fix
// This is hacked in because something in Accelerant.h or SupportDefs.h is lame!
typedef unsigned int			uint;
#include <Screen.h>
#include <TextView.h>
#include <View.h>

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Resources.h>

#include <Autolock.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
#define DEFAULT_RECT	BRect(100, 100, 100, 100)
#define max(LHS, RHS)	((LHS) > (RHS) ? (LHS) : (RHS))

// Globals ---------------------------------------------------------------------
const unsigned int kAlertButtonMsg	= 'ALTB';
const int kSemTimeOut				= 50000;

const int kButtonBottomOffset		=   9;
const int kDefButtonBottomOffset	=   6;
const int kButtonRightOffset		=   6;
const int kButtonSpaceOffset		=   6;
const int kButtonOffsetSpaceOffset	=  26;
const int kButtonLeftOffset			=  62;
const int kButtonUsualWidth			=  75;

const int kWindowIconOffset			=  27;
const int kWindowMinWidth			= 310;
const int kWindowMinOffset			=  12;
const int kWindowOffsetMinWidth		= 335;

const int kIconStripeWidth			=  30;

const int kTextLeftOffset			=  10;
const int kTextIconOffset			=  kWindowIconOffset + kIconStripeWidth - 2;
const int kTextTopOffset			=   6;
const int kTextRightOffset			=  10;
const int kTextBottomOffset			=  45;

//------------------------------------------------------------------------------
class TAlertView : public BView
{
	public:
		TAlertView(BRect frame);
		TAlertView(BMessage* archive);
		~TAlertView();

		static TAlertView*	Instantiate(BMessage* archive);
		status_t			Archive(BMessage* archive, bool deep = true);

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
//------------------------------------------------------------------------------
// I'm making a guess based on the name and TextEntryAlert's implementation that
// this is a BMessageFilter.  I'm not sure, but I think I actually prefer how
// TextEntryAlert does it, but there are clearly no message filtering functions
// on BAlert so here we go.
class _BAlertFilter_ : public BMessageFilter
{
	public:
		_BAlertFilter_(BAlert* Alert);
		~_BAlertFilter_();

		virtual filter_result Filter(BMessage* msg, BHandler** target);

	private:
		BAlert*	fAlert;
};
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
BAlert::BAlert(const char *title, const char *text, const char *button1,
			   const char *button2, const char *button3, button_width width,
			   alert_type type)
	:	BWindow(DEFAULT_RECT, title, B_MODAL_WINDOW,
				B_NOT_CLOSABLE | B_NOT_RESIZABLE)
{
	InitObject(text, button1, button2, button3, width, B_EVEN_SPACING, type);
}
//------------------------------------------------------------------------------
BAlert::BAlert(const char *title, const char *text, const char *button1,
			   const char *button2, const char *button3, button_width width,
			   button_spacing spacing, alert_type type)
	:	BWindow(DEFAULT_RECT, title, B_MODAL_WINDOW,
				B_NOT_CLOSABLE | B_NOT_RESIZABLE)
{
	InitObject(text, button1, button2, button3, width, spacing, type);
}
//------------------------------------------------------------------------------
BAlert::~BAlert()
{
	// Probably not necessary, but it makes me feel better.
	if (fAlertSem >= B_OK)
	{
		delete_sem(fAlertSem);
	}
}
//------------------------------------------------------------------------------
BAlert::BAlert(BMessage* data)
	:	BWindow(data)
{
	BAutolock Autolock(this);
	if (Autolock.IsLocked())
	{
		fInvoker = NULL;
		fAlertSem = -1;
		fAlertVal = -1;

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

		TAlertView* Master = (TAlertView*)FindView("_master_");
		if (Master)
		{
			Master->SetBitmap(InitIcon());
		}

		// Get keys
		char key;
		for (int32 i = 0; i < 3; ++i)
		{
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

		AddCommonFilter(new _BAlertFilter_(this));
	}
}
//------------------------------------------------------------------------------
BArchivable* BAlert::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data, "BAlert"))
	{
		return NULL;
	}

	return new BAlert(data);
}
//------------------------------------------------------------------------------
status_t BAlert::Archive(BMessage* data, bool deep) const
{
	BWindow::Archive(data, deep);

	// Stow the text
	data->AddString("_text", fTextView->Text());

	// Stow the alert type
	data->AddInt32("_atype", fMsgType);

	// Stow the button width
	data->AddInt32("_but_width", fButtonWidth);

	// Stow the shortcut keys
	if (fKeys[0] || fKeys[1] || fKeys[2])
	{
		// If we have any to save, we must save something for everyone so it
		// doesn't get confusing on the unarchive.
		data->AddInt8("_but_key", fKeys[0]);
		data->AddInt8("_but_key", fKeys[1]);
		data->AddInt8("_but_key", fKeys[2]);
	}

	return B_OK;
}
//------------------------------------------------------------------------------
void BAlert::SetShortcut(int32 index, char key)
{
	if (index >= 0 && index < 3)
		fKeys[index] = key;
}
//------------------------------------------------------------------------------
char BAlert::Shortcut(int32 index) const
{
	if (index >= 0 && index < 3)
		return fKeys[index];

	return 0;
}
//------------------------------------------------------------------------------
int32 BAlert::Go()
{
	// TODO: Add sound?
	fAlertSem = create_sem(0, "AlertSem");
	if (fAlertSem < B_OK)
	{
		Quit();
		return -1;
	}

	// Get the originating window, if it exists
	BWindow* Window =
		dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));

	Show();

	// Heavily modified from TextEntryAlert code; the original didn't let the
	// blocked window ever draw.
	if (Window)
	{
		status_t err;
		for (;;)
		{
			do
			{
				err = acquire_sem_etc(fAlertSem, 1, B_RELATIVE_TIMEOUT,
									  kSemTimeOut);
				// We've (probably) had our time slice taken away from us
			} while (err == B_INTERRUPTED);
			if (err == B_BAD_SEM_ID)
			{
				// Semaphore was finally nuked in MessageReceived
				break;
			}
			Window->UpdateIfNeeded();
		}
	}
	else
	{
		// No window to update, so just hang out until we're done.
		while (acquire_sem(fAlertSem) == B_INTERRUPTED)
		{
			;
		}
	}

	// Have to cache the value since we delete on Quit()
	int32 value = fAlertVal;
	if (Lock())
	{
		Quit();
	}

	return value;
}
//------------------------------------------------------------------------------
status_t BAlert::Go(BInvoker* invoker)
{
	// TODO: Add sound?
	// It would be cool if we triggered a system sound depending on the type of
	// alert.
	fInvoker = invoker;
	Show();
	return B_OK;
}
//------------------------------------------------------------------------------
void BAlert::MessageReceived(BMessage* msg)
{
	if (msg->what == kAlertButtonMsg)
	{
		int32 which;
		if (msg->FindInt32("which", &which) == B_OK)
		{
			if (fAlertSem < B_OK)
			{
				// Semaphore hasn't been created; we're running asynchronous
				if (fInvoker)
				{
					BMessage* out = fInvoker->Message();
					if (out && (out->AddInt32("which", which) == B_OK ||
						out->ReplaceInt32("which", which) == B_OK))
					{
						fInvoker->Invoke();
					}
				}
			}
			else
			{
				// Created semaphore means were running synchronously
				fAlertVal = which;

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
}
//------------------------------------------------------------------------------
void BAlert::FrameResized(float new_width, float new_height)
{
	// Nothing in documentation; will have to test
	// TODO: Implement?
	BWindow::FrameResized(new_width, new_height);
}
//------------------------------------------------------------------------------
BButton* BAlert::ButtonAt(int32 index) const
{
	BButton* Button = NULL;
	if (index >= 0 && index < 3)
		Button = fButtons[index];

	return Button;
}
//------------------------------------------------------------------------------
BTextView* BAlert::TextView() const
{
	return fTextView;
}
//------------------------------------------------------------------------------
BHandler* BAlert::ResolveSpecifier(BMessage* msg, int32 index,
								   BMessage* specifier, int32 form,
								   const char* property)
{
	// Nothing in documentation; will have to test
	// TODO: Implement?
	return BWindow::ResolveSpecifier(msg, index, specifier, form, property);
}
//------------------------------------------------------------------------------
status_t BAlert::GetSupportedSuites(BMessage* data)
{
	// Nothing in documentation; will have to test
	// TODO: Implement?
	return BWindow::GetSupportedSuites(data);
}
//------------------------------------------------------------------------------
void BAlert::DispatchMessage(BMessage* msg, BHandler* handler)
{
	// Nothing in documentation; will have to test
	// TODO: Implement?
	BWindow::DispatchMessage(msg, handler);
}
//------------------------------------------------------------------------------
void BAlert::Quit()
{
	// Nothing in documentation; will have to test
	// TODO: Implement?
	BWindow::Quit();
}
//------------------------------------------------------------------------------
bool BAlert::QuitRequested()
{
	// Nothing in documentation; will have to test
	// TODO: Implement?
	return BWindow::QuitRequested();
}
//------------------------------------------------------------------------------
BPoint BAlert::AlertPosition(float width, float height)
{
	BPoint result(100, 100);

	BWindow* Window =
		dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));

	BScreen Screen(Window);
	if (!Screen.IsValid())
	{
		// ... then we're in deep trouble
		// But what to say about it?
		// debugger(???);
	}

	BRect screenRect = Screen.Frame();

	// Horizontally, we're smack in the middle
	result.x = (screenRect.Width() / 2.0) - (width / 2.0);

	// This is probably sooo wrong, but it looks right on 1024 x 768
	result.y = (screenRect.Height() / 4.0) - ceil(height / 3.0);

	return result;
}
//------------------------------------------------------------------------------
status_t BAlert::Perform(perform_code d, void* arg)
{
	return BWindow::Perform(d, arg);
}
//------------------------------------------------------------------------------
void BAlert::_ReservedAlert1()
{
	;
}
//------------------------------------------------------------------------------
void BAlert::_ReservedAlert2()
{
	;
}
//------------------------------------------------------------------------------
void BAlert::_ReservedAlert3()
{
	;
}
//------------------------------------------------------------------------------
void BAlert::InitObject(const char* text, const char* button0,
						const char* button1, const char* button2,
						button_width width, button_spacing spacing,
						alert_type type)
{
	BAutolock Autolock(this);
	if (Autolock.IsLocked())
	{
		fInvoker = NULL;
		fAlertSem = -1;
		fAlertVal = -1;
		fButtons[0] = fButtons[1] = fButtons[2] = NULL;
		fTextView = NULL;
		fKeys[0] = fKeys[1] = fKeys[2] = 0;
		fMsgType = type;
		fButtonWidth = width;
	
		// Set up the "_master_" view
		TAlertView* MasterView = new TAlertView(Bounds());
		MasterView->SetBitmap(InitIcon());
		AddChild(MasterView);
	
		// Set up the buttons
		int buttonCount = 0;
	
		// Have to have at least one button
		if (button0 == NULL)
		{
			debugger("BAlert's must have at least one button.");
			button0 = "";
		}
	
		BMessage ProtoMsg(kAlertButtonMsg);
		ProtoMsg.AddInt32("which", 0);
		fButtons[0] = new BButton(BRect(0, 0, 0, 0), "_b0_", button0,
								  new BMessage(ProtoMsg),
								  B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		++buttonCount;
	
		if (button1)
		{
			ProtoMsg.ReplaceInt32("which", 1);
			fButtons[buttonCount] = new BButton(BRect(0, 0, 0, 0), "_b1_", button1,
												new BMessage(ProtoMsg),
												B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
			++buttonCount;
		}
	
		if (button2)
		{
			ProtoMsg.ReplaceInt32("which", 2);
			fButtons[buttonCount] = new BButton(BRect(0, 0, 0, 0), "_b2_", button2,
												new BMessage(ProtoMsg),
												B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
			++buttonCount;
		}
	
		SetDefaultButton(fButtons[buttonCount - 1]);

		float buttonWidth = 0;
		float buttonHeight = 0;
		for (int i = 0; i < buttonCount; ++i)
		{
			float temp;
			fButtons[i]->GetPreferredSize(&temp, &buttonHeight);
			buttonWidth = max(buttonWidth, temp);
		}
	
		// Add first, because the buttons will ResizeToPreferred()
		// in AttachedToWindow()
		for (int i = 0; i < buttonCount; ++i)
		{
			MasterView->AddChild(fButtons[i]);
		}

		for (int i = buttonCount - 1; i >= 0; --i)
		{
			switch (fButtonWidth)
			{
				case B_WIDTH_FROM_WIDEST:
					fButtons[i]->ResizeTo(buttonWidth, buttonHeight);
					break;
					
				case B_WIDTH_FROM_LABEL:
					fButtons[i]->ResizeToPreferred();
					break;
	
				default:	// B_WIDTH_AS_USUAL
					fButtons[i]->GetPreferredSize(&buttonWidth, &buttonHeight);
					buttonWidth = max(buttonWidth, kButtonUsualWidth);
					fButtons[i]->ResizeTo(buttonWidth, buttonHeight);
					break;
			}

			float buttonX;
			float buttonY;
			float temp;

			fButtons[i]->GetPreferredSize(&temp, &buttonHeight);
			buttonY = Bounds().bottom - buttonHeight;

			if (i == buttonCount - 1)	// the right-most button
			{
				buttonX = Bounds().right - fButtons[i]->Frame().Width() -
						  kButtonRightOffset;
				buttonY -= kDefButtonBottomOffset;
			}
			else
			{
				buttonX = fButtons[i + 1]->Frame().left -
						  fButtons[i]->Frame().Width() -
						  kButtonSpaceOffset;
	
				if (i == 0)
				{
					if (spacing == B_OFFSET_SPACING)
					{
						buttonX -= kButtonOffsetSpaceOffset;
					}
					else if (buttonCount == 3)
					{
						buttonX -= 3;
					}
				}
				buttonY -= kButtonBottomOffset;
			}
	
			fButtons[i]->MoveTo(buttonX, buttonY);
		}
	
	
		// Resize the window, if necessary
		float totalWidth = kButtonRightOffset;
		totalWidth += fButtons[buttonCount - 1]->Frame().right -
					  fButtons[0]->Frame().left;
		if (MasterView->Bitmap())
		{
			totalWidth += kIconStripeWidth + kWindowIconOffset;
		}
		else
		{
			totalWidth += kWindowMinOffset;
		}

		if (spacing == B_OFFSET_SPACING)
		{
			totalWidth = max(kWindowOffsetMinWidth, totalWidth);
		}
		else
		{
			totalWidth += 5;
			totalWidth = max(kWindowMinWidth, totalWidth);
		}
		ResizeTo(totalWidth, Bounds().Height());
	
		// Set up the text view
		BRect TextViewRect(kTextLeftOffset, kTextTopOffset,
						   Bounds().right - kTextRightOffset,
						   Bounds().bottom - kTextBottomOffset);
		if (MasterView->Bitmap())
		{
			TextViewRect.left = kTextIconOffset;
		}
	
		fTextView = new BTextView(TextViewRect, "_tv_",
								  TextViewRect,
								  B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
		fTextView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fTextView->SetText(text, strlen(text));
		fTextView->MakeEditable(false);
		fTextView->MakeSelectable(false);
		fTextView->SetWordWrap(true);
	
		// Now resize the window vertically so that all the text is visible
		float textHeight = fTextView->TextHeight(0, fTextView->CountLines());
		TextViewRect.OffsetTo(0, 0);
		textHeight -= TextViewRect.Height();
		ResizeBy(0, textHeight);
		fTextView->ResizeBy(0, textHeight);
		TextViewRect.bottom += textHeight;
		fTextView->SetTextRect(TextViewRect);
	
		MasterView->AddChild(fTextView);
	
		AddCommonFilter(new _BAlertFilter_(this));

		MoveTo(AlertPosition(Frame().Width(), Frame().Height()));
	}
}
//------------------------------------------------------------------------------
BBitmap* BAlert::InitIcon()
{
	// After a bit of a search, I found the icons in app_server. =P
	BBitmap* Icon = NULL;
	BPath Path;
	if (find_directory(B_BEOS_SERVERS_DIRECTORY, &Path) == B_OK)
	{
		Path.Append("app_server");
		BFile File;
		if (File.SetTo(Path.Path(), B_READ_ONLY) == B_OK)
		{
			BResources Resources;
			if (Resources.SetTo(&File) == B_OK)
			{
				// Which icon are we trying to load?
				const char* iconName = "";	// Don't want any seg faults
				switch (fMsgType)
				{
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
						return Icon;
				}

				// Load the raw icon data
				size_t size;
				const void* rawIcon =
					Resources.LoadResource('ICON', iconName, &size);

				if (rawIcon)
				{
					// Now build the bitmap
					Icon = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
					Icon->SetBits(rawIcon, size, 0, B_CMAP8);
				}
			}
		}
	}

	if (!Icon)
	{
		// If there's no icon, it's an empty alert indeed.
		fMsgType = B_EMPTY_ALERT;
	}

	return Icon;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//	#pragma mark -
//	#pragma mark TAlertView
//	#pragma mark -
//------------------------------------------------------------------------------
TAlertView::TAlertView(BRect frame)
	:	BView(frame, "TAlertView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
		fIconBitmap(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
//------------------------------------------------------------------------------
TAlertView::TAlertView(BMessage* archive)
	:	BView(archive),
		fIconBitmap(NULL)
{
}
//------------------------------------------------------------------------------
TAlertView::~TAlertView()
{
	if (fIconBitmap)
	{
		delete fIconBitmap;
	}
}
//------------------------------------------------------------------------------
TAlertView* TAlertView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "TAlertView"))
	{
		return NULL;
	}

	return new TAlertView(archive);
}
//------------------------------------------------------------------------------
status_t TAlertView::Archive(BMessage* archive, bool deep)
{
	return BView::Archive(archive, deep);
}
//------------------------------------------------------------------------------
void TAlertView::Draw(BRect updateRect)
{
	// Here's the fun stuff
	if (fIconBitmap)
	{
		BRect StripeRect = Bounds();
		StripeRect.right = kIconStripeWidth;
		SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		FillRect(StripeRect);

		SetDrawingMode(B_OP_OVER);
		DrawBitmapAsync(fIconBitmap, BPoint(18, 6));
		SetDrawingMode(B_OP_COPY);
	}
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//	#pragma mark -
//	#pragma mark _BAlertFilter_
//	#pragma mark -
//------------------------------------------------------------------------------
_BAlertFilter_::_BAlertFilter_(BAlert* Alert)
	:	BMessageFilter(B_KEY_DOWN),
		fAlert(Alert)
{
}
//------------------------------------------------------------------------------
_BAlertFilter_::~_BAlertFilter_()
{
	;
}
//------------------------------------------------------------------------------
filter_result _BAlertFilter_::Filter(BMessage* msg, BHandler** target)
{
	if (msg->what == B_KEY_DOWN)
	{
		char byte;
		if (msg->FindInt8("byte", (int8*)&byte) == B_OK)
		{
			for (int i = 0; i < 3; ++i)
			{
				if (byte == fAlert->Shortcut(i) && fAlert->ButtonAt(i))
				{
					char space = ' ';
					fAlert->ButtonAt(i)->KeyDown(&space, 1);

					return B_SKIP_MESSAGE;
				}
			}
		}
	}

	return B_DISPATCH_MESSAGE;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

