/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//
// ComboBox.cpp
//
//

/*
	TODO:
		- Better up/down arrow handling (if text in input box matches a list item,
		  pressing down should select the next item, pressing up should select the
		  previous.  If no item matched, the first or last item should be selected.
		  In any case, pressing up or down should show the popup if it is hidden
		- Properly draw the label, taking alignment into account
		- Draw nicer border around text input and popup window
		- Escaping out of the popup menu should restore the text in the input to the
		  value it had previous to popping up the menu.
		- Fix popup behavior when the widget is near the bottom of the screen.  The
		  popup window should be able to go above the text input area.  Also, the popup
		  should size itself in a smart manner so that it is small if there are few
		  choices and large if there are many and the window under it is big.  Perhaps
		  the developer should be able to influence the size of the popup.
		- Improve button drawing and (?) button behavior
		- Fix and test enable/disable behavior
		- Add auto-scrolling and/or drag-scrolling to the poup-menu
		- Add support for other navigation keys, like page up, page down, home, end.
		- Fix up choice functions (remove choice, add at index, etc) and make sure they
		  properly invalidate/scroll/etc the list when it is visible
		- Change auto-complete behavior to be non-greedy, or perhaps add some type of
		  tab-cycling to the choices
		- Add mode whereby you can pop up a list of only those items that match 
*/

#include <Button.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <ListItem.h>
#include <ListView.h>
#include <Menu.h>			// for menu_info
#include <MessageFilter.h>
#include "ObjectList.h"
#include <ScrollBar.h>
#include <String.h>
#include <TextControl.h>
#include <Window.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ComboBox.h"

//static const uint32 kTextControlInvokeMessage	= 'tCIM';
static const uint32 kTextInputModifyMessage = 'tIMM';
static const uint32 kPopupButtonInvokeMessage = 'pBIM';
static const uint32 kPopupWindowHideMessage = 'pUWH';
static const uint32 kWindowMovedMessage = 'wMOV';

static const float kTextInputMargin = (float)3.0;
static const float kLabelRightMargin = (float)6.0;
static const float kButtonWidth = (float)15.0;

#define disable_color(_c_) tint_color(_c_, B_DISABLED_LABEL_TINT)

#define TV_MARGIN 3.0
#define TV_DIVIDER_MARGIN 6.0

rgb_color create_color(uchar r, uchar g, uchar b, uchar a = 255);

rgb_color create_color(uchar r, uchar g, uchar b, uchar a) {
	rgb_color col;
	col.red = r;
	col.green = g;
	col.blue = b;
	col.alpha = a;
	return col;
}

class StringObjectList : public BObjectList<BString> {};

// ----------------------------------------------------------------------------

// ChoiceListView is similar to a BListView, but it's implementation is tied to
// BComboBox.  BListView is not used because it requires that a BStringItem be
// created for each choice.  ChoiceListView just pulls the choice strings
// directly from the BComboBox and draws them.
class BComboBox::ChoiceListView : public BView
{
	public:
		ChoiceListView(	BRect frame, BComboBox *parent);
		virtual ~ChoiceListView();
			
		virtual void Draw(BRect update);
		virtual void MouseDown(BPoint where);
		virtual void MouseUp(BPoint where);
		virtual void MouseMoved(BPoint where, uint32 transit,
			const BMessage *dragMessage);
		virtual void KeyDown(const char *bytes, int32 numBytes);
		virtual void SetFont(const BFont *font, uint32 properties = B_FONT_ALL);

		void ScrollToSelection();
		void InvalidateItem(int32 index, bool force = false);
		BRect ItemFrame(int32 index);
		void AdjustScrollBar();
		// XXX: add BArchivable functionality
	
	private:
		inline float LineHeight();
		
		BPoint fClickLoc;
		font_height fFontHeight;
		bigtime_t fClickTime;
		rgb_color fForeCol;
		rgb_color fBackCol;
		rgb_color fSelCol;
		int32 fSelIndex;
		BComboBox *fParent;
		bool fTrackingMouseDown;
};


// ----------------------------------------------------------------------------

// TextInput is a somewhat modified version of the _BTextInput_ class defined
// in TextControl.cpp.

class BComboBox::TextInput : public BTextView {
	public:
		TextInput(BRect rect, BRect trect, ulong rMask, ulong flags);
		TextInput(BMessage *data);
		virtual ~TextInput();
		static BArchivable *Instantiate(BMessage *data);
		virtual status_t Archive(BMessage *data, bool deep = true) const;
	
		virtual void KeyDown(const char *bytes, int32 numBytes);
		virtual void MakeFocus(bool state);
		virtual void FrameResized(float x, float y);
		virtual void Paste(BClipboard *clipboard);
	
		void AlignTextRect();
	
		void SetInitialText();
		void SetFilter(text_input_filter_hook hook);

		// XXX: add BArchivable functionality
	
	protected:
		virtual void InsertText(const char *inText, int32 inLength, int32 inOffset,
			const text_run_array *inRuns);
		virtual void DeleteText(int32 fromOffset, int32 toOffset);
	
	private:
		char *fInitialText;
		text_input_filter_hook fFilter;
		bool fClean;
};

// ----------------------------------------------------------------------------

class BComboBox::ComboBoxWindow : public BWindow
{
	public:
		ComboBoxWindow(BComboBox *box);
		virtual ~ComboBoxWindow();
		virtual void WindowActivated(bool active);
		virtual void FrameResized(float width, float height);
		
		void DoPosition();
		BComboBox::ChoiceListView *ListView();
		BScrollBar *ScrollBar();
	
		// XXX: add BArchivable functionality
		
	private:
		BScrollBar			*fScrollBar;
		ChoiceListView		*fListView;
		BComboBox			*fParent;
};

// ----------------------------------------------------------------------------

// In BeOS R4.5, SetEventMask(B_POINTER_EVENTS, ...) does not work for getting
// all mouse events as they happen.  Specifically, when the user clicks on the
// window dressing (the borders or the title tab) no mouse event will be
// delivered until after the user releases the mouse button.  This has the
// unfortunate side effect of allowing the user to move the window that
// contains the BComboBox around with no notification being sent to the
// BComboBox.  We need to intercept the B_WINDOW_MOVED messages so that we can
// hide the popup window when the window moves.

class BComboBox::MovedMessageFilter : public BMessageFilter
{
	public:
		MovedMessageFilter(BHandler *target);
		virtual filter_result Filter(BMessage *message, BHandler **target);
		
	private:
		BHandler *fTarget;
};


// ----------------------------------------------------------------------------


