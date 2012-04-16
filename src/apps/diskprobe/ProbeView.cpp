/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ProbeView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <Directory.h>
#include <Entry.h>
#include <ExpressionParser.h>
#include <fs_attr.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageQueue.h>
#include <NodeInfo.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PrintJob.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Slider.h>
#include <String.h>
#include <TextControl.h>
#include <Volume.h>
#include <Window.h>

#include "DataView.h"
#include "DiskProbe.h"
#include "TypeEditors.h"


#ifndef __HAIKU__
#	define DRAW_SLIDER_BAR
	// if this is defined, the standard slider bar is replaced with
	// one that looks exactly like the one in the original DiskProbe
	// (even in Dano/Zeta)
#endif

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProbeView"

static const uint32 kMsgSliderUpdate = 'slup';
static const uint32 kMsgPositionUpdate = 'poup';
static const uint32 kMsgLastPosition = 'lpos';
static const uint32 kMsgFontSize = 'fnts';
static const uint32 kMsgBlockSize = 'blks';
static const uint32 kMsgAddBookmark = 'bmrk';
static const uint32 kMsgPrint = 'prnt';
static const uint32 kMsgPageSetup = 'pgsp';
static const uint32 kMsgViewAs = 'vwas';

static const uint32 kMsgStopFind = 'sfnd';


class IconView : public BView {
	public:
		IconView(BRect frame, const entry_ref *ref, bool isDevice);
		virtual ~IconView();

		virtual void AttachedToWindow();
		virtual void Draw(BRect updateRect);

		void UpdateIcon();

	private:
		entry_ref	fRef;
		bool		fIsDevice;
		BBitmap		*fBitmap;
};


class PositionSlider : public BSlider {
	public:
		PositionSlider(BRect rect, const char *name, BMessage *message,
			off_t size, uint32 blockSize);
		virtual ~PositionSlider();

#ifdef DRAW_SLIDER_BAR
		virtual void DrawBar();
#endif

		off_t Position() const;
		off_t Size() const { return fSize; }
		uint32 BlockSize() const { return fBlockSize; }

		virtual void SetPosition(float position);
		void SetPosition(off_t position);
		void SetSize(off_t size);
		void SetBlockSize(uint32 blockSize);

	private:
		void Reset();

		static const int32 kMaxSliderLimit = 0x7fffff80;
			// this is the maximum value that BSlider seem to work with fine

		off_t	fSize;
		uint32	fBlockSize;
};


class HeaderView : public BView, public BInvoker {
	public:
		HeaderView(BRect frame, const entry_ref *ref, DataEditor &editor);
		virtual ~HeaderView();

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void Draw(BRect updateRect);
		virtual void GetPreferredSize(float *_width, float *_height);
		virtual void MessageReceived(BMessage *message);

		base_type Base() const { return fBase; }
		void SetBase(base_type);

		off_t CursorOffset() const { return fOffset; }
		off_t Position() const { return fPosition; }
		uint32 BlockSize() const { return fBlockSize; }
		void SetTo(off_t position, uint32 blockSize);

		void UpdateIcon();

	private:
		void FormatValue(char *buffer, size_t bufferSize, off_t value);
		void UpdatePositionViews(bool all = true);
		void UpdateOffsetViews(bool all = true);
		void UpdateFileSizeView();
		void NotifyTarget();

		const char		*fAttribute;
		off_t			fFileSize;
		uint32			fBlockSize;
		off_t			fOffset;
		base_type		fBase;
		off_t			fPosition;
		off_t			fLastPosition;

		BTextControl	*fTypeControl;
		BTextControl	*fPositionControl;
		BStringView		*fPathView;
		BStringView		*fSizeView;
		BStringView		*fOffsetView;
		BStringView		*fFileOffsetView;
		PositionSlider	*fPositionSlider;
		IconView		*fIconView;
		BButton			*fStopButton;
};


class TypeMenuItem : public BMenuItem {
	public:
		TypeMenuItem(const char *name, const char *type, BMessage *message);

		virtual void GetContentSize(float *_width, float *_height);
		virtual void DrawContent();

	private:
		BString fType;
};


class EditorLooper : public BLooper {
	public:
		EditorLooper(const char *name, DataEditor &editor, BMessenger messenger);
		virtual ~EditorLooper();

		virtual void MessageReceived(BMessage *message);

		bool FindIsRunning() const { return !fQuitFind; }
		void Find(off_t startAt, const uint8 *data, size_t dataSize,
				bool caseInsensitive, BMessenger progressMonitor);
		void QuitFind();

	private:
		DataEditor		&fEditor;
		BMessenger		fMessenger;
		volatile bool	fQuitFind;
};


class TypeView : public BView {
	public:
		TypeView(BRect rect, const char* name, int32 index,
			DataEditor& editor, int32 resizingMode);
		virtual ~TypeView();

		virtual void FrameResized(float width, float height);

	private:
		BView*	fTypeEditorView;
};


//	#pragma mark - utility functions


static void
get_type_string(char *buffer, size_t bufferSize, type_code type)
{
	for (int32 i = 0; i < 4; i++) {
		buffer[i] = type >> (24 - 8 * i);
		if (buffer[i] < ' ' || buffer[i] == 0x7f) {
			snprintf(buffer, bufferSize, "0x%04lx", type);
			break;
		} else if (i == 3)
			buffer[4] = '\0';
	}
}


//	#pragma mark - IconView


IconView::IconView(BRect rect, const entry_ref *ref, bool isDevice)
	: BView(rect, NULL, B_FOLLOW_NONE, B_WILL_DRAW),
	fRef(*ref),
	fIsDevice(isDevice),
	fBitmap(NULL)
{
	UpdateIcon();
}


IconView::~IconView()
{
	delete fBitmap;
}


void
IconView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
IconView::Draw(BRect updateRect)
{
	if (fBitmap == NULL)
		return;

	SetDrawingMode(B_OP_ALPHA);
	DrawBitmap(fBitmap, updateRect, updateRect);
	SetDrawingMode(B_OP_COPY);
}


void
IconView::UpdateIcon()
{
	if (fBitmap == NULL)
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		fBitmap = new BBitmap(BRect(0, 0, 31, 31), B_RGBA32);
#else
		fBitmap = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
#endif

	if (fBitmap != NULL) {
		status_t status = B_ERROR;

		if (fIsDevice) {
			BPath path(&fRef);
#ifdef __HAIKU__
			status = get_device_icon(path.Path(), fBitmap, B_LARGE_ICON);
#else
			status = get_device_icon(path.Path(), fBitmap->Bits(), B_LARGE_ICON);
#endif
		} else
			status = BNodeInfo::GetTrackerIcon(&fRef, fBitmap);

		if (status != B_OK) {
			// Try to get generic icon
			BMimeType type(B_FILE_MIME_TYPE);
			status = type.GetIcon(fBitmap, B_LARGE_ICON);
		}

		if (status != B_OK) {
			delete fBitmap;
			fBitmap = NULL;
		}

		Invalidate();
	}
}


//	#pragma mark - PositionSlider


PositionSlider::PositionSlider(BRect rect, const char *name, BMessage *message,
	off_t size, uint32 blockSize)
	: BSlider(rect, name, NULL, message, 0, kMaxSliderLimit, B_HORIZONTAL,
		B_TRIANGLE_THUMB, B_FOLLOW_LEFT_RIGHT),
	fSize(size),
	fBlockSize(blockSize)
{
	Reset();

#ifndef DRAW_SLIDER_BAR
	rgb_color color = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	UseFillColor(true, &color);
#endif
}


PositionSlider::~PositionSlider()
{
}


