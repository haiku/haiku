/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "ProbeView.h"
#include "DataView.h"
#include "DiskProbe.h"

#define BEOS_R5_COMPATIBLE
	// for SetLimits()

#include <Application.h>
#include <Window.h>
#include <Clipboard.h>
#include <Autolock.h>
#include <MessageQueue.h>
#include <TextControl.h>
#include <StringView.h>
#include <Slider.h>
#include <Bitmap.h>
#include <Box.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <String.h>
#include <Entry.h>
#include <Path.h>
#include <NodeInfo.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <Volume.h>
#include <fs_attr.h>

#include <stdio.h>


#define DRAW_SLIDER_BAR
	// if this is defined, the standard slider bar is replaced with
	// one that looks exactly like the one in the original DiskProbe
	// (even in Dano/Zeta)

static const uint32 kMsgSliderUpdate = 'slup';
static const uint32 kMsgPositionUpdate = 'poup';
static const uint32 kMsgLastPosition = 'lpos';


class IconView : public BView {
	public:
		IconView(BRect frame, entry_ref *ref, bool isDevice);
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
		HeaderView(BRect frame, entry_ref *ref, DataEditor &editor);
		virtual ~HeaderView();

		virtual void AttachedToWindow();
		virtual void Draw(BRect updateRect);
		virtual void GetPreferredSize(float *_width, float *_height);
		virtual void MessageReceived(BMessage *message);

		base_type Base() const { return fBase; }
		void SetBase(base_type);

	private:
		void FormatValue(char *buffer, size_t bufferSize, off_t value);
		void UpdatePositionViews();
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
};


class TypeMenuItem : public BMenuItem {
	public:
		TypeMenuItem(const char *name, const char *type, BMessage *message);
		
		virtual void GetContentSize(float *_width, float *_height);
		virtual void DrawContent();

	private:
		BString fType;
};


class UpdateLooper : public BLooper {
	public:
		UpdateLooper(const char *name, DataEditor &editor, BMessenger messenger);
		virtual ~UpdateLooper();

		virtual void MessageReceived(BMessage *message);

	private:
		DataEditor	&fEditor;
		BMessenger	fMessenger;
};


//----------------------


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


//	#pragma mark -


IconView::IconView(BRect rect, entry_ref *ref, bool isDevice)
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

	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fBitmap, updateRect, updateRect);
	SetDrawingMode(B_OP_COPY);
}


void 
IconView::UpdateIcon()
{
	if (fBitmap == NULL)
		fBitmap = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);

	if (fBitmap != NULL) {
		status_t status = B_ERROR;

		if (fIsDevice) {
			BPath path(&fRef);
			status = get_device_icon(path.Path(), fBitmap->Bits(), B_LARGE_ICON);
		} else
			status = BNodeInfo::GetTrackerIcon(&fRef, fBitmap);

		if (status != B_OK) {
			// ToDo: get a standard generic icon here?
			delete fBitmap;
			fBitmap = NULL;
		}

		Invalidate();
	}
}


//	#pragma mark -


PositionSlider::PositionSlider(BRect rect, const char *name, BMessage *message,
	off_t size, uint32 blockSize)
	: BSlider(rect, name, NULL, message, 0, kMaxSliderLimit, B_HORIZONTAL,
		B_TRIANGLE_THUMB, B_FOLLOW_LEFT_RIGHT),
	fSize(size),
	fBlockSize(blockSize)
{
	Reset();

#ifndef DRAW_SLIDER_BAR
	rgb_color color =  ui_color(B_CONTROL_HIGHLIGHT_COLOR);
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
	view->SetHighColor(ui_color(B_CONTROL_HIGHLIGHT_COLOR)); //102, 152, 203);
		// ToDo: the color should probably be retrieved from one of the ui colors
	view->FillRect(frame);

	frame.left = frame.right + 1;
	frame.right = barFrame.right - 1;
	view->SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	view->FillRect(frame);

	rgb_color cornerColor = tint_color(ViewColor(), B_DARKEN_1_TINT);
	rgb_color darkColor = tint_color(ViewColor(), B_DARKEN_3_TINT);

	view->BeginLineArray(9);

	// the corners
	view->AddLine(barFrame.LeftTop(), barFrame.LeftTop(), cornerColor);
	view->AddLine(barFrame.LeftBottom(), barFrame.LeftBottom(), cornerColor);
	view->AddLine(barFrame.RightTop(), barFrame.RightTop(), cornerColor);

	// the edges
	view->AddLine(BPoint(barFrame.left, barFrame.top + 1),
		BPoint(barFrame.left, barFrame.bottom - 1), darkColor);
	view->AddLine(BPoint(barFrame.right, barFrame.top + 1),
		BPoint(barFrame.right, barFrame.bottom), ui_color(B_SHINE_COLOR));

	barFrame.left++;
	barFrame.right--;
	view->AddLine(barFrame.LeftTop(), barFrame.RightTop(), darkColor);
	view->AddLine(barFrame.LeftBottom(), barFrame.RightBottom(), ui_color(B_SHINE_COLOR));

	// the inner edges
	barFrame.top++;
	view->AddLine(barFrame.LeftTop(), barFrame.RightTop(), ui_color(B_SHADOW_COLOR));
	view->AddLine(BPoint(barFrame.left, barFrame.top + 1),
		BPoint(barFrame.left, barFrame.bottom - 1), ui_color(B_SHADOW_COLOR));

	view->EndLineArray();
}
#endif	// DRAW_SLIDER_BAR