BComboBox::ChoiceListView::ChoiceListView(BRect frame, BComboBox *parent)
	: BView(frame, "_choice_list_view_", B_FOLLOW_ALL_SIDES, B_WILL_DRAW
			| B_NAVIGABLE),
	fClickLoc(-100, -100)
{
	fParent = parent;
	GetFontHeight(&fFontHeight);
	menu_info mi;
	get_menu_info(&mi);
	fForeCol = create_color(0, 0, 0);
	fBackCol = mi.background_color;
	fSelCol  = create_color(144, 144, 144);
	SetViewColor(B_TRANSPARENT_COLOR);
	SetHighColor(fForeCol);
	fTrackingMouseDown = false;
	fClickTime = 0;
}


BComboBox::ChoiceListView::~ChoiceListView()
{
}


void BComboBox::ChoiceListView::Draw(BRect update)
{
	float h = LineHeight();
	BRect rect(Bounds());
	int32 index;
	int32 choices = fParent->fChoiceList->CountChoices();
	int32 selected = (fTrackingMouseDown) ? fSelIndex : fParent->CurrentSelection();

	// draw each visible item
	for (index = (int32)floor(update.top / h); index < choices; index++)
	{
		rect.top = index * h;
		rect.bottom = rect.top + h;
		SetLowColor((index == selected) ? fSelCol : fBackCol);
		FillRect(rect, B_SOLID_LOW);
		DrawString(fParent->fChoiceList->ChoiceAt(index), BPoint(rect.left + 2,
			rect.bottom - fFontHeight.descent - 1));
	}

	// draw empty area on bottom
	if (rect.bottom < update.bottom)
	{
		update.top = rect.bottom;
		SetLowColor(fBackCol);
		FillRect(update, B_SOLID_LOW);
	}
}