#ifdef DRAW_SLIDER_BAR
void
PositionSlider::DrawBar()
{
	BView *view = OffscreenView();

	BRect barFrame = BarFrame();
	BRect frame = barFrame.InsetByCopy(1, 1);
	frame.top++;
	frame.left++;
	frame.right = ThumbFrame().left + ThumbFrame().Width() / 2;
#ifdef HAIKU_TARGET_PLATFORM_BEOS
	if (IsEnabled())
		view->SetHighColor(102, 152, 203);
	else
		view->SetHighColor(92, 102, 160);
#else
	view->SetHighColor(IsEnabled() ? ui_color(B_CONTROL_HIGHLIGHT_COLOR)
		: tint_color(ui_color(B_CONTROL_HIGHLIGHT_COLOR), B_DARKEN_1_TINT));
#endif
	view->FillRect(frame);

	frame.left = frame.right + 1;
	frame.right = barFrame.right - 1;
	view->SetHighColor(tint_color(ViewColor(), IsEnabled() ? B_DARKEN_1_TINT : B_LIGHTEN_1_TINT));
	view->FillRect(frame);

	rgb_color cornerColor = tint_color(ViewColor(), B_DARKEN_1_TINT);
	rgb_color darkColor = tint_color(ViewColor(), B_DARKEN_3_TINT);
#ifdef HAIKU_TARGET_PLATFORM_BEOS
	rgb_color shineColor = {255, 255, 255};
	rgb_color shadowColor = {0, 0, 0};
#else
	rgb_color shineColor = ui_color(B_SHINE_COLOR);
	rgb_color shadowColor = ui_color(B_SHADOW_COLOR);
#endif
	if (!IsEnabled()) {
		darkColor = tint_color(ViewColor(), B_DARKEN_1_TINT);
		shineColor = tint_color(shineColor, B_DARKEN_2_TINT);
		shadowColor = tint_color(shadowColor, B_LIGHTEN_2_TINT);
	}

	view->BeginLineArray(9);

	// the corners
	view->AddLine(barFrame.LeftTop(), barFrame.LeftTop(), cornerColor);
	view->AddLine(barFrame.LeftBottom(), barFrame.LeftBottom(), cornerColor);
	view->AddLine(barFrame.RightTop(), barFrame.RightTop(), cornerColor);

	// the edges
	view->AddLine(BPoint(barFrame.left, barFrame.top + 1),
		BPoint(barFrame.left, barFrame.bottom - 1), darkColor);
	view->AddLine(BPoint(barFrame.right, barFrame.top + 1),
		BPoint(barFrame.right, barFrame.bottom), shineColor);

	barFrame.left++;
	barFrame.right--;
	view->AddLine(barFrame.LeftTop(), barFrame.RightTop(), darkColor);
	view->AddLine(barFrame.LeftBottom(), barFrame.RightBottom(), shineColor);

	// the inner edges
	barFrame.top++;
	view->AddLine(barFrame.LeftTop(), barFrame.RightTop(), shadowColor);
	view->AddLine(BPoint(barFrame.left, barFrame.top + 1),
		BPoint(barFrame.left, barFrame.bottom - 1), shadowColor);

	view->EndLineArray();
}
#endif	// DRAW_SLIDER_BAR


void
PositionSlider::Reset()
{
	SetKeyIncrementValue(int32(1.0 * kMaxSliderLimit / ((fSize - 1) / fBlockSize) + 0.5));
	SetEnabled(fSize > fBlockSize);
}


off_t
PositionSlider::Position() const
{
	// ToDo:
	// Note: this code is far from being perfect: depending on the file size, it has
	//	a maxium granularity that might be less than the actual block size demands...
	//	The only way to work around this that I can think of, is to replace the slider
	//	class completely with one that understands off_t values.
	//	For example, with a block size of 512 bytes, it should be good enough for about
	//	1024 GB - and that's not really that far away these days.

	return (off_t(1.0 * (fSize - 1) * Value() / kMaxSliderLimit + 0.5) / fBlockSize) * fBlockSize;
}


void
PositionSlider::SetPosition(float position)
{
	BSlider::SetPosition(position);
}


void
PositionSlider::SetPosition(off_t position)
{
	position /= fBlockSize;
	SetValue(int32(1.0 * kMaxSliderLimit * position / ((fSize - 1) / fBlockSize) + 0.5));
}


void
PositionSlider::SetSize(off_t size)
{
	if (size == fSize)
		return;

	off_t position = Position();
	if (position >= size)
		position = size - 1;

	fSize = size;
	Reset();
	SetPosition(position);
}


void
PositionSlider::SetBlockSize(uint32 blockSize)
{
	if (blockSize == fBlockSize)
		return;

	off_t position = Position();
	fBlockSize = blockSize;
	Reset();
	SetPosition(position);
}


//	#pragma mark - HeaderView


