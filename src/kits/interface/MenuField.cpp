/*
 * Copyright 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include <stdlib.h>
#include <string.h>

#include <AbstractLayoutItem.h>
#include <LayoutUtils.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <Message.h>
#include <BMCPrivate.h>
#include <Window.h>


class BMenuField::LabelLayoutItem : public BAbstractLayoutItem {
public:
								LabelLayoutItem(BMenuField* parent);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

private:
			BMenuField*			fParent;
			BRect				fFrame;
};


class BMenuField::MenuBarLayoutItem : public BAbstractLayoutItem {
public:
								MenuBarLayoutItem(BMenuField* parent);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

private:
			BMenuField*			fParent;
			BRect				fFrame;
};


struct BMenuField::LayoutData {
	LayoutData()
		: label_layout_item(NULL),
		  menu_bar_layout_item(NULL),
		  previous_height(-1),
		  valid(false)
	{
	}

	LabelLayoutItem*	label_layout_item;
	MenuBarLayoutItem*	menu_bar_layout_item;
	float				previous_height;	// used in FrameResized() for
											// invalidation
	font_height			font_info;
	float				label_width;
	float				label_height;
	BSize				min;
	BSize				menu_bar_min;
	bool				valid;
};


// #pragma mark -


static float kVMargin = 2.0f;


BMenuField::BMenuField(BRect frame, const char *name, const char *label,
	BMenu *menu, uint32 resize, uint32 flags)
	: BView(frame, name, resize, flags)
{
	InitObject(label);

	frame.OffsetTo(B_ORIGIN);
	_InitMenuBar(menu, frame, false);

	InitObject2();
}


BMenuField::BMenuField(BRect frame, const char *name, const char *label,
	BMenu *menu, bool fixedSize, uint32 resize, uint32 flags)
	: BView(frame, name, resize, flags)
{
	InitObject(label);

	fFixedSizeMB = fixedSize;

	frame.OffsetTo(B_ORIGIN);
	_InitMenuBar(menu, frame, fixedSize);

	InitObject2();
}


BMenuField::BMenuField(const char* name, const char* label, BMenu* menu,
					   BMessage* message, uint32 flags)
	: BView(BRect(0, 0, -1, -1), name, B_FOLLOW_NONE,
			flags | B_FRAME_EVENTS | B_SUPPORTS_LAYOUT)
{
	InitObject(label);

	_InitMenuBar(menu, BRect(0, 0, 100, 15), false);

	InitObject2();
}


BMenuField::BMenuField(const char* label,
					   BMenu* menu, BMessage* message)
	: BView(BRect(0, 0, -1, -1), NULL, B_FOLLOW_NONE,
			B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS | B_SUPPORTS_LAYOUT)
{
	InitObject(label);

	_InitMenuBar(menu, BRect(0, 0, 100, 15), false);

	InitObject2();
}


BMenuField::BMenuField(BMessage *data)
	: BView(data)
{
	const char *label = NULL;
	data->FindString("_label", &label);

	InitObject(label);

	fMenuBar = (BMenuBar*)FindView("_mc_mb_");
	fMenu = fMenuBar->SubmenuAt(0);

	InitObject2();

	bool disable;
	if (data->FindBool("_disable", &disable) == B_OK)
		SetEnabled(!disable);

	int32 align;
	data->FindInt32("_align", &align);
		SetAlignment((alignment)align);

	data->FindFloat("_divide", &fDivider);

	bool fixed;
	if (data->FindBool("be:fixeds", &fixed) == B_OK)
		fFixedSizeMB = fixed;

	bool dmark = false;
	data->FindBool("be:dmark", &dmark);
	if (_BMCMenuBar_ *menuBar = dynamic_cast<_BMCMenuBar_ *>(fMenuBar)) {
		menuBar->TogglePopUpMarker(dmark);
	}
}


BMenuField::~BMenuField()
{
	free(fLabel);

	status_t dummy;
	if (fMenuTaskID >= 0)
		wait_for_thread(fMenuTaskID, &dummy);

	delete fLayoutData;
}


BArchivable *
BMenuField::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BMenuField"))
		return new BMenuField(data);

	return NULL;
}


status_t
BMenuField::Archive(BMessage *data, bool deep) const
{
	status_t ret = BView::Archive(data, deep);

	if (ret == B_OK && Label())
		ret = data->AddString("_label", Label());

	if (ret == B_OK && !IsEnabled())
		ret = data->AddBool("_disable", true);

	if (ret == B_OK)
		ret = data->AddInt32("_align", Alignment());
	if (ret == B_OK)
		ret = data->AddFloat("_divide", Divider());

	if (ret == B_OK && fFixedSizeMB)
		ret = data->AddBool("be:fixeds", true);

	bool dmark = false;
	if (_BMCMenuBar_ *menuBar = dynamic_cast<_BMCMenuBar_ *>(fMenuBar)) {
		dmark = menuBar->IsPopUpMarkerShown();
	}
	data->AddBool("be:dmark", dmark);

	return ret;
}


void
BMenuField::Draw(BRect update)
{
	BRect bounds(Bounds());
	bool active = false;

	if (IsFocus())
		active = Window()->IsActive();

	DrawLabel(bounds, update);

	BRect frame(fMenuBar->Frame());

	if (frame.InsetByCopy(-kVMargin, -kVMargin).Intersects(update)) {
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT));
		StrokeLine(BPoint(frame.left - 1.0f, frame.top - 1.0f),
			BPoint(frame.left - 1.0f, frame.bottom - 1.0f));
		StrokeLine(BPoint(frame.left - 1.0f, frame.top - 1.0f),
			BPoint(frame.right - 1.0f, frame.top - 1.0f));

		StrokeLine(BPoint(frame.left + 1.0f, frame.bottom + 1.0f),
			BPoint(frame.right + 1.0f, frame.bottom + 1.0f));
		StrokeLine(BPoint(frame.right + 1.0f, frame.top + 1.0f));

		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_4_TINT));
		StrokeLine(BPoint(frame.left - 1.0f, frame.bottom),
			BPoint(frame.left - 1.0f, frame.bottom));
		StrokeLine(BPoint(frame.right, frame.top - 1.0f),
			BPoint(frame.right, frame.top - 1.0f));
	}

	if (active || fTransition) {
		SetHighColor(active ? ui_color(B_KEYBOARD_NAVIGATION_COLOR) :
			ViewColor());
		StrokeRect(frame.InsetByCopy(-kVMargin, -kVMargin));

		fTransition = false;
	}
}


void
BMenuField::AttachedToWindow()
{
	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
		SetLowColor(Parent()->ViewColor());
	}
}


void
BMenuField::AllAttached()
{
	ResizeTo(Bounds().Width(),
		fMenuBar->Bounds().Height() + kVMargin + kVMargin);
}


void
BMenuField::MouseDown(BPoint where)
{
	if (!fMenuBar->Frame().Contains(where))
		return;

	BRect bounds = fMenuBar->ConvertFromParent(Bounds());

	fMenuBar->StartMenuBar(0, false, true, &bounds);

	fMenuTaskID = spawn_thread((thread_func)_thread_entry, "_m_task_", B_NORMAL_PRIORITY, this);
	if (fMenuTaskID >= 0)
		resume_thread(fMenuTaskID);
}


void
BMenuField::KeyDown(const char *bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_SPACE:
		case B_RIGHT_ARROW:
		case B_DOWN_ARROW:
		{
			if (!IsEnabled())
				break;

			BRect bounds = fMenuBar->ConvertFromParent(Bounds());

			fMenuBar->StartMenuBar(0, true, true, &bounds);

			fSelected = true;
			fTransition = true;

			bounds = Bounds();
			bounds.right = fDivider;

			Invalidate(bounds);
		}

		default:
			BView::KeyDown(bytes, numBytes);
	}
}


void
BMenuField::MakeFocus(bool state)
{
	if (IsFocus() == state)
		return;

	BView::MakeFocus(state);

	if (Window())
		Invalidate(); // TODO: use fLayoutData->label_width
}


void
BMenuField::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}


void
BMenuField::WindowActivated(bool state)
{
	BView::WindowActivated(state);

	if (IsFocus())
		Invalidate();
}


void
BMenuField::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}


void
BMenuField::MouseMoved(BPoint point, uint32 code, const BMessage *message)
{
	BView::MouseMoved(point, code, message);
}


void
BMenuField::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BMenuField::AllDetached()
{
	BView::AllDetached();
}


void
BMenuField::FrameMoved(BPoint newPosition)
{
	BView::FrameMoved(newPosition);
}


void
BMenuField::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);

	if (newHeight != fLayoutData->previous_height && Label()) {
		// The height changed, which means the label has to move and we
		// probably also invalidate a part of the borders around the menu bar.
		// So don't be shy and invalidate the whole thing.
		Invalidate();
	}

	fLayoutData->previous_height = newHeight;
}


BMenu *
BMenuField::Menu() const
{
	return fMenu;
}


BMenuBar *
BMenuField::MenuBar() const
{
	return fMenuBar;
}


BMenuItem *
BMenuField::MenuItem() const
{
	return fMenuBar->ItemAt(0);
}


void
BMenuField::SetLabel(const char *label)
{
	if (fLabel) {
		if (label && strcmp(fLabel, label) == 0)
			return;

		free(fLabel);
	}

	fLabel = strdup(label);

	if (Window())
		Invalidate();

	InvalidateLayout();
}


const char *
BMenuField::Label() const
{
	return fLabel;
}


void
BMenuField::SetEnabled(bool on)
{
	if (fEnabled == on)
		return;

	fEnabled = on;
	fMenuBar->SetEnabled(on);

	if (Window()) {
		fMenuBar->Invalidate(fMenuBar->Bounds());
		Invalidate(Bounds());
	}
}


bool
BMenuField::IsEnabled() const
{
	return fEnabled;
}


void
BMenuField::SetAlignment(alignment label)
{
	fAlign = label;
}


alignment
BMenuField::Alignment() const
{
	return fAlign;
}


void
BMenuField::SetDivider(float divider)
{
	divider = floorf(divider + 0.5);

	float dx = fDivider - divider;

	if (dx == 0.0f)
		return;

	fDivider = divider;

	if (Flags() & B_SUPPORTS_LAYOUT) {
		// We should never get here, since layout support means, we also
		// layout the divider, and don't use this method at all.
		Relayout();
	} else {
		BRect dirty(fMenuBar->Frame());

		fMenuBar->MoveTo(fDivider + 1, kVMargin);

		if (fFixedSizeMB) {
			fMenuBar->ResizeTo(Bounds().Width() - fDivider - 2,
							   dirty.Height());
		}

		dirty = dirty | fMenuBar->Frame();
		dirty.InsetBy(-kVMargin, -kVMargin);

		Invalidate(dirty);
	}
}


float
BMenuField::Divider() const
{
	return fDivider;
}


void
BMenuField::ShowPopUpMarker()
{
	if (_BMCMenuBar_ *menuBar = dynamic_cast<_BMCMenuBar_ *>(fMenuBar)) {
		menuBar->TogglePopUpMarker(true);
		menuBar->Invalidate();
	}
}


void
BMenuField::HidePopUpMarker()
{
	if (_BMCMenuBar_ *menuBar = dynamic_cast<_BMCMenuBar_ *>(fMenuBar)) {
		menuBar->TogglePopUpMarker(false);
		menuBar->Invalidate();
	}
}


BHandler *
BMenuField::ResolveSpecifier(BMessage *message, int32 index,
	BMessage *specifier, int32 form, const char *property)
{
	return BView::ResolveSpecifier(message, index, specifier, form, property);
}


status_t
BMenuField::GetSupportedSuites(BMessage *data)
{
	return BView::GetSupportedSuites(data);
}


void
BMenuField::ResizeToPreferred()
{
	fMenuBar->ResizeToPreferred();

	BView::ResizeToPreferred();

	if (fFixedSizeMB) {
		// we have let the menubar resize itsself, but
		// in fixed size mode, the menubar is supposed to
		// be at the right end of the view always. Since
		// the menu bar is in follow left/right mode then,
		// resizing ourselfs might have caused the menubar
		// to be outside now
		fMenuBar->ResizeTo(Bounds().Width() - fDivider - 2,
			fMenuBar->Frame().Height());
	}
}


void
BMenuField::GetPreferredSize(float *_width, float *_height)
{
	_ValidateLayoutData();

	if (_width)
		*_width = fLayoutData->min.width;

	if (_height)
		*_height = fLayoutData->min.height;
}


BSize
BMenuField::MinSize()
{
	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), fLayoutData->min);
}


BSize
BMenuField::MaxSize()
{
	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), fLayoutData->min);
}


BSize
BMenuField::PreferredSize()
{
	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), fLayoutData->min);
}


void
BMenuField::InvalidateLayout(bool descendants)
{
	fLayoutData->valid = false;

	BView::InvalidateLayout(descendants);
}


BLayoutItem*
BMenuField::CreateLabelLayoutItem()
{
	if (!fLayoutData->label_layout_item)
		fLayoutData->label_layout_item = new LabelLayoutItem(this);
	return fLayoutData->label_layout_item;
}


BLayoutItem*
BMenuField::CreateMenuBarLayoutItem()
{
	if (!fLayoutData->menu_bar_layout_item)
		fLayoutData->menu_bar_layout_item = new MenuBarLayoutItem(this);
	return fLayoutData->menu_bar_layout_item;
}


status_t
BMenuField::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}


void
BMenuField::DoLayout()
{
	// Bail out, if we shan't do layout.
	if (!(Flags() & B_SUPPORTS_LAYOUT))
		return;

	// If the user set a layout, we let the base class version call its
	// hook.
	if (GetLayout()) {
		BView::DoLayout();
		return;
	}

	_ValidateLayoutData();

	// validate current size
	BSize size(Bounds().Size());
	if (size.width < fLayoutData->min.width)
		size.width = fLayoutData->min.width;
	if (size.height < fLayoutData->min.height)
		size.height = fLayoutData->min.height;

	// divider
	float divider = 0;
	if (fLayoutData->label_layout_item && fLayoutData->menu_bar_layout_item) {
		// We have layout items. They define the divider location.
		divider = fLayoutData->menu_bar_layout_item->Frame().left
			- fLayoutData->label_layout_item->Frame().left;
	} else {
		if (fLayoutData->label_width > 0)
			divider = fLayoutData->label_width + 5;
	}

	// menu bar
	BRect dirty(fMenuBar->Frame());
	BRect menuBarFrame(divider + 1, kVMargin, size.width - 2,
		size.height - kVMargin);

	// place the menu bar and set the divider
	BLayoutUtils::AlignInFrame(fMenuBar, menuBarFrame);

	fDivider = divider;

	// invalidate dirty region
	dirty = dirty | fMenuBar->Frame();
	dirty.InsetBy(-kVMargin, -kVMargin);

	Invalidate(dirty);
}


void BMenuField::_ReservedMenuField1() {}
void BMenuField::_ReservedMenuField2() {}
void BMenuField::_ReservedMenuField3() {}


BMenuField &
BMenuField::operator=(const BMenuField &)
{
	return *this;
}


void
BMenuField::InitObject(const char *label)
{
	fLabel = NULL;
	fMenu = NULL;
	fMenuBar = NULL;
	fAlign = B_ALIGN_LEFT;
	fEnabled = true;
	fSelected = false;
	fTransition = false;
	fFixedSizeMB = false;
	fMenuTaskID = -1;
	fLayoutData = new LayoutData;

	SetLabel(label);

	if (label)
		fDivider = (float)floor(Frame().Width() / 2.0f);
	else
		fDivider = 0;

	// default to unlimited maximum width
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
}


void
BMenuField::InitObject2()
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	// TODO: fix this calculation
	float height = floorf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading) + 7;

	fMenuBar->ResizeTo(Bounds().Width() - fDivider - 2, height);
	fMenuBar->AddFilter(new _BMCFilter_(this, B_MOUSE_DOWN));
}


void
BMenuField::DrawLabel(BRect bounds, BRect update)
{
	_ValidateLayoutData();
	font_height& fh = fLayoutData->font_info;

	if (Label()) {
		SetLowColor(ViewColor());

		// horizontal alignment
		float x;
		switch (fAlign) {
			case B_ALIGN_RIGHT:
				x = fDivider - fLayoutData->label_width - 3.0f;
				break;

			case B_ALIGN_CENTER:
				x = fDivider - fLayoutData->label_width / 2.0f;
				break;

			default:
				x = 3.0f;
				break;
		}

		// vertical alignment
		float y = Bounds().top
			+ (Bounds().Height() + 1 - fh.ascent - fh.descent) / 2
			+ fh.ascent;
		y = floor(y + 0.5);

		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			IsEnabled() ? B_DARKEN_MAX_TINT : B_DISABLED_LABEL_TINT));
		DrawString(Label(), BPoint(x, y));
	}
}


void
BMenuField::InitMenu(BMenu *menu)
{
	menu->SetFont(be_plain_font);

	int32 index = 0;
	BMenu *subMenu;

	while ((subMenu = menu->SubmenuAt(index++)) != NULL)
		InitMenu(subMenu);
}


/* static */
int32
BMenuField::_thread_entry(void *arg)
{
	return static_cast<BMenuField *>(arg)->_MenuTask();
}


