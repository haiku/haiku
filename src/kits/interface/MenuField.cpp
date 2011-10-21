/*
 * Copyright 2001-2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */


#include <MenuField.h>

#include <stdlib.h>
#include <string.h>

#include <AbstractLayoutItem.h>
#include <ControlLook.h>
#include <LayoutUtils.h>
#include <MenuBar.h>
#include <Message.h>
#include <BMCPrivate.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>
#include <binary_compatibility/Support.h>


//#define TRACE_MENU_FIELD
#ifdef TRACE_MENU_FIELD
#	include <FunctionTracer.h>
	static int32 sFunctionDepth = -1;
#	define CALLED(x...)	FunctionTracer _ft("BMenuField", __FUNCTION__, \
							sFunctionDepth)
#	define TRACE(x...)	{ BString _to; \
							_to.Append(' ', (sFunctionDepth + 1) * 2); \
							printf("%s", _to.String()); printf(x); }
#else
#	define CALLED(x...)
#	define TRACE(x...)
#endif


namespace {
	const char* const kFrameField = "BMenuField:layoutItem:frame";
	const char* const kMenuBarItemField = "BMenuField:barItem";
	const char* const kLabelItemField = "BMenuField:labelItem";
}


class BMenuField::LabelLayoutItem : public BAbstractLayoutItem {
public:
								LabelLayoutItem(BMenuField* parent);
								LabelLayoutItem(BMessage* archive);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

			void				SetParent(BMenuField* parent);
	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

private:
			BMenuField*			fParent;
			BRect				fFrame;
};


class BMenuField::MenuBarLayoutItem : public BAbstractLayoutItem {
public:
								MenuBarLayoutItem(BMenuField* parent);
								MenuBarLayoutItem(BMessage* from);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

			void				SetParent(BMenuField* parent);
	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

private:
			BMenuField*			fParent;
			BRect				fFrame;
};


struct BMenuField::LayoutData {
	LayoutData()
		:
		label_layout_item(NULL),
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


BMenuField::BMenuField(BRect frame, const char* name, const char* label,
		BMenu* menu, uint32 resize, uint32 flags)
	:
	BView(frame, name, resize, flags)
{
	CALLED();

	TRACE("frame.width: %.2f, height: %.2f\n", frame.Width(), frame.Height());

	InitObject(label);

	frame.OffsetTo(B_ORIGIN);
	_InitMenuBar(menu, frame, false);

	InitObject2();
}


BMenuField::BMenuField(BRect frame, const char* name, const char* label,
		BMenu* menu, bool fixedSize, uint32 resize, uint32 flags)
	:
	BView(frame, name, resize, flags)
{
	InitObject(label);

	fFixedSizeMB = fixedSize;

	frame.OffsetTo(B_ORIGIN);
	_InitMenuBar(menu, frame, fixedSize);

	InitObject2();
}


BMenuField::BMenuField(const char* name, const char* label, BMenu* menu,
		uint32 flags)
	:
	BView(name, flags | B_FRAME_EVENTS)
{
	InitObject(label);

	_InitMenuBar(menu, BRect(0, 0, 100, 15), true);

	InitObject2();
}


BMenuField::BMenuField(const char* label, BMenu* menu, uint32 flags)
	:
	BView(NULL, flags | B_FRAME_EVENTS)
{
	InitObject(label);

	_InitMenuBar(menu, BRect(0, 0, 100, 15), true);

	InitObject2();
}


//! Copy&Paste error, should be removed at some point (already private)
BMenuField::BMenuField(const char* name, const char* label, BMenu* menu,
		BMessage* message, uint32 flags)
	:
	BView(name, flags | B_FRAME_EVENTS)
{
	InitObject(label);

	_InitMenuBar(menu, BRect(0, 0, 100, 15), true);

	InitObject2();
}


//! Copy&Paste error, should be removed at some point (already private)
BMenuField::BMenuField(const char* label, BMenu* menu, BMessage* message)
	:
	BView(NULL, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS)
{
	InitObject(label);

	_InitMenuBar(menu, BRect(0, 0, 100, 15), true);

	InitObject2();
}


BMenuField::BMenuField(BMessage* data)
	:
	BView(BUnarchiver::PrepareArchive(data))
{
	BUnarchiver unarchiver(data);
	const char* label = NULL;
	data->FindString("_label", &label);

	InitObject(label);

	data->FindFloat("_divide", &fDivider);

	int32 align;
	if (data->FindInt32("_align", &align) == B_OK)
		SetAlignment((alignment)align);

	if (!BUnarchiver::IsArchiveManaged(data))
		_InitMenuBar(data);
	unarchiver.Finish();
}


BMenuField::~BMenuField()
{
	free(fLabel);

	status_t dummy;
	if (fMenuTaskID >= 0)
		wait_for_thread(fMenuTaskID, &dummy);

	delete fLayoutData;
}


BArchivable*
BMenuField::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BMenuField"))
		return new BMenuField(data);