HeaderView::HeaderView(BRect frame, const entry_ref *ref, DataEditor &editor)
	: BView(frame, "probeHeader", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW),
	fAttribute(editor.Attribute()),
	fFileSize(editor.FileSize()),
	fBlockSize(editor.BlockSize()),
	fOffset(0),
	fBase(kHexBase),
	fPosition(0),
	fLastPosition(0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fIconView = new IconView(BRect(10, 10, 41, 41), ref, editor.IsDevice());
	AddChild(fIconView);

	BFont boldFont = *be_bold_font;
	boldFont.SetSize(10.0);
	BFont plainFont = *be_plain_font;
	plainFont.SetSize(10.0);

	BRect rect = Bounds();
	fStopButton = new BButton(BRect(0, 0, 20, 20), B_EMPTY_STRING, 
		B_TRANSLATE("Stop"), new BMessage(kMsgStopFind), 
		B_FOLLOW_TOP | B_FOLLOW_RIGHT);
	fStopButton->SetFont(&plainFont);
	fStopButton->ResizeToPreferred();
	fStopButton->MoveTo(rect.right - 4 - fStopButton->Bounds().Width(), 4);
	fStopButton->Hide();
	AddChild(fStopButton);

	BStringView *stringView = new BStringView(BRect(50, 6, rect.right, 20),
		B_EMPTY_STRING, editor.IsAttribute()
		? B_TRANSLATE("Attribute: ") 
		: editor.IsDevice() 
		? B_TRANSLATE("Device: ") 
		: B_TRANSLATE("File: "));
	stringView->SetFont(&boldFont);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	BPath path(ref);
	BString string = path.Path();
	if (fAttribute != NULL) {
		string.Prepend(" (");
		string.Prepend(fAttribute);
		string.Append(")");
	}
	rect = stringView->Frame();
	rect.left = rect.right;
	rect.right = fStopButton->Frame().right - 1;
	fPathView = new BStringView(rect, B_EMPTY_STRING, string.String(),
		B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
	fPathView->SetFont(&plainFont);
	AddChild(fPathView);

	float top = 28;
	if (editor.IsAttribute()) {
		top += 3;
		stringView = new BStringView(BRect(50, top, frame.right, top + 15),
			B_EMPTY_STRING, B_TRANSLATE("Attribute type: "));
		stringView->SetFont(&boldFont);
		stringView->ResizeToPreferred();
		AddChild(stringView);

		rect = stringView->Frame();
		rect.left = rect.right;
		rect.right += 100;
		rect.OffsetBy(0, -2);
			// BTextControl oddities

		char buffer[16];
		get_type_string(buffer, sizeof(buffer), editor.Type());
		fTypeControl = new BTextControl(rect, B_EMPTY_STRING, NULL, buffer,
			new BMessage(kMsgPositionUpdate));
		fTypeControl->SetDivider(0.0);
		fTypeControl->SetFont(&plainFont);
		fTypeControl->TextView()->SetFontAndColor(&plainFont);
		fTypeControl->SetEnabled(false);
			// ToDo: for now
		AddChild(fTypeControl);

		top += 25;
	} else
		fTypeControl = NULL;

	stringView = new BStringView(BRect(50, top, frame.right, top + 15),
		B_EMPTY_STRING, B_TRANSLATE("Block: "));
	stringView->SetFont(&boldFont);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect = stringView->Frame();
	rect.left = rect.right;
	rect.right += 75;
	rect.OffsetBy(0, -2);
		// BTextControl oddities
	fPositionControl = new BTextControl(rect, B_EMPTY_STRING, NULL, "0x0",
		new BMessage(kMsgPositionUpdate));
	fPositionControl->SetDivider(0.0);
	fPositionControl->SetFont(&plainFont);
	fPositionControl->TextView()->SetFontAndColor(&plainFont);
	fPositionControl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	AddChild(fPositionControl);

	rect.left = rect.right + 4;
	rect.right = rect.left + 75;
	rect.OffsetBy(0, 2);
	fSizeView = new BStringView(rect, B_EMPTY_STRING, B_TRANSLATE_COMMENT("of "
		"0x0", "This is a part of 'Block 0xXXXX of 0x0026' message. In "
		"languages without 'of' structure it can be replaced simply "
		"with '/'."));
	fSizeView->SetFont(&plainFont);
	AddChild(fSizeView);
	UpdateFileSizeView();

	rect.left = rect.right + 4;
	rect.right = frame.right;
	stringView = new BStringView(rect, B_EMPTY_STRING, B_TRANSLATE("Offset: "));
	stringView->SetFont(&boldFont);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect = stringView->Frame();
	rect.left = rect.right;
	rect.right = rect.left + 40;
	fOffsetView = new BStringView(rect, B_EMPTY_STRING, "0x0");
	fOffsetView->SetFont(&plainFont);
	AddChild(fOffsetView);
	UpdateOffsetViews(false);

	rect.left = rect.right + 4;
	rect.right = frame.right;
	stringView = new BStringView(rect, B_EMPTY_STRING, editor.IsAttribute()
		? B_TRANSLATE("Attribute offset: ") : editor.IsDevice()
			? B_TRANSLATE("Device offset: ") : B_TRANSLATE("File offset: "));
	stringView->SetFont(&boldFont);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect = stringView->Frame();
	rect.left = rect.right;
	rect.right = rect.left + 70;
	fFileOffsetView = new BStringView(rect, B_EMPTY_STRING, "0x0");
	fFileOffsetView->SetFont(&plainFont);
	AddChild(fFileOffsetView);

	rect = Bounds();
	rect.InsetBy(3, 0);
	rect.top = top + 21;
	rect.bottom = rect.top + 12;
	fPositionSlider = new PositionSlider(rect, "slider",
		new BMessage(kMsgSliderUpdate), editor.FileSize(), editor.BlockSize());
	fPositionSlider->SetModificationMessage(new BMessage(kMsgSliderUpdate));
	fPositionSlider->SetBarThickness(8);
	fPositionSlider->ResizeToPreferred();
	AddChild(fPositionSlider);
}


HeaderView::~HeaderView()
{
}


void
HeaderView::AttachedToWindow()
{
	SetTarget(Window());

	fStopButton->SetTarget(Parent());
	fPositionControl->SetTarget(this);
	fPositionSlider->SetTarget(this);

	BMessage *message;
	Window()->AddShortcut(B_HOME, B_COMMAND_KEY,
		message = new BMessage(kMsgPositionUpdate), this);
	message->AddInt64("block", 0);
	Window()->AddShortcut(B_END, B_COMMAND_KEY,
		message = new BMessage(kMsgPositionUpdate), this);
	message->AddInt64("block", -1);
	Window()->AddShortcut(B_PAGE_UP, B_COMMAND_KEY,
		message = new BMessage(kMsgPositionUpdate), this);
	message->AddInt32("delta", -1);
	Window()->AddShortcut(B_PAGE_DOWN, B_COMMAND_KEY,
		message = new BMessage(kMsgPositionUpdate), this);
	message->AddInt32("delta", 1);
}


void
HeaderView::DetachedFromWindow()
{
	Window()->RemoveShortcut(B_HOME, B_COMMAND_KEY);
	Window()->RemoveShortcut(B_END, B_COMMAND_KEY);
	Window()->RemoveShortcut(B_PAGE_UP, B_COMMAND_KEY);
	Window()->RemoveShortcut(B_PAGE_DOWN, B_COMMAND_KEY);
}


void
HeaderView::Draw(BRect updateRect)
{
	BRect rect = Bounds();

#ifdef HAIKU_TARGET_PLATFORM_BEOS
	SetHighColor(255, 255, 255);
#else
	SetHighColor(ui_color(B_SHINE_COLOR));
#endif
	StrokeLine(rect.LeftTop(), rect.LeftBottom());
	StrokeLine(rect.LeftTop(), rect.RightTop());

	// the gradient at the bottom is drawn by the BScrollView
}


void
HeaderView::GetPreferredSize(float *_width, float *_height)
{
	if (_width)
		*_width = Bounds().Width();
	if (_height)
		*_height = fPositionSlider->Frame().bottom + 2;
}


void
HeaderView::UpdateIcon()
{
	fIconView->UpdateIcon();
}


void
HeaderView::FormatValue(char *buffer, size_t bufferSize, off_t value)
{
	snprintf(buffer, bufferSize, fBase == kHexBase ? "0x%Lx" : "%Ld", value);
}


void
HeaderView::UpdatePositionViews(bool all)
{
	char buffer[64];
	FormatValue(buffer, sizeof(buffer), fPosition / fBlockSize);
	fPositionControl->SetText(buffer);

	if (all) {
		FormatValue(buffer, sizeof(buffer), fPosition + fOffset);
		fFileOffsetView->SetText(buffer);
	}
}


void
HeaderView::UpdateOffsetViews(bool all)
{
	char buffer[64];
	FormatValue(buffer, sizeof(buffer), fOffset);
	fOffsetView->SetText(buffer);

	if (all) {
		FormatValue(buffer, sizeof(buffer), fPosition + fOffset);
		fFileOffsetView->SetText(buffer);
	}
}


void
HeaderView::UpdateFileSizeView()
{
	BString string(B_TRANSLATE("of "));
	char buffer[64];
	FormatValue(buffer, sizeof(buffer),
		(fFileSize + fBlockSize - 1) / fBlockSize);
	string << buffer;

	fSizeView->SetText(string.String());
}


void
HeaderView::SetBase(base_type type)
{
	if (fBase == type)
		return;

	fBase = type;

	UpdatePositionViews();
	UpdateOffsetViews(false);
	UpdateFileSizeView();
}


void
HeaderView::SetTo(off_t position, uint32 blockSize)
{
	fPosition = position;
	fLastPosition = (fLastPosition / fBlockSize) * blockSize;
	fBlockSize = blockSize;

	fPositionSlider->SetBlockSize(blockSize);
	UpdatePositionViews();
	UpdateOffsetViews(false);
	UpdateFileSizeView();
}


void
HeaderView::NotifyTarget()
{
	BMessage update(kMsgPositionUpdate);
	update.AddInt64("position", fPosition);
	Messenger().SendMessage(&update);
}


void
HeaderView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE: {
			int32 what;
			if (message->FindInt32(B_OBSERVE_WHAT_CHANGE, &what) != B_OK)
				break;

			switch (what) {
				case kDataViewCursorPosition:
					off_t offset;
					if (message->FindInt64("position", &offset) == B_OK) {
						fOffset = offset;
						UpdateOffsetViews();
					}
					break;
			}
			break;
		}

		case kMsgSliderUpdate:
		{
			// First, make sure we're only considering the most
			// up-to-date message in the queue (which might not
			// be this one).
			// If there is another message of this type in the
			// queue, we're just ignoring the current message.

			if (Looper()->MessageQueue()->FindMessage(kMsgSliderUpdate, 0)
					!= NULL)
				break;

			// if nothing has changed, we can ignore this message as well
			if (fPosition == fPositionSlider->Position())
				break;

			fLastPosition = fPosition;
			fPosition = fPositionSlider->Position();

			// update position text control
			UpdatePositionViews();

			// notify our target
			NotifyTarget();
			break;
		}

		case kMsgDataEditorFindProgress:
		{
			bool state;
			if (message->FindBool("running", &state) == B_OK
				&& fFileSize > fBlockSize) {
				fPositionSlider->SetEnabled(!state);
				if (state) {
					fPathView->ResizeBy(-fStopButton->Bounds().Width(), 0);
					fStopButton->Show();
				} else {
					fStopButton->Hide();
					fPathView->ResizeBy(fStopButton->Bounds().Width(), 0);
				}
			}

			off_t position;
			if (message->FindInt64("position", &position) != B_OK)
				break;

			fPosition = (position / fBlockSize) * fBlockSize;
				// round to block size

			// update views
			UpdatePositionViews(false);
			fPositionSlider->SetPosition(fPosition);
			break;
		}

		case kMsgPositionUpdate:
		{
			off_t lastPosition = fPosition;

			off_t position;
			int32 delta;
			if (message->FindInt64("position", &position) == B_OK)
				fPosition = position;
			else if (message->FindInt64("block", &position) == B_OK) {
				if (position < 0)
					position += (fFileSize - 1) / fBlockSize + 1;
				fPosition = position * fBlockSize;
			} else if (message->FindInt32("delta", &delta) == B_OK) {
				fPosition += delta * off_t(fBlockSize);
			} else {
				try {
					ExpressionParser parser;
					parser.SetSupportHexInput(true);
					fPosition = parser.EvaluateToInt64(
						fPositionControl->Text()) * fBlockSize;
				} catch (...) {
					beep();
					break;
				}
			}

			fLastPosition = lastPosition;
			fPosition = (fPosition / fBlockSize) * fBlockSize;
				// round to block size

			if (fPosition < 0)
				fPosition = 0;
			else if (fPosition > ((fFileSize - 1) / fBlockSize) * fBlockSize)
				fPosition = ((fFileSize - 1) / fBlockSize) * fBlockSize;

			// update views
			UpdatePositionViews();
			fPositionSlider->SetPosition(fPosition);

			// notify our target
			NotifyTarget();
			break;
		}

		case kMsgLastPosition:
		{
			fPosition = fLastPosition;
			fLastPosition = fPositionSlider->Position();

			// update views
			UpdatePositionViews();
			fPositionSlider->SetPosition(fPosition);

			// notify our target
			NotifyTarget();
			break;
		}

		case kMsgBaseType:
		{
			int32 type;
			if (message->FindInt32("base", &type) != B_OK)
				break;

			SetBase((base_type)type);
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


//	#pragma mark - TypeMenuItem


/*!	The TypeMenuItem is a BMenuItem that displays a type string at its
	right border.
	It is used to display the attribute and type in the attributes menu.
	It does not mix nicely with short cuts.
*/
TypeMenuItem::TypeMenuItem(const char *name, const char *type,
		BMessage *message)
	: BMenuItem(name, message),
	fType(type)
{
}


void
TypeMenuItem::GetContentSize(float *_width, float *_height)
{
	BMenuItem::GetContentSize(_width, _height);

	if (_width)
		*_width += Menu()->StringWidth(fType.String());
}


void
TypeMenuItem::DrawContent()
{
	// draw the label
	BMenuItem::DrawContent();

	font_height fontHeight;
	Menu()->GetFontHeight(&fontHeight);

	// draw the type
	BPoint point = ContentLocation();
	point.x = Frame().right - 4 - Menu()->StringWidth(fType.String());
	point.y += fontHeight.ascent;

#ifdef HAIKU_TARGET_PLATFORM_BEOS
	Menu()->SetDrawingMode(B_OP_ALPHA);
#endif

	Menu()->DrawString(fType.String(), point);
}


//	#pragma mark - EditorLooper


/*!	The purpose of this looper is to off-load the editor data loading from
	the main window looper.

	It will listen to the offset changes of the editor, let him update its
	data, and will then synchronously notify the target.
	That way, simple offset changes will not stop the main looper from
	operating. Therefore, all offset updates for the editor will go through
	this looper.
	Also, it will run the find action in the editor.
*/
EditorLooper::EditorLooper(const char *name, DataEditor &editor,
		BMessenger target)
	: BLooper(name),
	fEditor(editor),
	fMessenger(target),
	fQuitFind(true)
{
	fEditor.StartWatching(this);
}


EditorLooper::~EditorLooper()
{
	fEditor.StopWatching(this);
}


void
EditorLooper::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgPositionUpdate:
		{
			// First, make sure we're only considering the most
			// up-to-date message in the queue (which might not
			// be this one).
			// If there is another message of this type in the
			// queue, we're just ignoring the current message.

			if (Looper()->MessageQueue()->FindMessage(kMsgPositionUpdate, 0) != NULL)
				break;

			off_t position;
			if (message->FindInt64("position", &position) == B_OK) {
				BAutolock locker(fEditor);
				fEditor.SetViewOffset(position);
			}
			break;
		}

		case kMsgDataEditorParameterChange:
		{
			bool updated = false;

			if (fEditor.Lock()) {
				fEditor.UpdateIfNeeded(&updated);
				fEditor.Unlock();
			}

			if (updated) {
				BMessage reply;
				fMessenger.SendMessage(kMsgUpdateData, &reply);
					// We are doing a synchronously transfer, to prevent
					// that we're already locking the editor again when
					// our target wants to get the editor data.
			}
			break;
		}

		case kMsgFind:
		{
			BMessenger progressMonitor;
			message->FindMessenger("progress_monitor", &progressMonitor);

			off_t startAt = 0;
			message->FindInt64("start", &startAt);

			bool caseInsensitive = !message->FindBool("case_sensitive");

			ssize_t dataSize;
			const uint8 *data;
			if (message->FindData("data", B_RAW_TYPE, (const void **)&data,
					&dataSize) == B_OK)
				Find(startAt, data, dataSize, caseInsensitive, progressMonitor);
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


void
EditorLooper::Find(off_t startAt, const uint8 *data, size_t dataSize,
	bool caseInsensitive, BMessenger progressMonitor)
{
	fQuitFind = false;

	BAutolock locker(fEditor);

	bigtime_t startTime = system_time();

	off_t foundAt = fEditor.Find(startAt, data, dataSize, caseInsensitive,
						true, progressMonitor, &fQuitFind);
	if (foundAt >= B_OK) {
		fEditor.SetViewOffset(foundAt);

		// select the part in our target
		BMessage message(kMsgSetSelection);
		message.AddInt64("start", foundAt - fEditor.ViewOffset());
		message.AddInt64("end", foundAt + dataSize - 1 - fEditor.ViewOffset());
		fMessenger.SendMessage(&message);
	} else if (foundAt == B_ENTRY_NOT_FOUND) {
		if (system_time() > startTime + 8000000LL) {
			// If the user had to wait more than 8 seconds for the result,
			// we are trying to please him with a requester...
			(new BAlert(B_TRANSLATE("DiskProbe request"),
				B_TRANSLATE("Could not find search string."), 
				B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, 
				B_WARNING_ALERT))->Go(NULL);
		} else
			beep();
	}
}


void
EditorLooper::QuitFind()
{
	fQuitFind = true;
		// this will cleanly stop the find process
}


//	#pragma mark - TypeView


TypeView::TypeView(BRect rect, const char* name, int32 index,
		DataEditor& editor, int32 resizingMode)
	: BView(rect, name, resizingMode, B_FRAME_EVENTS)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTypeEditorView = GetTypeEditorAt(index, Frame(), editor);
	if (fTypeEditorView == NULL) {
		AddChild(new BStringView(Bounds(), B_TRANSLATE("Type editor"),
			B_TRANSLATE("Type editor not supported"), B_FOLLOW_NONE));
	} else
		AddChild(fTypeEditorView);

	if ((fTypeEditorView->ResizingMode() & (B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM))
			!= 0) {
		BRect rect = Bounds();

		BRect frame = fTypeEditorView->Frame();
		rect.left = frame.left;
		rect.top = frame.top;
		if ((fTypeEditorView->ResizingMode() & B_FOLLOW_RIGHT) == 0)
			rect.right = frame.right;
		if ((fTypeEditorView->ResizingMode() & B_FOLLOW_BOTTOM) == 0)
			rect.bottom = frame.bottom;

		fTypeEditorView->ResizeTo(rect.Width(), rect.Height());
	}
}


TypeView::~TypeView()
{
}


void
TypeView::FrameResized(float width, float height)
{
	BRect rect = Bounds();

	BPoint point = fTypeEditorView->Frame().LeftTop();
	if ((fTypeEditorView->ResizingMode() & B_FOLLOW_RIGHT) == 0)
		point.x = (rect.Width() - fTypeEditorView->Bounds().Width()) / 2;
	if ((fTypeEditorView->ResizingMode() & B_FOLLOW_BOTTOM) == 0)
		point.y = (rect.Height() - fTypeEditorView->Bounds().Height()) / 2;

	fTypeEditorView->MoveTo(point);
}


//	#pragma mark - ProbeView


ProbeView::ProbeView(BRect rect, entry_ref *ref, const char *attribute,
		const BMessage *settings)
	: BView(rect, "probeView", B_FOLLOW_ALL, B_WILL_DRAW),
	fPrintSettings(NULL),
	fTypeView(NULL),
	fLastSearch(NULL)
{
	fEditor.SetTo(*ref, attribute);

	int32 baseType = kHexBase;
	float fontSize = 12.0f;
	if (settings != NULL) {
		settings->FindInt32("base_type", &baseType);
		settings->FindFloat("font_size", &fontSize);
	}

	rect = Bounds();
	fHeaderView = new HeaderView(rect, &fEditor.Ref(), fEditor);
	fHeaderView->ResizeToPreferred();
	fHeaderView->SetBase((base_type)baseType);
	AddChild(fHeaderView);

	rect = fHeaderView->Frame();
	rect.top = rect.bottom + 3;
	rect.bottom = Bounds().bottom - B_H_SCROLL_BAR_HEIGHT;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	fDataView = new DataView(rect, fEditor);
	fDataView->SetBase((base_type)baseType);
	fDataView->SetFontSize(fontSize);

	fScrollView = new BScrollView("scroller", fDataView, B_FOLLOW_ALL,
		B_WILL_DRAW, true, true);
	AddChild(fScrollView);

	fDataView->UpdateScroller();
}


ProbeView::~ProbeView()
{
}


void
ProbeView::UpdateSizeLimits()
{
	if (Window() == NULL)
		return;

	if (!fDataView->FontSizeFitsBounds()) {
		float width, height;
		fDataView->GetPreferredSize(&width, &height);

		BRect frame = Window()->ConvertFromScreen(ConvertToScreen(
			fHeaderView->Frame()));

		Window()->SetSizeLimits(250, width + B_V_SCROLL_BAR_WIDTH,
			200, height + frame.bottom + 4 + B_H_SCROLL_BAR_HEIGHT);
	} else
		Window()->SetSizeLimits(250, 32768, 200, 32768);

#ifdef HAIKU_TARGET_PLATFORM_BEOS
	// In Haiku and Dano, the window is resized automatically
	BRect bounds = Window()->Bounds();
	float minWidth, maxWidth, minHeight, maxHeight;
	Window()->GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	if (maxWidth < bounds.Width() || maxHeight < bounds.Height()) {
		Window()->ResizeTo(MIN(maxWidth, bounds.Width()), MIN(maxHeight,
			bounds.Height()));
	}
#endif
}


void
ProbeView::DetachedFromWindow()
{
	fEditorLooper->QuitFind();

	if (fEditorLooper->Lock())
		fEditorLooper->Quit();
	fEditorLooper = NULL;

	fEditor.StopWatching(this);
	fDataView->StopWatching(fHeaderView, kDataViewCursorPosition);
	fDataView->StopWatching(this, kDataViewSelection);
	fDataView->StopWatching(this, kDataViewPreferredSize);
	be_clipboard->StopWatching(this);
}


void
ProbeView::_UpdateAttributesMenu(BMenu *menu)
{
	// remove old contents

	for (int32 i = menu->CountItems(); i-- > 0;) {
		delete menu->RemoveItem(i);
	}

	// add new items (sorted)

	BNode node(&fEditor.AttributeRef());
	if (node.InitCheck() == B_OK) {
		char attribute[B_ATTR_NAME_LENGTH];
		node.RewindAttrs();

		while (node.GetNextAttrName(attribute) == B_OK) {
			attr_info info;
			if (node.GetAttrInfo(attribute, &info) != B_OK)
				continue;

			char type[16];
			type[0] = '[';
			get_type_string(type + 1, sizeof(type) - 2, info.type);
			strcat(type, "]");

			// find where to insert
			int32 i;
			for (i = 0; i < menu->CountItems(); i++) {
				if (strcasecmp(menu->ItemAt(i)->Label(), attribute) > 0)
					break;
			}

			BMessage *message = new BMessage(B_REFS_RECEIVED);
			message->AddRef("refs", &fEditor.AttributeRef());
			message->AddString("attributes", attribute);

			menu->AddItem(new TypeMenuItem(attribute, type, message), i);
		}
	}

	if (menu->CountItems() == 0) {
		// if there are no attributes, add an item to the menu
		// that says so
		BMenuItem *item = new BMenuItem(B_TRANSLATE_COMMENT("none", 
			"No attributes"), NULL);
		item->SetEnabled(false);
		menu->AddItem(item);
	}

	menu->SetTargetForItems(be_app);
}


void
ProbeView::AddSaveMenuItems(BMenu* menu, int32 index)
{
	menu->AddItem(fSaveMenuItem = new BMenuItem(B_TRANSLATE("Save"),
		new BMessage(B_SAVE_REQUESTED), 'S'), index);
	fSaveMenuItem->SetTarget(this);
	fSaveMenuItem->SetEnabled(false);
	//menu->AddItem(new BMenuItem("Save As" B_UTF8_ELLIPSIS, NULL), index);
}


void
ProbeView::AddPrintMenuItems(BMenu* menu, int32 index)
{
	BMenuItem *item;
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Page setup" B_UTF8_ELLIPSIS),
		new BMessage(kMsgPageSetup)), index++);
	item->SetTarget(this);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Print" B_UTF8_ELLIPSIS),
		new BMessage(kMsgPrint), 'P'), index++);
	item->SetTarget(this);
}