void
PositionSlider::Reset()
{
	SetKeyIncrementValue(int32(1.0 * kMaxSliderLimit / ((fSize - 1) / fBlockSize) + 0.5));
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

	SetEnabled(fSize > fBlockSize);

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


//	#pragma mark -


HeaderView::HeaderView(BRect frame, entry_ref *ref, DataEditor &editor)
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

	BStringView *stringView = new BStringView(BRect(50, 6, frame.right, 20),
		B_EMPTY_STRING, editor.IsAttribute() ? "Attribute: " : editor.IsDevice() ? "Device: " : "File: ");
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
	BRect rect = stringView->Frame();
	rect.left = rect.right;
	rect.right = frame.right;
	fPathView = new BStringView(rect, B_EMPTY_STRING, string.String());
	fPathView->SetFont(&plainFont);
	AddChild(fPathView);

	float top = 27;
	if (editor.IsAttribute()) {
		top += 3;
		stringView = new BStringView(BRect(50, top, frame.right, top + 15), B_EMPTY_STRING, "Attribute Type: ");
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
		fTypeControl = new BTextControl(rect, B_EMPTY_STRING, NULL, buffer, new BMessage(kMsgPositionUpdate));
		fTypeControl->SetDivider(0.0);
		fTypeControl->SetFont(&plainFont);
		fTypeControl->TextView()->SetFontAndColor(&plainFont);
		fTypeControl->SetEnabled(false);
			// ToDo: for now
		AddChild(fTypeControl);

		top += 24;
	} else
		fTypeControl = NULL;

	stringView = new BStringView(BRect(50, top, frame.right, top + 15), B_EMPTY_STRING, "Block: ");
	stringView->SetFont(&boldFont);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect = stringView->Frame();
	rect.left = rect.right;
	rect.right += 75;
	rect.OffsetBy(0, -2);
		// BTextControl oddities
	fPositionControl = new BTextControl(rect, B_EMPTY_STRING, NULL, "0x0", new BMessage(kMsgPositionUpdate));
	fPositionControl->SetDivider(0.0);
	fPositionControl->SetFont(&plainFont);
	fPositionControl->TextView()->SetFontAndColor(&plainFont);
	fPositionControl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	AddChild(fPositionControl);

	rect.left = rect.right + 4;
	rect.right = rect.left + 75;
	rect.OffsetBy(0, 2);
	fSizeView = new BStringView(rect, B_EMPTY_STRING, "of 0x0");
	fSizeView->SetFont(&plainFont);
	AddChild(fSizeView);
	UpdateFileSizeView();

	rect.left = rect.right + 4;
	rect.right = frame.right;
	stringView = new BStringView(rect, B_EMPTY_STRING, "Offset: ");
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
	stringView = new BStringView(rect, B_EMPTY_STRING,
		editor.IsAttribute() ? "Attribute Offset: " : editor.IsDevice() ? "Device Offset: " : "File Offset: ");
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
	fPositionSlider = new PositionSlider(rect, "slider", new BMessage(kMsgSliderUpdate),
		editor.FileSize(), editor.BlockSize());
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

	fPositionControl->SetTarget(this);
	fPositionSlider->SetTarget(this);
}