void BComboBox::ChoiceListView::MouseDown(BPoint where)
{
	BRect rect(Window()->Frame());
	ConvertFromScreen(&rect);
	if (!rect.Contains(where))
	{
		// hide the popup window when the user clicks outside of it
		if (fParent->Window()->Lock())
		{
			fParent->HidePopupWindow();
			fParent->Window()->Unlock();
		}

		// HACK: the window is locked and unlocked so that it will get
		// activated before we potentially send the mouse down event in the
		// code below.  Is there a way to wait until the window is activated
		// before sending the mouse down? Should we call
		// fParent->Window()->MakeActive(true) here?
		
		if (fParent->Window()->Lock())
		{
			// resend the mouse event to the textinput, if necessary
			BTextView *text = fParent->TextView();
			BPoint screenWhere(ConvertToScreen(where));
			rect = text->Window()->ConvertToScreen(text->Frame());
			if (rect.Contains(screenWhere))
			{
				//printf("  resending mouse down to textinput\n");
				BMessage *msg = new BMessage(*Window()->CurrentMessage());
				msg->RemoveName("be:view_where");
				text->ConvertFromScreen(&screenWhere);
				msg->AddPoint("be:view_where", screenWhere);
				text->Window()->PostMessage(msg, text);
				delete msg;
			}
			fParent->Window()->Unlock();
		}
		
		return;
	}

	rect = Bounds();
	if (!rect.Contains(where))
		return;
	
	fTrackingMouseDown = true;
	// check for double click
	bigtime_t now = system_time();
	bigtime_t clickSpeed;
	get_click_speed(&clickSpeed);
	if ((now - fClickTime < clickSpeed)
		&& ((abs((int)(fClickLoc.x - where.x)) < 3)
		&& (abs((int)(fClickLoc.y - where.y)) < 3)))
	{
		// this is a double click
		// XXX: what to do here?
		printf("BComboBox::ChoiceListView::MouseDown() -- unhandled double click\n");
	}
	fClickTime = now;
	fClickLoc = where;

	float h = LineHeight();
	int32 oldIndex = fSelIndex;
	fSelIndex = (int32)floor(where.y / h);
	int32 choices = fParent->fChoiceList->CountChoices();
	if (fSelIndex < 0 || fSelIndex >= choices)
		fSelIndex = -1;

	if (oldIndex != fSelIndex)
	{
		InvalidateItem(oldIndex);
		InvalidateItem(fSelIndex);
	}
	// XXX: this probably isn't necessary since we are doing a SetEventMask
	// whenever the popup window becomes visible which routes all mouse events
	// to this view
//	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}


void BComboBox::ChoiceListView::MouseUp(BPoint /*where*/)
{
	if (fTrackingMouseDown)
	{
		fTrackingMouseDown = false;
		if (fSelIndex >= 0)
			fParent->Select(fSelIndex, true);
		else
			fParent->Deselect();
	}
//	fClickLoc = where;
}


void BComboBox::ChoiceListView::MouseMoved(BPoint where, uint32 /*transit*/,
	const BMessage */*dragMessage*/)
{
	if (fTrackingMouseDown)
	{
		float h = LineHeight();
		int32 oldIndex = fSelIndex;
		fSelIndex = (int32)floor(where.y / h);
		int32 choices = fParent->fChoiceList->CountChoices();
		if (fSelIndex < 0 || fSelIndex >= choices)
			fSelIndex = -1;

		if (oldIndex != fSelIndex)
		{
			InvalidateItem(oldIndex);
			InvalidateItem(fSelIndex);
		}
	}
}


void BComboBox::ChoiceListView::KeyDown(const char *bytes, int32 /*numBytes*/)
{
	BComboBox *cb = fParent;
	BWindow *win = cb->Window();
	BComboBox::TextInput *text = dynamic_cast<BComboBox::TextInput*>(cb->TextView());
	uchar aKey = bytes[0];

	switch (aKey)
	{
		case B_UP_ARROW:	// fall through
		case B_DOWN_ARROW:
			if (win->Lock())
			{
				// change the selection
				int32 index = cb->CurrentSelection();
				int32 choices = cb->fChoiceList->CountChoices();
				if (choices > 0)
				{
					if (index < 0)
					{
						// no previous selection, so select first or last item
						// depending on whether this is a up or down arrow
						cb->Select((aKey == B_UP_ARROW) ? choices - 1 : 0);
					}
					else
					{
						// select the previous or the next item, if possible,
						// depending on whether this is an up or down arrow
						if (aKey == B_UP_ARROW && (index - 1 >= 0))
							cb->Select(index - 1, true);
						else if (aKey == B_DOWN_ARROW && (index + 1 < choices))
							cb->Select(index + 1, true);
					}
				}
				win->Unlock();
			}
			break;
		default:
		{	// send all other key down events to the text input view
			BMessage *msg = Window()->DetachCurrentMessage();
			if (msg) {
				win->PostMessage(msg, text);
				delete msg;
			}
			break;
		}
	}
}


void BComboBox::ChoiceListView::SetFont(const BFont *font, uint32 properties)
{
	BView::SetFont(font, properties);
	GetFontHeight(&fFontHeight);
	Invalidate();
}


void BComboBox::ChoiceListView::ScrollToSelection()
{
	int32 selected = fParent->CurrentSelection();
	if (selected >= 0)
	{
		BRect frame(ItemFrame(selected));
		BRect bounds(Bounds());
		float newY = -1.0; // dummy value -- not used
        bool doScroll = false;

		if (frame.bottom > bounds.bottom)
		{
			newY = frame.bottom - bounds.Height();
			doScroll = true;
		}
		else if (frame.top < bounds.top)
		{
			newY = frame.top;
			doScroll = true;
		}
		if (doScroll)
			ScrollTo(bounds.left, newY);
	}
}

// InvalidateItem() only does a real invalidate if the index is valid or the
// force flag is turned on

void BComboBox::ChoiceListView::InvalidateItem(int32 index, bool force)
{
	int32 choices = fParent->fChoiceList->CountChoices();
	if ((index >= 0 && index < choices) || force) {
		Invalidate(ItemFrame(index));
	}
}

// This method doesn't check the index to see if it is valid, it just returns
// the BRect that an item and the index would have if it existed.

BRect BComboBox::ChoiceListView::ItemFrame(int32 index)
{
	BRect rect(Bounds());
	float h = LineHeight();
	rect.top = index * h;
	rect.bottom = rect.top + h;
	return rect;
}

// The window must be locked before this method is called

void BComboBox::ChoiceListView::AdjustScrollBar()
{
	BScrollBar *sb = ScrollBar(B_VERTICAL);
	if (sb) {
		float h = LineHeight();
		float max = h * fParent->fChoiceList->CountChoices();
		BRect frame(Frame());
		float diff = max - frame.Height();
		float prop = frame.Height() / max;
		if (diff < 0) {
			diff = 0.0;
			prop = 1.0;
		}
		sb->SetSteps(h, h * (frame.IntegerHeight() / h));
		sb->SetRange(0.0, diff);
		sb->SetProportion(prop);
	}	
}


float BComboBox::ChoiceListView::LineHeight()
{
	return fFontHeight.ascent + fFontHeight.descent + fFontHeight.leading + 2;
}


// ----------------------------------------------------------------------------
//	#pragma mark -


BComboBox::TextInput::TextInput(BRect rect, BRect textRect, ulong rMask,
		ulong flags)
	: BTextView(rect, "_input_", textRect, be_plain_font, NULL, rMask, flags),
	fFilter(NULL)
{
	MakeResizable(true);
	fInitialText = NULL;
	fClean = false;
}


BComboBox::TextInput::TextInput(BMessage *data)
	: BTextView(data),
	fFilter(NULL)
{
	MakeResizable(true);
	fInitialText = NULL;
	fClean = false;
}


BComboBox::TextInput::~TextInput()
{
	free(fInitialText);
}


status_t
BComboBox::TextInput::Archive(BMessage* data, bool /*deep*/) const
{
	return BTextView::Archive(data);
}


BArchivable *
BComboBox::TextInput::Instantiate(BMessage* data)
{
	// XXX: is "TextInput" the correct name for this class? Perhaps
	// BComboBox::TextInput?
	if (!validate_instantiation(data, "TextInput"))
		return NULL;

	return new TextInput(data);
}


void
BComboBox::TextInput::SetInitialText()
{
	if (fInitialText) {
		free(fInitialText);
		fInitialText = NULL;
	}
	if (Text())
		fInitialText = strdup(Text());
}


void
BComboBox::TextInput::SetFilter(text_input_filter_hook hook)
{
	fFilter = hook;
}


void
BComboBox::TextInput::KeyDown(const char *bytes, int32 numBytes)
{
	BComboBox* cb;
	uchar aKey = bytes[0];

	switch (aKey) {
		case B_RETURN:
			cb = dynamic_cast<BComboBox*>(Parent());
			ASSERT(cb);

			if (!cb->IsEnabled())
				break;

			ASSERT(fInitialText);
			if (strcmp(fInitialText, Text()) != 0)
				cb->CommitValue();
			free(fInitialText);
			fInitialText = strdup(Text());
			{
				int32 end = TextLength();
				Select(end, end);
			}
			// hide popup window if it's showing when the user presses the
			// enter key
			if (cb->fPopupWindow && cb->fPopupWindow->Lock()) {
				if (!cb->fPopupWindow->IsHidden()) {
					cb->HidePopupWindow();
				}
				cb->fPopupWindow->Unlock();
			}
			break;
		case B_TAB:
//			cb = dynamic_cast<BComboBox*>Parent());
//			ASSERT(cb);
//			if (cb->fAutoComplete && cb->fCompletionIndex >= 0) {	
//				int32 from, to;
//				cb->fText->GetSelection(&from, &to);
//				if (from == to) {
//					// HACK: this should never happen.  The rest of the class
//					// should be fixed so that fCompletionIndex is set to -1 if the
//					// text is modified
//					printf("BComboBox::TextInput::KeyDown() -- HACK! this shouldn't happen!");
//					cb->fCompletionIndex = -1;
//				}
//				
//				const char *text = cb->fText->Text();
//				BString prefix;
//				prefix.Append(text, from);
//				
//				int32 match;
//				const char *completion;
//				if (cb->fChoiceList->GetMatch(	prefix.String(),
//											  	cb->fCompletionIndex + 1,
//												&match,
//												&completion) == B_OK)
//				{
//					cb->fText->Delete(); 			// delete the selection
//					cb->fText->Insert(completion);
//					cb->fText->Select(from, from + strlen(completion));
//					cb->fCompletionIndex = match;
//					cb->Select(cb->fCompletionIndex);
//				} else {
//					//system_beep();
//				}	
//			} else {
				BView::KeyDown(bytes, numBytes);
//			}
			break;
#if 0
		case B_UP_ARROW:		// fall through
		case B_DOWN_ARROW:
			cb = dynamic_cast<BComboBox*>(Parent());
			ASSERT(cb);
			if (cb->fChoiceList) {
				cb = dynamic_cast<BComboBox*>(Parent());
				ASSERT(cb);
				if (!(cb->fPopupWindow)) {
					cb->fPopupWindow = cb->CreatePopupWindow();
				}
				if (cb->fPopupWindow->Lock()) {
					// show popup window, if needed
					if (cb->fPopupWindow->IsHidden()) {
						cb->ShowPopupWindow();
					} else {
						printf("Whoa!!! Erroneously got up/down arrow key down in TextInput::KeyDown()!\n");
					}
					int32 index = cb->CurrentSelection();
					int32 choices = cb->fChoiceList->CountChoices();
					// select something, if no selection
					if (index < 0 && choices > 0) {
						if (aKey == B_UP_ARROW) {
							cb->Select(choices - 1);
						} else {
							cb->Select(0);
						}
					}
					cb->fPopupWindow->Unlock();
				}
			}
			break;
#endif
		case B_ESCAPE:
			cb = dynamic_cast<BComboBox*>(Parent());
			ASSERT(cb);
			if (cb->fChoiceList)
			{
				cb = dynamic_cast<BComboBox*>(Parent());
				ASSERT(cb);
				if (cb->fPopupWindow && cb->fPopupWindow->Lock())
				{
					if (!cb->fPopupWindow->IsHidden())
						cb->HidePopupWindow();

					cb->fPopupWindow->Unlock();
				}
			}
			break;
		case ',':
			{
				int32 startSel, endSel;
				GetSelection(&startSel, &endSel);
				int32 length = TextLength();
				if (endSel == length)
					Select(endSel, endSel);
				BTextView::KeyDown(bytes, numBytes);
			}
			break;
		default:
			BTextView::KeyDown(bytes, numBytes);
			break;
	}
}


void
BComboBox::TextInput::MakeFocus(bool state)
{
//+	PRINT(("_BTextInput_::MakeFocus(state=%d, view=%s)\n", state,
//+		Parent()->Name()));
	if (state == IsFocus())
		return;

	BComboBox* parent = dynamic_cast<BComboBox*>(Parent());
	ASSERT(parent);

	BTextView::MakeFocus(state);

	if (state) {
		SetInitialText();
		fClean = true;			// text hasn't been dirtied yet.

		BMessage *m;
		if (Window() && (m = Window()->CurrentMessage()) != 0
			&& m->what == B_KEY_DOWN) {
			// we're being focused by a keyboard event, so
			// select all...
			SelectAll();
		}
	} else {
		ASSERT(fInitialText);
		if (strcmp(fInitialText, Text()) != 0)
			parent->CommitValue();

		free(fInitialText);
		fInitialText = NULL;
		fClean = false;
		BMessage *m;
		if (Window() && (m = Window()->CurrentMessage()) != 0 && m->what == B_MOUSE_DOWN)
			Select(0,0);

		// hide popup window if it's showing when the text input loses focus
		if (parent->fPopupWindow && parent->fPopupWindow->Lock()) {
			if (!parent->fPopupWindow->IsHidden())
				parent->HidePopupWindow();

			parent->fPopupWindow->Unlock();
		}
	}

	// make sure the focus indicator gets drawn or undrawn
	if (Window()) {
		BRect invalRect(Bounds());
		invalRect.InsetBy(-kTextInputMargin, -kTextInputMargin);
		parent->Draw(invalRect);
		parent->Flush();
	}
}


void
BComboBox::TextInput::FrameResized(float x, float y)
{
	BTextView::FrameResized(x, y);
	AlignTextRect();
}


void
BComboBox::TextInput::Paste(BClipboard *clipboard)
{
	BTextView::Paste(clipboard);
	Invalidate();
}


// What a hack...
void
BComboBox::TextInput::AlignTextRect()
{
	BRect bounds = Bounds();
	BRect textRect = TextRect();

	switch (Alignment()) {
		default:
		case B_ALIGN_LEFT:
			textRect.OffsetTo(B_ORIGIN);
			break;

		case B_ALIGN_CENTER:
			textRect.OffsetTo((bounds.Width() - textRect.Width()) / 2,
				textRect.top);
			break;

		case B_ALIGN_RIGHT:
			textRect.OffsetTo(bounds.Width() - textRect.Width(), textRect.top);
			break;
	}

	SetTextRect(textRect);
}


void
BComboBox::TextInput::InsertText(const char *inText, int32 inLength, 
	int32 inOffset, const text_run_array *inRuns)
{
	char* ptr = NULL;

	// strip out any return characters
	// limiting to a reasonable amount of chars for a text control.
	// otherwise this code could malloc some huge amount which isn't good.
	if (strpbrk(inText, "\r\n") && inLength <= 1024) {
		int32 len = inLength;
		ptr = (char *)malloc(len + 1);
		if (ptr) {
			strncpy(ptr, inText, len);
			ptr[len] = '\0';

			char *p = ptr;

			while (len--) {
				if (*p == '\n')
					*p = ' ';
				else if (*p == '\r')
					*p = ' ';

				p++;
			}
		}
	}

	if (fFilter != NULL)
		inText = fFilter(inText, inLength, inRuns);
	BTextView::InsertText(ptr ? ptr : inText, inLength, inOffset, inRuns);

	BComboBox *parent = dynamic_cast<BComboBox *>(Parent());
	if (parent) {
		if (parent->fModificationMessage)
			parent->Invoke(parent->fModificationMessage);

		BMessage *msg;
		parent->Window()->PostMessage(msg = new BMessage(kTextInputModifyMessage),
			parent);
		delete msg;
	}

	if (ptr)
		free(ptr);
}


void
BComboBox::TextInput::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);
	BComboBox *parent = dynamic_cast<BComboBox *>(Parent());
	if (parent) {
		if (parent->fModificationMessage)
			parent->Invoke(parent->fModificationMessage);

		BMessage *msg;
		parent->Window()->PostMessage(msg = new BMessage(kTextInputModifyMessage),
			parent);
		delete msg;
	}
}