void
ProbeView::AddViewAsMenuItems()
{
#if 0
	BMenuBar* bar = Window()->KeyMenuBar();
	if (bar == NULL)
		return;

	BMenuItem* item = bar->FindItem(B_TRANSLATE("View"));
	BMenu* menu = NULL;
	if (item != NULL)
		menu = item->Submenu();
	else
		menu = bar->SubmenuAt(bar->CountItems() - 1);

	if (menu == NULL)
		return;

	menu->AddSeparatorItem();

	BMenu* subMenu = new BMenu(B_TRANSLATE("View As"));
	subMenu->SetRadioMode(true);

	BMessage* message = new BMessage(kMsgViewAs);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Raw"), message));
	item->SetMarked(true);

	const char* name;
	for (int32 i = 0; GetNthTypeEditor(i, &name) == B_OK; i++) {
		message = new BMessage(kMsgViewAs);
		message->AddInt32("editor index", i);
		subMenu->AddItem(new BMenuItem(name, message));
	}

	subMenu->SetTargetForItems(this);
	menu->AddItem(new BMenuItem(subMenu));
#endif
}


void
ProbeView::AttachedToWindow()
{
	fEditorLooper = new EditorLooper(fEditor.Ref().name, fEditor,
		BMessenger(fDataView));
	fEditorLooper->Run();

	fEditor.StartWatching(this);
	fDataView->StartWatching(fHeaderView, kDataViewCursorPosition);
	fDataView->StartWatching(this, kDataViewSelection);
	fDataView->StartWatching(this, kDataViewPreferredSize);
	be_clipboard->StartWatching(this);

	// Add menu to window

	BMenuBar *bar = Window()->KeyMenuBar();
	if (bar == NULL) {
		// there is none? Well, but we really want to have one
		bar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
		Window()->AddChild(bar);

		MoveBy(0, bar->Bounds().Height());
		ResizeBy(0, -bar->Bounds().Height());

		BMenu *menu = new BMenu(fEditor.IsAttribute()
			? B_TRANSLATE("Attribute") : fEditor.IsDevice() ? B_TRANSLATE("Device") : B_TRANSLATE("File"));
		AddSaveMenuItems(menu, 0);
		menu->AddSeparatorItem();
		AddPrintMenuItems(menu, menu->CountItems());
		menu->AddSeparatorItem();

		menu->AddItem(new BMenuItem(B_TRANSLATE("Close"), new BMessage(B_CLOSE_REQUESTED),
			'W'));
		bar->AddItem(menu);
	}

	// "Edit" menu

	BMenu *menu = new BMenu(B_TRANSLATE("Edit"));
	BMenuItem *item;
	menu->AddItem(fUndoMenuItem = new BMenuItem(B_TRANSLATE("Undo"), new BMessage(B_UNDO),
		'Z'));
	fUndoMenuItem->SetEnabled(fEditor.CanUndo());
	fUndoMenuItem->SetTarget(fDataView);
	menu->AddItem(fRedoMenuItem = new BMenuItem(B_TRANSLATE("Redo"), new BMessage(B_REDO),
		'Z', B_SHIFT_KEY));
	fRedoMenuItem->SetEnabled(fEditor.CanRedo());
	fRedoMenuItem->SetTarget(fDataView);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Copy"), new BMessage(B_COPY), 'C'));
	item->SetTarget(NULL, Window());
	menu->AddItem(fPasteMenuItem = new BMenuItem(B_TRANSLATE("Paste"), new BMessage(B_PASTE),
		'V'));
	fPasteMenuItem->SetTarget(NULL, Window());
	_CheckClipboard();
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Select all"), new BMessage(B_SELECT_ALL),
		'A'));
	item->SetTarget(NULL, Window());
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Find" B_UTF8_ELLIPSIS),
		new BMessage(kMsgOpenFindWindow), 'F'));
	item->SetTarget(this);
	menu->AddItem(fFindAgainMenuItem = new BMenuItem(B_TRANSLATE("Find again"),
		new BMessage(kMsgFind), 'G'));
	fFindAgainMenuItem->SetEnabled(false);
	fFindAgainMenuItem->SetTarget(this);
	bar->AddItem(menu);

	// "Block" menu

	menu = new BMenu(B_TRANSLATE("Block"));
	BMessage *message = new BMessage(kMsgPositionUpdate);
	message->AddInt32("delta", 1);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Next"), message, B_RIGHT_ARROW));
	item->SetTarget(fHeaderView);
	message = new BMessage(kMsgPositionUpdate);
	message->AddInt32("delta", -1);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Previous"), message, B_LEFT_ARROW));
	item->SetTarget(fHeaderView);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Back"), new BMessage(kMsgLastPosition),
		'J'));
	item->SetTarget(fHeaderView);

	BMenu *subMenu = new BMenu(B_TRANSLATE("Selection"));
	message = new BMessage(kMsgPositionUpdate);
	message->AddInt64("block", 0);
	subMenu->AddItem(fNativeMenuItem = new BMenuItem("", message, 'K'));
	fNativeMenuItem->SetTarget(fHeaderView);
	message = new BMessage(*message);
	subMenu->AddItem(fSwappedMenuItem = new BMenuItem("", message, 'L'));
	fSwappedMenuItem->SetTarget(fHeaderView);
	menu->AddItem(new BMenuItem(subMenu));
	_UpdateSelectionMenuItems(0, 0);
	menu->AddSeparatorItem();

	fBookmarkMenu = new BMenu(B_TRANSLATE("Bookmarks"));
	fBookmarkMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Add"),
		new BMessage(kMsgAddBookmark), 'B'));
	item->SetTarget(this);
	menu->AddItem(new BMenuItem(fBookmarkMenu));
	bar->AddItem(menu);

	// "Attributes" menu (it's only visible if the underlying
	// file system actually supports attributes)

	BDirectory directory;
	BVolume volume;
	if (directory.SetTo(&fEditor.AttributeRef()) == B_OK
		&& directory.IsRootDirectory())
		directory.GetVolume(&volume);
	else
		fEditor.File().GetVolume(&volume);

	if (!fEditor.IsAttribute() && volume.InitCheck() == B_OK
		&& (volume.KnowsMime() || volume.KnowsAttr())) {
		bar->AddItem(menu = new BMenu(B_TRANSLATE("Attributes")));
		_UpdateAttributesMenu(menu);
	}

	// "View" menu

	menu = new BMenu(B_TRANSLATE_COMMENT("View", 
		"This is the last menubar item 'File Edit Block View'"));

	// Number Base (hex/decimal)

	subMenu = new BMenu(B_TRANSLATE_COMMENT("Base", "A menu item, the number "
		"that is basis for a system of calculation. The base 10 system is a "
		"decimal system. This is in the same menu window than 'Font size' "
		"and 'BlockSize'"));
	message = new BMessage(kMsgBaseType);
	message->AddInt32("base_type", kDecimalBase);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE_COMMENT("Decimal", 
		"A menu item, as short as possible, noun is recommended if it is "
		"shorter than adjective."), message, 'D'));
	item->SetTarget(this);
	if (fHeaderView->Base() == kDecimalBase)
		item->SetMarked(true);

	message = new BMessage(kMsgBaseType);
	message->AddInt32("base_type", kHexBase);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE_COMMENT("Hex", 
		"A menu item, as short as possible, noun is recommended if it is "
		"shorter than adjective."), message, 'H'));
	item->SetTarget(this);
	if (fHeaderView->Base() == kHexBase)
		item->SetMarked(true);

	subMenu->SetRadioMode(true);
	menu->AddItem(new BMenuItem(subMenu));

	// Block Size

	subMenu = new BMenu(B_TRANSLATE_COMMENT("BlockSize", "A menu item, a "
		"shortened form from 'block size'. This is in the same menu window"
		"than 'Base' and 'Font size'"));
	subMenu->SetRadioMode(true);
	const uint32 blockSizes[] = {512, 1024, 2048};
	for (uint32 i = 0; i < sizeof(blockSizes) / sizeof(blockSizes[0]); i++) {
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%ld%s", blockSizes[i],
			fEditor.IsDevice() && fEditor.BlockSize() == blockSizes[i]
			? B_TRANSLATE(" (native)") : "");
		subMenu->AddItem(item = new BMenuItem(buffer,
			message = new BMessage(kMsgBlockSize)));
		message->AddInt32("block_size", blockSizes[i]);
		if (fEditor.BlockSize() == blockSizes[i])
			item->SetMarked(true);
	}
	if (subMenu->FindMarked() == NULL) {
		// if the device has some weird block size, we'll add it here, too
		char buffer[32];
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("%ld (native)"), fEditor.BlockSize());
		subMenu->AddItem(item = new BMenuItem(buffer,
			message = new BMessage(kMsgBlockSize)));
		message->AddInt32("block_size", fEditor.BlockSize());
		item->SetMarked(true);
	}
	subMenu->SetTargetForItems(this);
	menu->AddItem(new BMenuItem(subMenu));
	menu->AddSeparatorItem();

	// Font Size

	subMenu = new BMenu(B_TRANSLATE("Font size"));
	subMenu->SetRadioMode(true);
	const int32 fontSizes[] = {9, 10, 11, 12, 13, 14, 18, 24, 36, 48};
	int32 fontSize = int32(fDataView->FontSize() + 0.5);
	if (fDataView->FontSizeFitsBounds())
		fontSize = 0;
	for (uint32 i = 0; i < sizeof(fontSizes) / sizeof(fontSizes[0]); i++) {
		char buffer[16];
		snprintf(buffer, sizeof(buffer), "%ld", fontSizes[i]);
		subMenu->AddItem(item = new BMenuItem(buffer,
			message = new BMessage(kMsgFontSize)));
		message->AddFloat("font_size", fontSizes[i]);
		if (fontSizes[i] == fontSize)
			item->SetMarked(true);
	}
	subMenu->AddSeparatorItem();
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE_COMMENT("Fit", 
		"Size of fonts, fits to available room"), 
		message = new BMessage(kMsgFontSize)));
	message->AddFloat("font_size", 0.0f);
	if (fontSize == 0)
		item->SetMarked(true);

	subMenu->SetTargetForItems(this);
	menu->AddItem(new BMenuItem(subMenu));

	bar->AddItem(menu);
}