void 
HeaderView::Draw(BRect updateRect)
{
	BRect rect = Bounds();

	SetHighColor(ui_color(B_SHINE_COLOR));
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
HeaderView::FormatValue(char *buffer, size_t bufferSize, off_t value)
{
	snprintf(buffer, bufferSize, fBase == kHexBase ? "0x%Lx" : "%Ld", value);
}


void
HeaderView::UpdatePositionViews()
{
	char buffer[64];
	FormatValue(buffer, sizeof(buffer), fPosition / fBlockSize);
	fPositionControl->SetText(buffer);

	FormatValue(buffer, sizeof(buffer), fPosition + fOffset);
	fFileOffsetView->SetText(buffer);
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
	char buffer[64];
	strcpy(buffer, "of ");
	FormatValue(buffer + 3, sizeof(buffer) - 3, (fFileSize + fBlockSize - 1) / fBlockSize);
	fSizeView->SetText(buffer);
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

			if (Looper()->MessageQueue()->FindMessage(kMsgSliderUpdate, 0) != NULL)
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

		case kMsgPositionUpdate:
		{
			fLastPosition = fPosition;

			int32 delta;
			if (message->FindInt32("delta", &delta) != B_OK)
				fPosition = strtoll(fPositionControl->Text(), NULL, 0) * fBlockSize;
			else
				fPosition += delta * fBlockSize;

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


//	#pragma mark -


/**	The TypeMenuItem is a BMenuItem that displays a type string
 *	at its right border.
 *	It is used to display the attribute and type in the attributes menu.
 *	It does not mix nicely with short cuts.
 */

TypeMenuItem::TypeMenuItem(const char *name, const char *type, BMessage *message)
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

	// draw the type
	BPoint point = Menu()->PenLocation();
	point.x = Frame().right - 4 - Menu()->StringWidth(fType.String());
	Menu()->DrawString(fType.String(), point);
}


//	#pragma mark -


/** The purpose of this looper is to off-load the editor data
 *	loading from the main window looper.
 *	It will listen to the offset changes of the editor, let
 *	him update its data, and will then synchronously notify
 *	the target.
 *	That way, simple offset changes will not stop the main
 *	looper from operating. Therefore, all offset updates
 *	for the editor will go through this looper.
 */

UpdateLooper::UpdateLooper(const char *name, DataEditor &editor, BMessenger target)
	: BLooper(name),
	fEditor(editor),
	fMessenger(target)
{
	fEditor.StartWatching(this);
}


UpdateLooper::~UpdateLooper()
{
	fEditor.StopWatching(this);
}


void
UpdateLooper::MessageReceived(BMessage *message)
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

		case kMsgDataEditorOffsetChange:
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

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


//	#pragma mark -


ProbeView::ProbeView(BRect rect, entry_ref *ref, const char *attribute, const BMessage *settings)
	: BView(rect, "probeView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fEditor.SetTo(*ref, attribute);

	int32 baseType = kHexBase;
	float fontSize = 12.0f;
	if (settings != NULL) {
		settings->FindInt32("base_type", &baseType);
		settings->FindFloat("font_size", &fontSize);
	}

	rect = Bounds();
	fHeaderView = new HeaderView(rect, ref, fEditor);
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

	fScrollView = new BScrollView("scroller", fDataView, B_FOLLOW_ALL, B_WILL_DRAW, true, true);
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

	float width, height;
	fDataView->GetPreferredSize(&width, &height);

	BRect frame = Window()->ConvertFromScreen(ConvertToScreen(fHeaderView->Frame()));

	Window()->SetSizeLimits(200, width + B_V_SCROLL_BAR_WIDTH,
		200, height + frame.bottom + 4 + B_H_SCROLL_BAR_HEIGHT);
}


void 
ProbeView::DetachedFromWindow()
{
	if (fUpdateLooper->Lock())
		fUpdateLooper->Quit();
	fUpdateLooper = NULL;

	fEditor.StopWatching(this);
	fDataView->StopWatching(fHeaderView, kDataViewCursorPosition);
	fDataView->StopWatching(this, kDataViewSelection);
	be_clipboard->StopWatching(this);
}


void
ProbeView::UpdateAttributesMenu(BMenu *menu)
{
	// remove old contents

	for (int32 i = menu->CountItems(); i-- > 0;) {
		delete menu->RemoveItem(i);
	}

	// add new items (sorted)

	BNode node(&fEditor.Ref());
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
			message->AddRef("refs", &fEditor.Ref());
			message->AddString("attributes", attribute);

			menu->AddItem(new TypeMenuItem(attribute, type, message), i);
		}
	}

	if (menu->CountItems() == 0) {
		// if there are no attributes, add an item to the menu
		// that says so
		BMenuItem *item = new BMenuItem("none", NULL);
		item->SetEnabled(false);
		menu->AddItem(item);
	}

	menu->SetTargetForItems(be_app);
}