int32
BMenuField::_MenuTask()
{
	if (!LockLooper())
		return 0;
	
	fSelected = true;
	fTransition = true;
	Invalidate();
	UnlockLooper();
	
	bool tracking;
	do {
		snooze(20000);
		if (!LockLooper())
			return 0;

		tracking = fMenuBar->fTracking;

		UnlockLooper();
	} while (tracking);

	if (LockLooper()) {
		fSelected = false;
		fTransition = true;
		Invalidate();	
		UnlockLooper();
	}

	return 0;
}


void
BMenuField::_UpdateFrame()
{
	if (fLayoutData->label_layout_item && fLayoutData->menu_bar_layout_item) {
		BRect labelFrame = fLayoutData->label_layout_item->Frame();
		BRect menuFrame = fLayoutData->menu_bar_layout_item->Frame();

		// update divider
		fDivider = menuFrame.left - labelFrame.left;

		// update our frame
		MoveTo(labelFrame.left, labelFrame.top);
		BSize oldSize = Bounds().Size();
		ResizeTo(menuFrame.left + menuFrame.Width() - labelFrame.left,
			menuFrame.top + menuFrame.Height() - labelFrame.top);
		BSize newSize = Bounds().Size();

		// If the size changes, ResizeTo() will trigger a relayout, otherwise
		// we need to do that explicitly.
		if (newSize != oldSize)
			Relayout();
	}
}