void
ProbeView::AllAttached()
{
	fHeaderView->SetTarget(fEditorLooper);
}


void
ProbeView::WindowActivated(bool active)
{
	if (!active)
		return;

	fDataView->MakeFocus(true);

	// set this view as the current find panel's target
	BMessage target(kMsgFindTarget);
	target.AddMessenger("target", this);
	be_app_messenger.SendMessage(&target);
}


void
ProbeView::_UpdateSelectionMenuItems(int64 start, int64 end)
{
	int64 position = 0;
	const uint8 *data = fDataView->DataAt(start);
	if (data == NULL) {
		fNativeMenuItem->SetEnabled(false);
		fSwappedMenuItem->SetEnabled(false);
		return;
	}

	// retrieve native endian position

	int size;
	if (end < start + 8)
		size = end + 1 - start;
	else
		size = 8;

	int64 bigEndianPosition = 0;
	memcpy(&bigEndianPosition, data, size);

	position = B_BENDIAN_TO_HOST_INT64(bigEndianPosition) >> (8 * (8 - size));

	// update menu items

	char buffer[128];
	if (fDataView->Base() == kHexBase)
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("Native: 0x%0*Lx"), size * 2, position);
	else
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("Native: %Ld (0x%0*Lx)"), position, size * 2, position);

	fNativeMenuItem->SetLabel(buffer);
	fNativeMenuItem->SetEnabled(position >= 0 && (position * fEditor.BlockSize()) < fEditor.FileSize());
	fNativeMenuItem->Message()->ReplaceInt64("block", position);

	position = B_SWAP_INT64(position) >> (8 * (8 - size));
	if (fDataView->Base() == kHexBase)
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("Swapped: 0x%0*Lx"), size * 2, position);
	else
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("Swapped: %Ld (0x%0*Lx)"), position, size * 2, position);

	fSwappedMenuItem->SetLabel(buffer);
	fSwappedMenuItem->SetEnabled(position >= 0 && (position * fEditor.BlockSize()) < fEditor.FileSize());
	fSwappedMenuItem->Message()->ReplaceInt64("block", position);
}