void 
ProbeView::AddFileMenuItems(BMenu *menu, int32 index)
{
	BMenuItem *item;
	menu->AddItem(item = new BMenuItem("Page Setup" B_UTF8_ELLIPSIS, NULL), index++);
	item->SetEnabled(false);
	menu->AddItem(item = new BMenuItem("Print" B_UTF8_ELLIPSIS, NULL, 'P', B_COMMAND_KEY), index++);
	item->SetEnabled(false);
}


void 
ProbeView::AttachedToWindow()
{
	fUpdateLooper = new UpdateLooper(fEditor.Ref().name, fEditor, BMessenger(fDataView));
	fUpdateLooper->Run();

	fEditor.StartWatching(this);
	fDataView->StartWatching(fHeaderView, kDataViewCursorPosition);
	fDataView->StartWatching(this, kDataViewSelection);
	be_clipboard->StartWatching(this);

	// Add menu to window

	BMenuBar *bar = Window()->KeyMenuBar();
	if (bar == NULL) {
		// there is none? Well, but we really want to have one
		bar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
		Window()->AddChild(bar);

		MoveBy(0, bar->Bounds().Height());
		ResizeBy(0, -bar->Bounds().Height());

		BMenu *menu = new BMenu(fEditor.IsAttribute() ? "Attribute" : fEditor.IsDevice() ? "Device" : "File");
		AddFileMenuItems(menu, 0);
		menu->AddSeparatorItem();

		menu->AddItem(new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED), 'W', B_COMMAND_KEY));
		bar->AddItem(menu);
	}

	// "Edit" menu

	BMenu *menu = new BMenu("Edit");
	BMenuItem *item;
	menu->AddItem(fUndoMenuItem = new BMenuItem("Undo", new BMessage(B_UNDO), 'Z', B_COMMAND_KEY));
	fUndoMenuItem->SetEnabled(fEditor.CanUndo());
	fUndoMenuItem->SetTarget(fDataView);
	menu->AddItem(fRedoMenuItem = new BMenuItem("Redo", new BMessage(B_REDO), 'Z', B_COMMAND_KEY | B_SHIFT_KEY));
	fRedoMenuItem->SetEnabled(fEditor.CanRedo());
	fRedoMenuItem->SetTarget(fDataView);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem("Copy", new BMessage(B_COPY), 'C', B_COMMAND_KEY));
	item->SetTarget(fDataView);
	menu->AddItem(fPasteMenuItem = new BMenuItem("Paste", new BMessage(B_PASTE), 'V', B_COMMAND_KEY));
	fPasteMenuItem->SetTarget(fDataView);
	CheckClipboard();
	menu->AddItem(item = new BMenuItem("Select All", new BMessage(B_SELECT_ALL), 'A', B_COMMAND_KEY));
	item->SetTarget(fDataView);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Find" B_UTF8_ELLIPSIS, NULL, 'F', B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Find Again", NULL, 'G', B_COMMAND_KEY));
	bar->AddItem(menu);

	// "Block" menu

	menu = new BMenu("Block");
	BMessage *message = new BMessage(kMsgPositionUpdate);
	message->AddInt32("delta", 1);
	menu->AddItem(item = new BMenuItem("Next", message, B_RIGHT_ARROW, B_COMMAND_KEY));
	item->SetTarget(fHeaderView);
	message = new BMessage(kMsgPositionUpdate);
	message->AddInt32("delta", -1);
	menu->AddItem(item = new BMenuItem("Previous", message, B_LEFT_ARROW, B_COMMAND_KEY));
	item->SetTarget(fHeaderView);
	menu->AddItem(item = new BMenuItem("Back", new BMessage(kMsgLastPosition), 'J', B_COMMAND_KEY));
	item->SetTarget(fHeaderView);

	BMenu *subMenu = new BMenu("Selection");
	menu->AddItem(new BMenuItem(subMenu));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Write", NULL, 'W', B_COMMAND_KEY));
	menu->AddSeparatorItem();

	subMenu = new BMenu("Bookmarks");
	subMenu->AddItem(new BMenuItem("Add", NULL, 'B', B_COMMAND_KEY));
	menu->AddItem(new BMenuItem(subMenu));
	bar->AddItem(menu);

	// "Attributes" menu (it's only visible if the underlying
	// file system actually supports attributes)

	BVolume volume;
	if (!fEditor.IsAttribute()
		&& fEditor.File().GetVolume(&volume) == B_OK
		&& (volume.KnowsMime() || volume.KnowsAttr())) {
		bar->AddItem(menu = new BMenu("Attributes"));
		UpdateAttributesMenu(menu);
	}

	// "View" menu

	menu = new BMenu("View");

	// Number Base (hex/decimal)

	subMenu = new BMenu("Base");
	message = new BMessage(kMsgBaseType);
	message->AddInt32("base_type", kDecimalBase);
	subMenu->AddItem(item = new BMenuItem("Decimal", message, 'D', B_COMMAND_KEY));
	item->SetTarget(this);
	if (fHeaderView->Base() == kDecimalBase)
		item->SetMarked(true);

	message = new BMessage(kMsgBaseType);
	message->AddInt32("base_type", kHexBase);
	subMenu->AddItem(item = new BMenuItem("Hex", message, 'H', B_COMMAND_KEY));
	item->SetTarget(this);
	if (fHeaderView->Base() == kHexBase)
		item->SetMarked(true);

	subMenu->SetRadioMode(true);
	menu->AddItem(new BMenuItem(subMenu));

	// Block Size

	subMenu = new BMenu("BlockSize");
	subMenu->AddItem(new BMenuItem("512", NULL));
	menu->AddItem(new BMenuItem(subMenu));
	menu->AddSeparatorItem();

	// Font Size

	subMenu = new BMenu("Font Size");
	const int32 fontSizes[] = {9, 10, 12, 14, 18, 24, 36, 48};
	int32 fontSize = int32(fDataView->FontSize() + 0.5);
	for (uint32 i = 0; i < sizeof(fontSizes) / sizeof(fontSizes[0]); i++) {
		char buffer[16];
		snprintf(buffer, sizeof(buffer), "%ld", fontSizes[i]);
		subMenu->AddItem(item = new BMenuItem(buffer, NULL));
		if (fontSizes[i] == fontSize)
			item->SetMarked(true);
	}
	subMenu->AddSeparatorItem();
	subMenu->AddItem(item = new BMenuItem("Fit", NULL));
	if (fontSize == 0)
		item->SetMarked(true);

	subMenu->SetRadioMode(true);
	menu->AddItem(new BMenuItem(subMenu));

	bar->AddItem(menu);
}