//	#pragma mark -


BComboBox::ComboBoxWindow::ComboBoxWindow(BComboBox *box)
	: BWindow(BRect(0, 0, 10, 10), NULL,  B_BORDERED_WINDOW_LOOK,
		B_FLOATING_SUBSET_WINDOW_FEEL, B_NOT_MOVABLE | B_NOT_RESIZABLE
			| B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
			| B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS)
{
	fParent = box;
	DoPosition();
	BWindow *parentWin = fParent->Window();
	if (parentWin)
		AddToSubset(parentWin);

	BRect rect(Bounds());
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	fListView = new ChoiceListView(rect, fParent);
	AddChild(fListView);
	rect.left = rect.right;
	rect.right += B_V_SCROLL_BAR_WIDTH;
	fScrollBar = new BScrollBar(rect, "_popup_scroll_bar_", fListView, 0, 1000,
		B_VERTICAL);
	AddChild(fScrollBar);
	fListView->AdjustScrollBar();
}


BComboBox::ComboBoxWindow::~ComboBoxWindow()
{
	fListView->RemoveSelf();
	delete fListView;
}


void BComboBox::ComboBoxWindow::WindowActivated(bool /*active*/)
{
//	if (active)
//		fListView->AdjustScrollBar();
}


void BComboBox::ComboBoxWindow::FrameResized(float /*width*/, float /*height*/)
{
	fListView->AdjustScrollBar();
}