void
BMenuField::_InitMenuBar(BMenu* menu, BRect frame, bool fixedSize)
{
	fMenu = menu;
	InitMenu(menu);

	fMenuBar = new _BMCMenuBar_(BRect(frame.left + fDivider + 1,
		frame.top + kVMargin, frame.right - 2, frame.bottom - kVMargin),
		fixedSize, this);

	// by default align the menu bar left in the available space
	fMenuBar->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));

	AddChild(fMenuBar);
	fMenuBar->AddItem(menu);

	fMenuBar->SetFont(be_plain_font);
}


void
BMenuField::_ValidateLayoutData()
{
	if (fLayoutData->valid)
		return;

	// cache font height
	font_height& fh = fLayoutData->font_info;
	GetFontHeight(&fh);

	fLayoutData->label_width = (Label() ? ceilf(StringWidth(Label())) : 0);
	fLayoutData->label_height = ceilf(fh.ascent) + ceilf(fh.descent);

	// compute the minimal divider
	float divider = 0;
	if (fLayoutData->label_width > 0)
		divider = fLayoutData->label_width + 5;

	// If we shan't do real layout, we let the current divider take influence.
	if (!(Flags() & B_SUPPORTS_LAYOUT))
		divider = max_c(divider, fDivider);

	// get the minimal (== preferred) menu bar size
	fLayoutData->menu_bar_min = fMenuBar->MinSize();

	// compute our minimal (== preferred) size
	// TODO: The layout is a bit broken. A one pixel wide border is draw around
	// the menu bar to give it it's look. When the view has the focus,
	// additionally a one pixel wide blue frame is drawn around it. In order
	// to be able to easily visually align the menu bar with the text view of
	// a text control, the divider must ignore the focus frame, though. Hence
	// we add one less pixel to our width.
	BSize min(fLayoutData->menu_bar_min);
	min.width += 2 * kVMargin - 1;
	min.height += 2 * kVMargin;

	if (fLayoutData->label_width > 0)
		min.width += fLayoutData->label_width + 5;
	if (fLayoutData->label_height > min.height)
		min.height = fLayoutData->label_height;

	fLayoutData->min = min;

	fLayoutData->valid = true;
}