	return NULL;
}


status_t
BMenuField::Archive(BMessage* data, bool deep) const
{
	BArchiver archiver(data);
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
	if (_BMCMenuBar_* menuBar = dynamic_cast<_BMCMenuBar_*>(fMenuBar))
		dmark = menuBar->IsPopUpMarkerShown();

	data->AddBool("be:dmark", dmark);

	return archiver.Finish(ret);
}


status_t
BMenuField::AllArchived(BMessage* into) const
{
	status_t err;
	if ((err = BView::AllArchived(into)) != B_OK)
		return err;

	BArchiver archiver(into);

	BArchivable* menuBarItem = fLayoutData->menu_bar_layout_item;
	if (archiver.IsArchived(menuBarItem))
		err = archiver.AddArchivable(kMenuBarItemField, menuBarItem);

	if (err != B_OK)
		return err;

	BArchivable* labelBarItem = fLayoutData->label_layout_item;
	if (archiver.IsArchived(labelBarItem))
		err = archiver.AddArchivable(kLabelItemField, labelBarItem);

	return err;
}


status_t
BMenuField::AllUnarchived(const BMessage* from)
{
	BUnarchiver unarchiver(from);

	status_t err = B_OK;
	if ((err = BView::AllUnarchived(from)) != B_OK)
		return err;

	_InitMenuBar(from);

	if (unarchiver.IsInstantiated(kMenuBarItemField)) {
		MenuBarLayoutItem*& menuItem = fLayoutData->menu_bar_layout_item;
		err = unarchiver.FindObject(kMenuBarItemField,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP, menuItem);

		if (err == B_OK)
			menuItem->SetParent(this);
		else
			return err;
	}

	if (unarchiver.IsInstantiated(kLabelItemField)) {
		LabelLayoutItem*& labelItem = fLayoutData->label_layout_item;
		err = unarchiver.FindObject(kLabelItemField,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP, labelItem);

		if (err == B_OK)
			labelItem->SetParent(this);
	}

	return err;
}


void
BMenuField::Draw(BRect update)
{
	BRect bounds(Bounds());
	bool active = IsFocus() && Window()->IsActive();

	DrawLabel(bounds, update);

	BRect frame(fMenuBar->Frame());

	if (be_control_look != NULL) {
		frame.InsetBy(-kVMargin, -kVMargin);
		rgb_color base = fMenuBar->LowColor();
		rgb_color background = LowColor();
		uint32 flags = 0;
		if (!fMenuBar->IsEnabled())
			flags |= BControlLook::B_DISABLED;
		if (active)
			flags |= BControlLook::B_FOCUSED;
		be_control_look->DrawMenuFieldFrame(this, frame, update, base,
			background, flags);
		return;
	}

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
	CALLED();

	BView* parent = Parent();
	if (parent != NULL) {
		// inherit the color from parent
		rgb_color color = parent->ViewColor();
		if (color == B_TRANSPARENT_COLOR)
			color = ui_color(B_PANEL_BACKGROUND_COLOR);

		SetViewColor(color);
		SetLowColor(color);
	}
}


void
BMenuField::AllAttached()
{
	CALLED();

	TRACE("width: %.2f, height: %.2f\n", Frame().Width(), Frame().Height());

	ResizeTo(Bounds().Width(),
		fMenuBar->Bounds().Height() + kVMargin + kVMargin);

	TRACE("width: %.2f, height: %.2f\n", Frame().Width(), Frame().Height());
}


void
BMenuField::MouseDown(BPoint where)
{
	if (!fMenuBar->Frame().Contains(where))
		return;

	if (!fMenuBar->IsEnabled())
		return;

	BRect bounds = fMenuBar->ConvertFromParent(Bounds());

	fMenuBar->StartMenuBar(-1, false, true, &bounds);

	fMenuTaskID = spawn_thread((thread_func)_thread_entry,
			 	"_m_task_", B_NORMAL_PRIORITY, this);
	if (fMenuTaskID >= 0)
		resume_thread(fMenuTaskID);
}