void BComboBox::ComboBoxWindow::DoPosition()
{
	BRect winRect(fParent->fText->Frame());
	winRect = fParent->ConvertToScreen(winRect);
//	winRect.left += fParent->Divider() + 5;
	winRect.right -= 2;
	winRect.OffsetTo(winRect.left, winRect.bottom + kTextInputMargin);
	winRect.bottom = winRect.top + 100;
	MoveTo(winRect.LeftTop());
	ResizeTo(winRect.IntegerWidth(), winRect.IntegerHeight());
}


BComboBox::ChoiceListView *BComboBox::ComboBoxWindow::ListView()
{
	return fListView;
}


BScrollBar *BComboBox::ComboBoxWindow::ScrollBar()
{
	return fScrollBar;
}


// ----------------------------------------------------------------------------
//	#pragma mark -


BComboBox::BComboBox(BRect frame, const char *name, const char *label,
	BMessage *message, uint32 resizeMask, uint32 flags)
	: BControl(frame, name, label, message, resizeMask,
		flags | B_WILL_DRAW | B_FRAME_EVENTS),
	  fPopupWindow(NULL),
	  fModificationMessage(NULL),
	  fChoiceList(0),
	  fLabelAlign(B_ALIGN_LEFT),
	  fAutoComplete(false),
	  fButtonDepressed(false),
	  fDepressedWhenClicked(false),
	  fTrackingButtonDown(false),
	  fFrameCache(frame)
{
	// If the user wants this control to be keyboard navigable, then we really
	// want the underlying text view to be navigable, not this view.
	bool navigate = ((Flags() & B_NAVIGABLE) != 0);
	if (navigate)
	{
		fSkipSetFlags = true;
		SetFlags(Flags() & ~B_NAVIGABLE);	// disable navigation for this
		fSkipSetFlags = false;
	}

	fDivider = StringWidth(label);

	BRect rect(frame);
	rect.OffsetTo(0, 0);
	rect.left += fDivider + kLabelRightMargin;
//	rect.right -= kButtonWidth + 1;
//	rect.right;
	rect.InsetBy(kTextInputMargin, kTextInputMargin);
	BRect textRect(rect);
	textRect.OffsetTo(0, 0);
	textRect.left += 2;
	textRect.right -= 2;
	
	fText = new TextInput(rect, textRect, B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
		B_WILL_DRAW | B_FRAME_EVENTS | (navigate ? B_NAVIGABLE : 0));
	float height = fText->LineHeight();
	rect.bottom = rect.top + height;
//	fText->ResizeTo(rect.IntegerWidth(), height);
	AddChild(fText);

	font_height	fontInfo;
	GetFontHeight(&fontInfo);
	float h1 = ceil(fontInfo.ascent + fontInfo.descent + fontInfo.leading);
	float h2 = fText->LineHeight();

	// Height of main view must be the larger of h1 and h2+(TV_MARGIN*2)
	float h = (h1 > h2 + (TV_MARGIN*2)) ? h1 : h2 + (TV_MARGIN*2);
	BRect b = Bounds();
	ResizeTo(b.Width(), h);
	b.bottom = h;

	// set height and position of text entry view
	fText->ResizeTo(fText->Bounds().Width(), h2);
	// vertically center this view
	fText->MoveBy(0, (b.Height() - (h2+(TV_MARGIN*2))) / 2);

	rect.left = rect.right + 1;
	rect.right = rect.left + kButtonWidth;

	fButtonRect = rect;
	fTextEnd = 0;
	fSelected = -1;
	fCompletionIndex = -1;
	fWinMovedFilter = new MovedMessageFilter(this);
}


BComboBox::~BComboBox()
{
	if (fPopupWindow && fPopupWindow->Lock())
		fPopupWindow->Quit();

	RemoveChild(fText);
	delete fText;

	if (fWinMovedFilter->Looper())
		fWinMovedFilter->Looper()->RemoveFilter(fWinMovedFilter);

	delete fWinMovedFilter;
	
}


void BComboBox::SetChoiceList(BChoiceList *list)
{
//	delete fChoiceList;
	fChoiceList = list;
	ChoiceListUpdated();
}


BChoiceList *BComboBox::ChoiceList()
{
	return fChoiceList;
}


void BComboBox::ChoiceListUpdated()
{
	if (fPopupWindow && fPopupWindow->Lock())
	{
		if (!fPopupWindow->IsHidden())
		{
			// do an invalidate on the choice list
			fPopupWindow->ListView()->Invalidate();
			fPopupWindow->ListView()->AdjustScrollBar();
			// XXX: change the selection and select the proper item, if possible
		}
		fPopupWindow->Unlock();
	}
}


//void BComboBox::AddChoice(const char *text)
//{
//	fChoiceList.AddItem((char *)text);
//	if (fPopupWindow && fPopupWindow->Lock()) {
//		if (!fPopupWindow->IsHidden()) {
//			// do an invalidate on the new item's location
//			int32 index = CountChoices() - 1;
//			fPopupWindow->ListView()->InvalidateItem(index);
//			fPopupWindow->ListView()->AdjustScrollBar();
//		}
//		fPopupWindow->Unlock();
//	}
//}


//const char *BComboBox::ChoiceAt(int32 index)
//{
//	return (const char *)fChoiceList.ItemAt(index);
//}


//int32 BComboBox::CountChoices()
//{
//	return fChoiceList.CountItems();
//}


void
BComboBox::Select(int32 index, bool changeTextSelection)
{
	int32 oldIndex = fSelected;
	if (index < fChoiceList->CountChoices() && index >= 0) {
		BWindow *win = Window();
		bool gotLock = (win && win->Lock());
		if (!win || gotLock) {
			fSelected = index;
			if (fPopupWindow && fPopupWindow->Lock()) {
				ChoiceListView *lv = fPopupWindow->ListView();
				lv->InvalidateItem(oldIndex);
				lv->InvalidateItem(fSelected);		
				lv->ScrollToSelection();
				fPopupWindow->Unlock();
			}

			if (changeTextSelection) {
				// Find last coma
				const char *ptr = fText->Text();
				const char *end;
				int32 tlength = fText->TextLength();

				for (end = ptr+tlength-1; end>ptr; end--) {
					if (*end == ',') {
						// Find end of whitespace
						for (end++; isspace(*end); end++) {}
						break;
					}
				}
				int32 soffset = end-ptr;
				int32 eoffset = tlength;
				if (end != 0)
					fText->Delete(soffset, eoffset);

				tlength = strlen(fChoiceList->ChoiceAt(fSelected));
				fText->Insert(soffset, fChoiceList->ChoiceAt(fSelected), tlength);
				eoffset = fText->TextLength();
				fText->Select(soffset, eoffset);
//				fText->SetText(fChoiceList->ChoiceAt(fSelected));
//				fText->SelectAll();
			}

			if (gotLock)
				win->Unlock();
		}
	} else {
		Deselect();
		return;
	}
}