void
ProbeView::_UpdateBookmarkMenuItems()
{
	for (int32 i = 2; i < fBookmarkMenu->CountItems(); i++) {
		BMenuItem *item = fBookmarkMenu->ItemAt(i);
		if (item == NULL)
			break;

		BMessage *message = item->Message();
		if (message == NULL)
			break;

		off_t block = message->FindInt64("block");

		char buffer[128];
		if (fDataView->Base() == kHexBase)
			snprintf(buffer, sizeof(buffer), B_TRANSLATE("Block 0x%Lx"), block);
		else
			snprintf(buffer, sizeof(buffer), B_TRANSLATE("Block %Ld (0x%Lx)"), block, block);

		item->SetLabel(buffer);
	}
}


void
ProbeView::_AddBookmark(off_t position)
{
	int32 count = fBookmarkMenu->CountItems();

	if (count == 1) {
		fBookmarkMenu->AddSeparatorItem();
		count++;
	}

	// insert current position as bookmark

	off_t block = position / fEditor.BlockSize();

	off_t bookmark = -1;
	BMenuItem *item;
	int32 i;
	for (i = 2; (item = fBookmarkMenu->ItemAt(i)) != NULL; i++) {
		BMessage *message = item->Message();
		if (message != NULL && message->FindInt64("block", &bookmark) == B_OK) {
			if (block <= bookmark)
				break;
		}
	}

	// the bookmark already exists
	if (block == bookmark)
		return;

	char buffer[128];
	if (fDataView->Base() == kHexBase)
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("Block 0x%Lx"), block);
	else
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("Block %Ld (0x%Lx)"), block, block);

	BMessage *message;
	item = new BMenuItem(buffer, message = new BMessage(kMsgPositionUpdate));
	item->SetTarget(fHeaderView);
	if (count < 12)
		item->SetShortcut('0' + count - 2, B_COMMAND_KEY);
	message->AddInt64("block", block);

	fBookmarkMenu->AddItem(item, i);
}