// #pragma mark -


BMenuField::LabelLayoutItem::LabelLayoutItem(BMenuField* parent)
	: fParent(parent),
	  fFrame()
{
}


bool
BMenuField::LabelLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
BMenuField::LabelLayoutItem::SetVisible(bool visible)
{
	// not allowed
}


BRect
BMenuField::LabelLayoutItem::Frame()
{
	return fFrame;
}


void
BMenuField::LabelLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


BView*
BMenuField::LabelLayoutItem::View()
{
	return fParent;
}


BSize
BMenuField::LabelLayoutItem::BaseMinSize()
{
	fParent->_ValidateLayoutData();

	if (!fParent->Label())
		return BSize(-1, -1);

	return BSize(fParent->fLayoutData->label_width + 5,
		fParent->fLayoutData->label_height);
}


BSize
BMenuField::LabelLayoutItem::BaseMaxSize()
{
	return BaseMinSize();
}


BSize
BMenuField::LabelLayoutItem::BasePreferredSize()
{
	return BaseMinSize();
}


BAlignment
BMenuField::LabelLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


// #pragma mark -


BMenuField::MenuBarLayoutItem::MenuBarLayoutItem(BMenuField* parent)
	: fParent(parent),
	  fFrame()
{
	// by default the part left of the divider shall have an unlimited maximum
	// width
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
}


bool
BMenuField::MenuBarLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
BMenuField::MenuBarLayoutItem::SetVisible(bool visible)
{
	// not allowed
}


BRect
BMenuField::MenuBarLayoutItem::Frame()
{
	return fFrame;
}


void
BMenuField::MenuBarLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


BView*
BMenuField::MenuBarLayoutItem::View()
{
	return fParent;
}


BSize
BMenuField::MenuBarLayoutItem::BaseMinSize()
{
	fParent->_ValidateLayoutData();

	// TODO: Cf. the TODO in _ValidateLayoutData().
	BSize size = fParent->fLayoutData->menu_bar_min;
	size.width += 2 * kVMargin - 1;
	size.height += 2 * kVMargin;

	return size;
}


BSize
BMenuField::MenuBarLayoutItem::BaseMaxSize()
{
	return BaseMinSize();
}


BSize
BMenuField::MenuBarLayoutItem::BasePreferredSize()
{
	return BaseMinSize();
}


BAlignment
BMenuField::MenuBarLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}