void
BComboBox::Deselect()
{
	BWindow *win = Window();
	bool gotLock = (win && win->Lock());
	if (!win || gotLock) {
		int32 oldIndex = fSelected;
		fSelected = -1;
		// invalidate the old selected item, if needed
		if (oldIndex >= 0 && fPopupWindow && fPopupWindow->Lock()) {
			fPopupWindow->ListView()->InvalidateItem(oldIndex);
			fPopupWindow->Unlock();
		}

		if (gotLock)
			win->Unlock();
	}
}


int32
BComboBox::CurrentSelection()
{
	return fSelected;
}


void
BComboBox::SetAutoComplete(bool on)
{
	fAutoComplete = on;
}


bool
BComboBox::GetAutoComplete()
{
	return fAutoComplete;
}


void
BComboBox::SetLabel(const char *text)
{
	BControl::SetLabel(text);
	BRect invalRect = Bounds();
	invalRect.right = fDivider;
	Invalidate(invalRect);
}


void
BComboBox::SetValue(int32 value)
{
	BControl::SetValue(value);
}


void
BComboBox::SetText(const char *text)
{
	fText->SetText(text);
	if (fText->IsFocus())
		fText->SetInitialText();

	fText->Invalidate();
}


const char *
BComboBox::Text() const
{
	return fText->Text();
}


BTextView *
BComboBox::TextView()
{
	return fText;
}


void
BComboBox::SetDivider(float divide)
{
	float diff = fDivider - divide;
	fDivider = divide;

	fText->MoveBy(-diff, 0);
	fText->ResizeBy(diff, 0);

	if (Window()) {
		fText->Invalidate();
		Invalidate();
	}
}


float
BComboBox::Divider() const
{
	return fDivider;
}


void
BComboBox::SetAlignment(alignment label, alignment text)
{
	fText->SetAlignment(text);
	fText->AlignTextRect();

	if (fLabelAlign != label) {
		fLabelAlign = label;
		Invalidate();
	}
}


void
BComboBox::GetAlignment(alignment *label, alignment *text) const
{
	*text = fText->Alignment();
	*label = fLabelAlign;
}


void
BComboBox::SetModificationMessage(BMessage *message)
{
	delete fModificationMessage;	
	fModificationMessage = message;
}


BMessage *
BComboBox::ModificationMessage() const
{
	return fModificationMessage;
}


void
BComboBox::SetFilter(text_input_filter_hook hook)
{
	fText->SetFilter(hook);
}


void
BComboBox::GetPreferredSize(float */*width*/, float */*height*/)
{
//	BFont font;
//	GetFont(&font);
//
//	*width = Bounds().IntegerWidth();
//	if (Label() != NULL) {
//		float strWidth = font.StringWidth(Label());
//		*width = ceil(kTextInputMargin + strWidth + kLabelRightMargin + 
//					  (strWidth * 1.50) + kTextInputMargin);
//	}
//
//	font_height	finfo;
//	float		h1;
//	float		h2;
//
//	font.GetHeight(&finfo);
//	h1 = ceil(finfo.ascent + finfo.descent + finfo.leading);
//	h2 = fText->LineHeight();
//
//	// Height of main view must be the larger of h1 and h2+(kTextInputMargin*2)
//	*height = ceil((h1 > h2 + (kTextInputMargin*2)) ? h1 : h2 + (kTextInputMargin*2));
}


void
BComboBox::ResizeToPreferred()
{
	BControl::ResizeToPreferred();
}


void
BComboBox::FrameMoved(BPoint new_position)
{
	if (fPopupWindow && fPopupWindow->Lock()) {
		fPopupWindow->MoveBy(new_position.x - fFrameCache.left,
			new_position.y - fFrameCache.top); 
		fPopupWindow->Unlock();
	}
	fFrameCache.OffsetTo(new_position);
}


void
BComboBox::FrameResized(float new_width, float new_height)
{
	// It's the cheese!
	float dx = new_width - fFrameCache.Width();
	float dy = new_height - fFrameCache.Height();
	if (dx != 0 && Window()) {
//		BRect inval(fFrameCache.right, fFrameCache.top,
//			fFrameCache.right+dx, fFrameCache.bottom);
		BRect inval(Bounds());
		if (dx > 0)
			inval.left = inval.right-dx-1;
		else
			inval.left = inval.right-3;
//		Window()->ConvertToScreen(&inval);
//		ConvertFromScreen(&inval);
		Invalidate(inval);
	}

	fFrameCache.right += dx;
	fFrameCache.bottom += dy;
//	fButtonRect.OffsetBy(dx, 0);

	if (fPopupWindow && fPopupWindow->Lock()) {
		if (!fPopupWindow->IsHidden())
			HidePopupWindow();

		fPopupWindow->Unlock();
	}
}


void
BComboBox::WindowActivated(bool /*active*/)
{
	if (fText->IsFocus())
		Draw(Bounds());
}