void
ProbeView::_RemoveTypeEditor()
{
	if (fTypeView == NULL)
		return;

	if (Parent() != NULL)
		Parent()->RemoveChild(fTypeView);
	else
		Window()->RemoveChild(fTypeView);

	delete fTypeView;
	fTypeView = NULL;
}


void
ProbeView::_SetTypeEditor(int32 index)
{
	if (index == -1) {
		// remove type editor, show raw editor
		if (IsHidden())
			Show();

		_RemoveTypeEditor();
	} else {
		// hide raw editor, create and show type editor
		if (!IsHidden())
			Hide();

		_RemoveTypeEditor();

		fTypeView = new TypeView(Frame(), "type shell", index, fEditor,
			B_FOLLOW_ALL);

		if (Parent() != NULL)
			Parent()->AddChild(fTypeView);
		else
			Window()->AddChild(fTypeView);
	}
}


void
ProbeView::_CheckClipboard()
{
	if (!be_clipboard->Lock())
		return;

	bool hasData = false;
	BMessage *clip;
	if ((clip = be_clipboard->Data()) != NULL) {
		const void *data;
		ssize_t size;
		if (clip->FindData(B_FILE_MIME_TYPE, B_MIME_TYPE, &data, &size) == B_OK
			|| clip->FindData("text/plain", B_MIME_TYPE, &data, &size) == B_OK)
			hasData = true;
	}

	be_clipboard->Unlock();

	fPasteMenuItem->SetEnabled(hasData);
}