void 
ProbeView::AllAttached()
{
	fHeaderView->SetTarget(fUpdateLooper);
}


void
ProbeView::WindowActivated(bool active)
{
	if (active)
		fDataView->MakeFocus(true);
}


void
ProbeView::CheckClipboard()
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


void 
ProbeView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgBaseType:
		{
			int32 type;
			if (message->FindInt32("base_type", &type) != B_OK)
				break;

			fHeaderView->SetBase((base_type)type);
			fDataView->SetBase((base_type)type);

			// update the applications settings
			BMessage update(*message);
			update.what = kMsgSettingsChanged;
			be_app_messenger.SendMessage(&update);
			break;
		}

		case B_NODE_MONITOR:
		{
			switch (message->FindInt32("opcode")) {
				case B_ATTR_CHANGED:
				{
					BMenuBar *bar = Window()->KeyMenuBar();
					if (bar != NULL) {
						BMenuItem *item = bar->FindItem("Attributes");
						if (item != NULL && item->Submenu() != NULL)
							UpdateAttributesMenu(item->Submenu());
					}
					break;
				}
			}
			break;
		}

		case B_CLIPBOARD_CHANGED:
			CheckClipboard();
			break;

		case kMsgDataEditorStateChange:
		{
			bool enabled;
			if (message->FindBool("can_undo", &enabled) == B_OK)
				fUndoMenuItem->SetEnabled(enabled);

			if (message->FindBool("can_redo", &enabled) == B_OK)
				fRedoMenuItem->SetEnabled(enabled);
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}