void
BComboBox::Draw(BRect /*updateRect*/)
{
	BRect bounds = Bounds();
	font_height	fInfo;
	rgb_color high = HighColor();
	rgb_color base = ViewColor();
	bool focused;
	bool enabled;
	rgb_color white = {255, 255, 255, 255};
	rgb_color black = { 0, 0, 0, 255 };

	enabled = IsEnabled();
	focused = fText->IsFocus() && Window()->IsActive();

	BRect fr = fText->Frame();

	fr.InsetBy(-3, -3);
	fr.bottom -= 1;
	if (enabled)
		SetHighColor(tint_color(base, B_DARKEN_1_TINT));
	else
		SetHighColor(base);

	StrokeLine(fr.LeftBottom(), fr.LeftTop());
	StrokeLine(fr.RightTop());

	if (enabled)
		SetHighColor(white);
	else
		SetHighColor(tint_color(base, B_LIGHTEN_2_TINT));

	StrokeLine(fr.LeftBottom()+BPoint(1,0), fr.RightBottom());
	StrokeLine(fr.RightTop()+BPoint(0,1));
	fr.InsetBy(1,1);

	if (focused) {
		// draw UI indication for 'active'
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(fr);
	} else {
		if (enabled)
			SetHighColor(tint_color(base, B_DARKEN_4_TINT));
		else
			SetHighColor(tint_color(base, B_DARKEN_2_TINT));
		StrokeLine(fr.LeftBottom(), fr.LeftTop());
		StrokeLine(fr.RightTop());
		SetHighColor(base);
		StrokeLine(fr.LeftBottom()+BPoint(1,0), fr.RightBottom());
		StrokeLine(fr.RightTop()+BPoint(0,1));
	}

	fr.InsetBy(1,1);

	if (!enabled)
		SetHighColor(tint_color(base, B_DISABLED_MARK_TINT));
	else
		SetHighColor(white);

	StrokeRect(fr);
	SetHighColor(high);	

	bounds.right = bounds.left + fDivider;
	if ((Label()) && (fDivider > 0.0)) {
		BPoint	loc;
		GetFontHeight(&fInfo);

		switch (fLabelAlign) {
			default:
			case B_ALIGN_LEFT:
				loc.x = bounds.left + TV_MARGIN;
				break;
			case B_ALIGN_CENTER:
			{
				float width = StringWidth(Label());
				float center = (bounds.right - bounds.left) / 2;
				loc.x = center - (width/2);
				break;
			}
			case B_ALIGN_RIGHT:
			{
				float width = StringWidth(Label());
				loc.x = bounds.right - width - TV_MARGIN;
				break;
			}
		}
		
		uint32 rmode = ResizingMode();
		if ((rmode & _rule_(0xf, 0, 0xf, 0)) == _rule_(_VIEW_TOP_, 0, _VIEW_BOTTOM_, 0))
			loc.y = fr.bottom - 2;
		else
			loc.y = bounds.bottom - (2 + ceil(fInfo.descent));

		MovePenTo(loc);
		SetHighColor(black);
		DrawString(Label());
		SetHighColor(high);
	}
}


void
BComboBox::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kTextInputModifyMessage:
			TryAutoComplete();
			break;
		case kPopupButtonInvokeMessage:
			if (fChoiceList && fChoiceList->CountChoices() && !fPopupWindow)
				fPopupWindow = CreatePopupWindow();

			if (fPopupWindow->Lock()) {
				if (fPopupWindow->IsHidden())
					ShowPopupWindow();
				else
					HidePopupWindow();

				fPopupWindow->Unlock();
			}
			break;
		case kWindowMovedMessage:
			if (fPopupWindow && fPopupWindow->Lock()) {
				if (!fPopupWindow->IsHidden())
					HidePopupWindow();

				fPopupWindow->Unlock();
			}
			break;
		default:
			BControl::MessageReceived(msg);
	}
}


void
BComboBox::MouseDown(BPoint where)
{
//	printf("BComboBox::MouseDown(%f, %f)\n", where.x, where.y);
	/*if (fButtonRect.Contains(where)) { 		// clicked in button area
		fDepressedWhenClicked = fButtonDepressed;
		fButtonDepressed = !fButtonDepressed;
		fTrackingButtonDown = true;
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
		Invalidate(fButtonRect);
		fText->MakeFocus(true);
	}*/
	BControl::MouseDown(where);
}


void
BComboBox::MouseUp(BPoint /*where*/)
{
	if (fTrackingButtonDown) {
		// send an invoke message when the button changes state
		if (fButtonDepressed != fDepressedWhenClicked) {
			BMessage *msg;
			Window()->PostMessage(msg = new BMessage(kPopupButtonInvokeMessage),this);
			delete msg;
		}
		fTrackingButtonDown = false;
	}
}


void
BComboBox::MouseMoved(BPoint where, uint32 /*transit*/,const BMessage */*dragMessage*/)
{
	if (fTrackingButtonDown) {
		BRect sloppyRect = fButtonRect;
		sloppyRect.InsetBy(-3, -3);

		bool oldState = fButtonDepressed;
		fButtonDepressed = sloppyRect.Contains(where) ? !fDepressedWhenClicked
			: fDepressedWhenClicked;

		if (oldState != fButtonDepressed)
			Invalidate(fButtonRect);
	}
}


status_t
BComboBox::Invoke(BMessage *msg)
{
	return BControl::Invoke(msg);
}


void
BComboBox::AttachedToWindow()
{
	Window()->AddFilter(fWinMovedFilter);
	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
		SetLowColor(ViewColor());
	}

	bool enabled = IsEnabled();
	rgb_color mc = HighColor();
	rgb_color base;
	BFont textFont;

	// mc used to be base in this line
	if (mc.red == 255 && mc.green == 255 && mc.blue == 255)
		base = ViewColor();
	else
		base = LowColor();

	fText->GetFontAndColor(0, &textFont);
	mc = enabled ? mc : disable_color(base);

	fText->SetFontAndColor(&textFont, B_FONT_ALL, &mc);

	if (!enabled)
		base = tint_color(base, B_DISABLED_MARK_TINT);
	else
		base.red = base.green = base.blue = 255;

	fText->SetLowColor(base);
	fText->SetViewColor(base);

	fText->MakeEditable(enabled);
}


void
BComboBox::DetachedFromWindow()
{
	fWinMovedFilter->Looper()->RemoveFilter(fWinMovedFilter);
}


void
BComboBox::SetFlags(uint32 flags)
{
	if (!fSkipSetFlags) {
		uint32 te_flags = fText->Flags();
		bool te_nav = ((te_flags & B_NAVIGABLE) != 0);
		bool wants_nav = ((flags & B_NAVIGABLE) != 0);

		// the ComboBox should never be navigable
		ASSERT((Flags() & B_NAVIGABLE) == 0);

		if (!te_nav && wants_nav) {
			// The combo box wants to be navigable. Pass that along to
			// the text view
			fText->SetFlags(te_flags | B_NAVIGABLE);
		} else if (te_nav && !wants_nav) {
			// Caller wants to end NAV on the text view;
			fText->SetFlags(te_flags & ~B_NAVIGABLE);
		}

		flags = flags & ~B_NAVIGABLE;	// never want NAV for the combo box
	}
	BControl::SetFlags(flags);
}