void
BMenuField::KeyDown(const char* bytes, int32 numBytes)
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
BMenuField::MessageReceived(BMessage* msg)
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
BMenuField::MouseMoved(BPoint point, uint32 code, const BMessage* message)
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


BMenu*
BMenuField::Menu() const
{
	return fMenu;
}


BMenuBar*
BMenuField::MenuBar() const
{
	return fMenuBar;
}


BMenuItem*
BMenuField::MenuItem() const
{
	return fMenuBar->ItemAt(0);
}


void
BMenuField::SetLabel(const char* label)
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


const char*
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

		fMenuBar->MoveTo(_MenuBarOffset(), kVMargin);

		if (fFixedSizeMB)
			fMenuBar->ResizeTo(_MenuBarWidth(), dirty.Height());

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
	if (_BMCMenuBar_* menuBar = dynamic_cast<_BMCMenuBar_*>(fMenuBar)) {
		menuBar->TogglePopUpMarker(true);
		menuBar->Invalidate();
	}
}


void
BMenuField::HidePopUpMarker()
{
	if (_BMCMenuBar_* menuBar = dynamic_cast<_BMCMenuBar_*>(fMenuBar)) {
		menuBar->TogglePopUpMarker(false);
		menuBar->Invalidate();
	}
}


BHandler*
BMenuField::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	return BView::ResolveSpecifier(message, index, specifier, form, property);
}


status_t
BMenuField::GetSupportedSuites(BMessage* data)
{
	return BView::GetSupportedSuites(data);
}


void
BMenuField::ResizeToPreferred()
{
	CALLED();

	TRACE("fMenuBar->Frame().width: %.2f, height: %.2f\n",
		fMenuBar->Frame().Width(), fMenuBar->Frame().Height());

	fMenuBar->ResizeToPreferred();

	TRACE("fMenuBar->Frame().width: %.2f, height: %.2f\n",
		fMenuBar->Frame().Width(), fMenuBar->Frame().Height());

	BView::ResizeToPreferred();

	if (fFixedSizeMB) {
		// we have let the menubar resize itself, but
		// in fixed size mode, the menubar is supposed to
		// be at the right end of the view always. Since
		// the menu bar is in follow left/right mode then,
		// resizing ourselfs might have caused the menubar
		// to be outside now
		fMenuBar->ResizeTo(_MenuBarWidth(), fMenuBar->Frame().Height());
	}
}


void
BMenuField::GetPreferredSize(float* _width, float* _height)
{
	CALLED();

	_ValidateLayoutData();

	if (_width)
		*_width = fLayoutData->min.width;

	if (_height)
		*_height = fLayoutData->min.height;
}


BSize
BMenuField::MinSize()
{
	CALLED();

	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), fLayoutData->min);
}


BSize
BMenuField::MaxSize()
{
	CALLED();

	_ValidateLayoutData();

	BSize max = fLayoutData->min;
	max.width = B_SIZE_UNLIMITED;

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), max);
}


BSize
BMenuField::PreferredSize()
{
	CALLED();

	_ValidateLayoutData();
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), fLayoutData->min);
}


void
BMenuField::InvalidateLayout(bool descendants)
{
	CALLED();

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
	if (!fLayoutData->menu_bar_layout_item) {
		// align the menu bar in the full available space
		fMenuBar->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_VERTICAL_UNSET));
		fLayoutData->menu_bar_layout_item = new MenuBarLayoutItem(this);
	}
	return fLayoutData->menu_bar_layout_item;
}


status_t
BMenuField::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BMenuField::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BMenuField::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BMenuField::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BMenuField::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BMenuField::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BMenuField::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BMenuField::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BMenuField::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BMenuField::DoLayout();
			return B_OK;
		}
		case PERFORM_CODE_ALL_UNARCHIVED:
		{
			perform_data_all_unarchived* data
				= (perform_data_all_unarchived*)_data;

			data->return_value = BMenuField::AllUnarchived(data->archive);
			return B_OK;
		}
		case PERFORM_CODE_ALL_ARCHIVED:
		{
			perform_data_all_archived* data
				= (perform_data_all_archived*)_data;

			data->return_value = BMenuField::AllArchived(data->archive);
			return B_OK;
		}
	}

	return BView::Perform(code, _data);
}