status_t
ProbeView::_PageSetup()
{
	BPrintJob printJob(Window()->Title());
	if (fPrintSettings != NULL)
		printJob.SetSettings(new BMessage(*fPrintSettings));

	status_t status = printJob.ConfigPage();
	if (status == B_OK) {
		// replace the print settings on success
		delete fPrintSettings;
		fPrintSettings = printJob.Settings();
	}

	return status;
}


void
ProbeView::_Print()
{
	if (fPrintSettings == NULL && _PageSetup() != B_OK)
		return;

	BPrintJob printJob(Window()->Title());
	printJob.SetSettings(new BMessage(*fPrintSettings));

	if (printJob.ConfigJob() == B_OK) {
		BRect rect = printJob.PrintableRect();

		float width, height;
		fDataView->GetPreferredSize(&width, &height);

		printJob.BeginJob();

		fDataView->SetScale(rect.Width() / width);
		printJob.DrawView(fDataView, rect, rect.LeftTop());
		fDataView->SetScale(1.0);
		printJob.SpoolPage();

		printJob.CommitJob();
	}
}


status_t
ProbeView::_Save()
{
	status_t status = fEditor.Save();
	if (status == B_OK)
		return B_OK;

	char buffer[1024];
	snprintf(buffer, sizeof(buffer),
		B_TRANSLATE("Writing to the file failed:\n"
		"%s\n\n"
		"All changes will be lost when you quit."),
		strerror(status));

	(new BAlert(B_TRANSLATE("DiskProbe request"),
		buffer, B_TRANSLATE("OK"), NULL, NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go(NULL);

	return status;
}


bool
ProbeView::QuitRequested()
{
	fEditorLooper->QuitFind();

	if (!fEditor.IsModified())
		return true;

	int32 chosen = (new BAlert(B_TRANSLATE("DiskProbe request"),
		B_TRANSLATE("Save changes before closing?"), B_TRANSLATE("Don't save"), B_TRANSLATE("Cancel"), 
		B_TRANSLATE("Save"),	B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();

	if (chosen == 0)
		return true;
	if (chosen == 1)
		return false;

	return _Save() == B_OK;
}


void
ProbeView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_SAVE_REQUESTED:
			_Save();
			break;

		case B_OBSERVER_NOTICE_CHANGE: {
			int32 what;
			if (message->FindInt32(B_OBSERVE_WHAT_CHANGE, &what) != B_OK)
				break;

			switch (what) {
				case kDataViewSelection:
				{
					int64 start, end;
					if (message->FindInt64("start", &start) == B_OK
						&& message->FindInt64("end", &end) == B_OK)
						_UpdateSelectionMenuItems(start, end);
					break;
				}
				case kDataViewPreferredSize:
					UpdateSizeLimits();
					break;
			}
			break;
		}

		case kMsgBaseType:
		{
			int32 type;
			if (message->FindInt32("base_type", &type) != B_OK)
				break;

			fHeaderView->SetBase((base_type)type);
			fDataView->SetBase((base_type)type);

			// The selection menu items depend on the base type as well
			int32 start, end;
			fDataView->GetSelection(start, end);
			_UpdateSelectionMenuItems(start, end);

			_UpdateBookmarkMenuItems();

			// update the application's settings
			BMessage update(*message);
			update.what = kMsgSettingsChanged;
			be_app_messenger.SendMessage(&update);
			break;
		}

		case kMsgFontSize:
		{
			float size;
			if (message->FindFloat("font_size", &size) != B_OK)
				break;

			fDataView->SetFontSize(size);

			// update the application's settings
			BMessage update(*message);
			update.what = kMsgSettingsChanged;
			be_app_messenger.SendMessage(&update);
			break;
		}

		case kMsgBlockSize:
		{
			int32 blockSize;
			if (message->FindInt32("block_size", &blockSize) != B_OK)
				break;

			BAutolock locker(fEditor);

			if (fEditor.SetViewSize(blockSize) == B_OK
				&& fEditor.SetBlockSize(blockSize) == B_OK)
				fHeaderView->SetTo(fEditor.ViewOffset(), blockSize);
			break;
		}

		case kMsgViewAs:
		{
			int32 index;
			if (message->FindInt32("editor index", &index) != B_OK)
				index = -1;

			_SetTypeEditor(index);
			break;
		}

		case kMsgAddBookmark:
			_AddBookmark(fHeaderView->Position());
			break;

		case kMsgPrint:
			_Print();
			break;

		case kMsgPageSetup:
			_PageSetup();
			break;

		case kMsgOpenFindWindow:
		{
			fEditorLooper->QuitFind();

			// set this view as the current find panel's target
			BMessage find(*fFindAgainMenuItem->Message());
			find.what = kMsgOpenFindWindow;
			find.AddMessenger("target", this);
			be_app_messenger.SendMessage(&find);
			break;
		}

		case kMsgFind:
		{
			const uint8 *data;
			ssize_t size;
			if (message->FindData("data", B_RAW_TYPE, (const void **)&data, &size) != B_OK) {
				// search again for last pattern
				BMessage *itemMessage = fFindAgainMenuItem->Message();
				if (itemMessage == NULL
					|| itemMessage->FindData("data", B_RAW_TYPE, (const void **)&data, &size) != B_OK) {
					// this shouldn't ever happen, but well...
					beep();
					break;
				}
			} else {
				// remember the search pattern
				fFindAgainMenuItem->SetMessage(new BMessage(*message));
				fFindAgainMenuItem->SetEnabled(true);
			}

			int32 start, end;
			fDataView->GetSelection(start, end);

			BMessage find(*message);
			find.AddInt64("start", fHeaderView->Position() + start + 1);
			find.AddMessenger("progress_monitor", BMessenger(fHeaderView));
			fEditorLooper->PostMessage(&find);
			break;
		}

		case kMsgStopFind:
			fEditorLooper->QuitFind();
			break;

		case B_NODE_MONITOR:
		{
			switch (message->FindInt32("opcode")) {
				case B_STAT_CHANGED:
					fEditor.ForceUpdate();
					break;
				case B_ATTR_CHANGED:
				{
					const char *name;
					if (message->FindString("attr", &name) != B_OK)
						break;

					if (fEditor.IsAttribute()) {
						if (!strcmp(name, fEditor.Attribute()))
							fEditor.ForceUpdate();
					} else {
						BMenuBar *bar = Window()->KeyMenuBar();
						if (bar != NULL) {
							BMenuItem *item = bar->FindItem("Attributes");
							if (item != NULL && item->Submenu() != NULL)
								_UpdateAttributesMenu(item->Submenu());
						}
					}

					// There might be a new icon
					if (!strcmp(name, "BEOS:TYPE")
						|| !strcmp(name, "BEOS:M:STD_ICON")
						|| !strcmp(name, "BEOS:L:STD_ICON")
						|| !strcmp(name, "BEOS:ICON"))
						fHeaderView->UpdateIcon();
					break;
				}
			}
			break;
		}

		case B_CLIPBOARD_CHANGED:
			_CheckClipboard();
			break;

		case kMsgDataEditorStateChange:
		{
			bool enabled;
			if (message->FindBool("can_undo", &enabled) == B_OK)
				fUndoMenuItem->SetEnabled(enabled);

			if (message->FindBool("can_redo", &enabled) == B_OK)
				fRedoMenuItem->SetEnabled(enabled);

			if (message->FindBool("modified", &enabled) == B_OK)
				fSaveMenuItem->SetEnabled(enabled);
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}