void
BComboBox::SetEnabled(bool enabled)
{
	if (enabled == IsEnabled())
		return;

	if (Window()) {
		fText->MakeEditable(enabled);
		rgb_color mc = HighColor();
		rgb_color base = ViewColor();

		mc = (enabled) ? mc : disable_color(base);
		BFont textFont;
		fText->GetFontAndColor(0, &textFont);
		fText->SetFontAndColor(&textFont, B_FONT_ALL, &mc);

		if (!enabled)
			base = tint_color(base, B_DISABLED_MARK_TINT);
		else
			base.red = base.green = base.blue = 255;

		fText->SetLowColor(base);
		fText->SetViewColor(base);

		fText->Invalidate();
		Window()->UpdateIfNeeded();
	}

	fSkipSetFlags = true;
	BControl::SetEnabled(enabled);
	fSkipSetFlags = false;

//+	// Want the sub_view to be the navigable one. We always want to be able
//+	// to navigate to that view, even if disabled since Copy still works.
//+	fText->SetFlags(fText->Flags() | B_NAVIGABLE);
//+	SetFlags(Flags() & ~B_NAVIGABLE);
}


//void BComboBox::AllAttached()
//{
//}


BComboBox::ComboBoxWindow*
BComboBox::CreatePopupWindow()
{
	ComboBoxWindow *win = new ComboBoxWindow(this);
	return win;
}


void
BComboBox::CommitValue()
{
	Invoke();
}


void
BComboBox::TryAutoComplete()
{
	int32 from, to;
	fText->GetSelection(&from, &to);
	if (fAutoComplete && from == to) {
		bool autoCompleted = false;
		const char *ptr = fText->Text();
		if (to > fTextEnd && from == fText->TextLength()) {
			const char *completion;
			// find the first matching choice and do auto-completion

			// Find last comma
			const char *end;
			for (end = fText->Text()+fText->TextLength()-1; end>ptr; end--) {
				if (*end == ',') {
					// Find end of whitespace
					for (end++; isspace(*end); end++) {}
					if (*end == 0)
						return;
					break;
				}
			}
			if (fChoiceList->GetMatch(end, 0, &fCompletionIndex, &completion) == B_OK) {
				fText->Insert(completion);
				fText->Select(to, to + strlen(completion));
				Select(fCompletionIndex);
				autoCompleted = true;
			} else
				fCompletionIndex = -1;
		}
		fTextEnd = to;

		if (!autoCompleted) {
			int32 sel = CurrentSelection();
			if (sel >= 0) {
				const char *selText = fChoiceList->ChoiceAt(sel);
				if (selText && !strcmp(ptr, selText)) {
					// don't Deselect() if the text input matches the selection
					return;
				}
			}
			fCompletionIndex = -1;
			Deselect();
		}
	}
}


// fPopupWindow must exist and already be locked & hidden when this function
// is called
void
BComboBox::ShowPopupWindow()
{
	// adjust position of the popup window
	fPopupWindow->DoPosition();
	fPopupWindow->ListView()->SetEventMask(B_POINTER_EVENTS, 0);
	fPopupWindow->Show();
	fPopupWindow->ListView()->MakeFocus(true);
}


// fPopupWindow must exist and already be locked & shown when this function
// is called
void
BComboBox::HidePopupWindow()
{
	fPopupWindow->Hide();
	fPopupWindow->ListView()->SetEventMask(0, 0);
	fButtonDepressed = false;
	Invalidate(fButtonRect);
}


void
BComboBox::MakeFocus(bool state)
{
	fText->MakeFocus(state);
	if (state)
		fText->SelectAll();
}


//	#pragma mark -


BComboBox::MovedMessageFilter::MovedMessageFilter(BHandler *target)
	: BMessageFilter(B_WINDOW_MOVED)
{
	fTarget = target;
}


filter_result
BComboBox::MovedMessageFilter::Filter(BMessage *message,BHandler **/*target*/)
{
	BMessage *dup = new BMessage(*message);
	dup->what = kWindowMovedMessage;
	if (fTarget->Looper())
		fTarget->Looper()->PostMessage(dup, fTarget);

	delete dup;
	return B_DISPATCH_MESSAGE;
}


//	#pragma mark -


BDefaultChoiceList::BDefaultChoiceList(BComboBox *owner)
{
	fOwner = owner;
	fList = new StringObjectList();
}


BDefaultChoiceList::~BDefaultChoiceList()
{
	BString *string;
	while ((string = fList->RemoveItemAt(0)) != NULL) {
		delete string;
	}

	delete fList;
}


const char*
BDefaultChoiceList::ChoiceAt(int32 index)
{
	BString *string = fList->ItemAt(index);
	if (string)
		return string->String();

	return NULL;
}


status_t
BDefaultChoiceList::GetMatch(const char *prefix, int32 startIndex,
	int32 *matchIndex, const char **completionText)
{
	BString *str;
	int32 len = strlen(prefix);
	int32 choices = fList->CountItems();

	for (int32 i = startIndex; i < choices; i++) {
		str = fList->ItemAt(i);
		if (!str->ICompare(prefix, len)) {
			// prefix matches
			*matchIndex = i;
			*completionText = str->String() + len;
			return B_OK;
		}
	}
	*matchIndex = -1;
	*completionText = NULL;
	return B_ERROR;
}


int32
BDefaultChoiceList::CountChoices()
{
	return fList->CountItems();
}


status_t
BDefaultChoiceList::AddChoice(const char *toAdd)
{
	BString *str = new BString(toAdd);
	bool r = fList->AddItem(str);
	if (fOwner)
		fOwner->ChoiceListUpdated();

	return (r) ? B_OK : B_ERROR;
}


status_t
BDefaultChoiceList::AddChoiceAt(const char *toAdd, int32 index)
{
	BString *str = new BString(toAdd);
	bool r = fList->AddItem(str, index);
	if (fOwner)
		fOwner->ChoiceListUpdated();

	return r ? B_OK : B_ERROR;
}


//int BStringCompareFunction(const BString *s1, const BString *s2)
//{
//	return s1->Compare(s2);
//}


status_t
BDefaultChoiceList::RemoveChoice(const char *toRemove)
{
	BString *string;
	int32 choices = fList->CountItems();
	for (int32 i = 0; i < choices; i++) {
		string = fList->ItemAt(i);
		if (!string->Compare(toRemove)) {
			fList->RemoveItemAt(i);
			if (fOwner)
				fOwner->ChoiceListUpdated();

			return B_OK;
		}		
	}
	return B_ERROR;
}


status_t
BDefaultChoiceList::RemoveChoiceAt(int32 index)
{
	BString *string = fList->RemoveItemAt(index);
	if (string) {
		delete string;
		if (fOwner)
			fOwner->ChoiceListUpdated();

		return B_OK;
	}
	return B_ERROR;
}


void
BDefaultChoiceList::SetOwner(BComboBox *owner)
{
	fOwner = owner;
}


BComboBox*
BDefaultChoiceList::Owner()
{
	return fOwner;
}