void
BMenuField::DoLayout()
{
	// Bail out, if we shan't do layout.
	if (!(Flags() & B_SUPPORTS_LAYOUT))
		return;

	CALLED();

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
	BRect menuBarFrame(divider + kVMargin, kVMargin, size.width - kVMargin,
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


void
BMenuField::InitObject(const char* label)
{
	CALLED();

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
}


void
BMenuField::InitObject2()
{
	CALLED();

	float height;
	fMenuBar->GetPreferredSize(NULL, &height);
	fMenuBar->ResizeTo(_MenuBarWidth(), height);

	TRACE("frame(%.1f, %.1f, %.1f, %.1f) (%.2f, %.2f)\n",
		fMenuBar->Frame().left, fMenuBar->Frame().top,
		fMenuBar->Frame().right, fMenuBar->Frame().bottom,
		fMenuBar->Frame().Width(), fMenuBar->Frame().Height());

	fMenuBar->AddFilter(new _BMCFilter_(this, B_MOUSE_DOWN));
}


void
BMenuField::DrawLabel(BRect bounds, BRect update)
{
	CALLED();

	_ValidateLayoutData();
	font_height& fh = fLayoutData->font_info;

	if (Label()) {
		SetLowColor(ViewColor());

		// horizontal alignment
		float x;
		switch (fAlign) {
			case B_ALIGN_RIGHT:
				x = fDivider - fLayoutData->label_width - 3.0;
				break;

			case B_ALIGN_CENTER:
				x = fDivider - fLayoutData->label_width / 2.0;
				break;

			default:
				x = 0.0;
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
BMenuField::InitMenu(BMenu* menu)
{
	menu->SetFont(be_plain_font);

	int32 index = 0;
	BMenu* subMenu;

	while ((subMenu = menu->SubmenuAt(index++)) != NULL)
		InitMenu(subMenu);
}


/*static*/ int32
BMenuField::_thread_entry(void* arg)
{
	return static_cast<BMenuField*>(arg)->_MenuTask();
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
	CALLED();

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
	CALLED();

	fMenu = menu;
	InitMenu(menu);

	if ((Flags() & B_SUPPORTS_LAYOUT)) {
		fMenuBar = new _BMCMenuBar_(fixedSize, this);
	} else {
		frame.left = _MenuBarOffset();
		frame.top = kVMargin;
		frame.right -= kVMargin;
		frame.bottom -= kVMargin;

		TRACE("frame(%.1f, %.1f, %.1f, %.1f) (%.2f, %.2f)\n",
			frame.left, frame.top, frame.right, frame.bottom,
			frame.Width(), frame.Height());

		fMenuBar = new _BMCMenuBar_(frame, fixedSize, this);
	}

	if (fixedSize) {
		// align the menu bar in the full available space
		fMenuBar->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_VERTICAL_UNSET));
	} else {
		// align the menu bar left in the available space
		fMenuBar->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
			B_ALIGN_VERTICAL_UNSET));
	}

	AddChild(fMenuBar);
	fMenuBar->AddItem(menu);

	fMenuBar->SetFont(be_plain_font);
}


void
BMenuField::_InitMenuBar(const BMessage* archive)
{
	bool fixed;
	if (archive->FindBool("be:fixeds", &fixed) == B_OK)
		fFixedSizeMB = fixed;

	fMenuBar = (BMenuBar*)FindView("_mc_mb_");
	if (!fMenuBar) {
		_InitMenuBar(new BMenu(""), BRect(0, 0, 100, 15), fFixedSizeMB);
		InitObject2();
	} else {
		fMenuBar->AddFilter(new _BMCFilter_(this, B_MOUSE_DOWN));
			// this is normally done in InitObject2()
	}

	fMenu = fMenuBar->SubmenuAt(0);

	bool disable;
	if (archive->FindBool("_disable", &disable) == B_OK)
		SetEnabled(!disable);

	bool dmark = false;
	archive->FindBool("be:dmark", &dmark);
	if (_BMCMenuBar_* menuBar = dynamic_cast<_BMCMenuBar_*>(fMenuBar))
		menuBar->TogglePopUpMarker(dmark);
}


void
BMenuField::_ValidateLayoutData()
{
	CALLED();

	if (fLayoutData->valid)
		return;

	// cache font height
	font_height& fh = fLayoutData->font_info;
	GetFontHeight(&fh);

	if (Label() != NULL) {
		fLayoutData->label_width = ceilf(StringWidth(Label()));
		fLayoutData->label_height = ceilf(fh.ascent) + ceilf(fh.descent);
	} else {
		fLayoutData->label_width = 0;
		fLayoutData->label_height = 0;
	}

	// compute the minimal divider
	float divider = 0;
	if (fLayoutData->label_width > 0)
		divider = fLayoutData->label_width + 5;

	// If we shan't do real layout, we let the current divider take influence.
	if (!(Flags() & B_SUPPORTS_LAYOUT))
		divider = max_c(divider, fDivider);

	// get the minimal (== preferred) menu bar size
	// TODO: BMenu::MinSize() is using the ResizeMode() to decide the
	// minimum width. If the mode is B_FOLLOW_LEFT_RIGHT, it will use the
	// parent's frame width or window's frame width. So at least the returned
	// size is wrong, but apparantly it doesn't have much bad effect.
	fLayoutData->menu_bar_min = fMenuBar->MinSize();

	TRACE("menu bar min width: %.2f\n", fLayoutData->menu_bar_min.width);

	// compute our minimal (== preferred) size
	BSize min(fLayoutData->menu_bar_min);
	min.width += 2 * kVMargin;
	min.height += 2 * kVMargin;

	if (divider > 0)
		min.width += divider;
	if (fLayoutData->label_height > min.height)
		min.height = fLayoutData->label_height;

	fLayoutData->min = min;

	fLayoutData->valid = true;
	ResetLayoutInvalidation();

	TRACE("width: %.2f, height: %.2f\n", min.width, min.height);
}


float
BMenuField::_MenuBarOffset() const
{
	return max_c(kVMargin, fDivider + kVMargin);
}


float
BMenuField::_MenuBarWidth() const
{
	return Bounds().Width() - (_MenuBarOffset() + kVMargin);
}


// #pragma mark -


BMenuField::LabelLayoutItem::LabelLayoutItem(BMenuField* parent)
	:
	fParent(parent),
	fFrame()
{
}


BMenuField::LabelLayoutItem::LabelLayoutItem(BMessage* from)
	:
	BAbstractLayoutItem(from),
	fParent(NULL),
	fFrame()
{
	from->FindRect(kFrameField, &fFrame);
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


void
BMenuField::LabelLayoutItem::SetParent(BMenuField* parent)
{
	fParent = parent;
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


status_t
BMenuField::LabelLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BAbstractLayoutItem::Archive(into, deep);

	if (err == B_OK)
		err = into->AddRect(kFrameField, fFrame);

	return archiver.Finish(err);
}


BArchivable*
BMenuField::LabelLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BMenuField::LabelLayoutItem"))
		return new LabelLayoutItem(from);
	return NULL;
}


// #pragma mark -


BMenuField::MenuBarLayoutItem::MenuBarLayoutItem(BMenuField* parent)
	:
	fParent(parent),
	fFrame()
{
	// by default the part right of the divider shall have an unlimited maximum
	// width
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
}


BMenuField::MenuBarLayoutItem::MenuBarLayoutItem(BMessage* from)
	:
	BAbstractLayoutItem(from),
	fParent(NULL),
	fFrame()
{
	from->FindRect(kFrameField, &fFrame);
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


void
BMenuField::MenuBarLayoutItem::SetParent(BMenuField* parent)
{
	fParent = parent;
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

	BSize size = fParent->fLayoutData->menu_bar_min;
	size.width += 2 * kVMargin;
	size.height += 2 * kVMargin;

	return size;
}


BSize
BMenuField::MenuBarLayoutItem::BaseMaxSize()
{
	BSize size(BaseMinSize());
	size.width = B_SIZE_UNLIMITED;
	return size;
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


status_t
BMenuField::MenuBarLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BAbstractLayoutItem::Archive(into, deep);

	if (err == B_OK)
		err = into->AddRect(kFrameField, fFrame);

	return archiver.Finish(err);
}


BArchivable*
BMenuField::MenuBarLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BMenuField::MenuBarLayoutItem"))
		return new MenuBarLayoutItem(from);
	return NULL;
}

