/*
 * Copyright 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <stdlib.h>
#include <string.h>

#include <MenuBar.h>
#include <MenuField.h>
#include <Message.h>
#include <BMCPrivate.h>
#include <Window.h>


static float kVMargin = 2.0f;


BMenuField::BMenuField(BRect frame, const char *name, const char *label,
	BMenu *menu, uint32 resize, uint32 flags)
	: BView(frame, name, resize, flags)
{
	InitObject(label);
	SetFont(be_plain_font);
	fMenu = menu;
	InitMenu(menu);

	frame.OffsetTo(B_ORIGIN);
	fMenuBar = new _BMCMenuBar_(BRect(frame.left + fDivider + 1,
		frame.top + kVMargin, frame.right, frame.bottom - kVMargin),
		false, this);

	AddChild(fMenuBar);
	fMenuBar->AddItem(menu);

	fMenuBar->SetFont(be_plain_font);

	InitObject2();
}


BMenuField::BMenuField(BRect frame, const char *name, const char *label,
	BMenu *menu, bool fixedSize, uint32 resize, uint32 flags)
	: BView(frame, name, resize, flags)
{
	InitObject(label);
	SetFont(be_plain_font);
	fMenu = menu;
	InitMenu(menu);

	fFixedSizeMB = fixedSize;

	frame.OffsetTo(B_ORIGIN);
	fMenuBar = new _BMCMenuBar_(BRect(frame.left + fDivider + 1,
		frame.top + kVMargin, frame.right, frame.bottom - kVMargin),
		fixedSize, this);

	AddChild(fMenuBar);
	fMenuBar->AddItem(menu);

	fMenuBar->SetFont(be_plain_font);

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

//	BMenuItem *item = fMenuBar->ItemAt(0);
//	if (!item)
//		return;
//
//	_BMCItem_ *bmcitem = dynamic_cast<_BMCItem_*>(item);
//	if (!bmcitem)
//		return;
//
//	bool dmark;
//	if (data->FindBool("be:dmark", &dmark))
//		bmcitem->fShowPopUpMarker = dmark;
}


BMenuField::~BMenuField()
{
	free(fLabel);

	status_t dummy;
	if (fMenuTaskID >= 0)
		wait_for_thread(fMenuTaskID, &dummy);
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
	BView::Archive(data, deep);

	if (Label())
		data->AddString("_label", Label());

	if (!IsEnabled())
		data->AddBool("_disable", true);

	data->AddInt32("_align", Alignment());
	data->AddFloat("_divide", Divider());

	if (fFixedSizeMB)
		data->AddBool("be:fixeds", true);

//	BMenuItem *item = fMenuBar->ItemAt(0);
//	if (!item)
//		return B_OK;
//
//	_BMCItem_ *bmcitem = dynamic_cast<_BMCItem_*>(item);
//	if (bmcitem && !bmcitem->fShowPopUpMarker)
//		data->AddBool("be:dmark", false);

	return B_OK;
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

	if (fLabel)
		fStringWidth = StringWidth(fLabel);
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
	if (!IsEnabled() || (where.x > fDivider && !fMenuBar->Frame().Contains(where)))
		return;

	BRect bounds = fMenuBar->ConvertFromParent(Bounds());

	fMenuBar->StartMenuBar(0, false, true, &bounds);

	fMenuTaskID = spawn_thread((thread_func)MenuTask, "_m_task_",
		B_NORMAL_PRIORITY, this);
	if (fMenuTaskID)
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
		Invalidate(); // TODO: use fStringWidth
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

	if (Window()) {
		Invalidate();
		if (fLabel)
			fStringWidth = StringWidth(fLabel);
	}
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

	MenuBar()->MoveBy(-dx, 0.0f);
	MenuBar()->ResizeBy(dx, 0.0f);
}


float
BMenuField::Divider() const
{
	return fDivider;
}


void
BMenuField::ShowPopUpMarker()
{
	// TODO:
}


void
BMenuField::HidePopUpMarker()
{
	// TODO:
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
}


void
BMenuField::GetPreferredSize(float *_width, float *_height)
{
	fMenuBar->GetPreferredSize(_width, _height);

	if (_width) {
		// the width is already the menu bars preferred width
		// add the room we need to draw the shadow and stuff
		*_width += 2 * kVMargin;

		// add the room needed for the label
		float labelWidth = fDivider;
		if (Label()) {
			labelWidth = ceilf(StringWidth(Label()));
			if (labelWidth > 0.0) {
				// add some room between label and menu bar
				labelWidth += 5.0;
			}

			// have divider override the calculated label width
			labelWidth = max_c(labelWidth, fDivider);
		}

		*_width += labelWidth;
	}

	if (_height) {
		// the height is already the menu bars preferred height
		// add the room we need to draw the shadow and stuff
		*_height += 2 * kVMargin;

		// see if our label would fit vertically
		font_height fh;
		GetFontHeight(&fh);
		*_height = max_c(*_height, ceilf(fh.ascent + fh.descent));
	}
}


status_t
BMenuField::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
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
	fStringWidth = 0;
	fEnabled = true;
	fSelected = false;
	fTransition = false;
	fFixedSizeMB = false;
	fMenuTaskID = -1;

	SetLabel(label);

	if (label)
		fDivider = (float)floor(Frame().Width() / 2.0f);
	else
		fDivider = 0;
}


void
BMenuField::InitObject2()
{
	// TODO set filter
	
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	// TODO: fix this calculation
	float height = floorf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading) + 7;

	fMenuBar->ResizeTo(Bounds().Width() - fDivider, height);
}


void
BMenuField::DrawLabel(BRect bounds, BRect update)
{
	font_height fh;
	GetFontHeight(&fh);

	if (Label()) {
		SetLowColor(ViewColor());

		float y = (float)ceil(fh.ascent + fh.descent + fh.leading) + 2.0f;
		float x;

		switch (fAlign) {
			case B_ALIGN_RIGHT:
				x = fDivider - StringWidth(Label()) - 3.0f;
				break;

			case B_ALIGN_CENTER:
				x = fDivider - StringWidth(Label()) / 2.0f;
				break;

			default:
				x = 3.0f;
				break;
		}

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


long
BMenuField::MenuTask(void *arg)
{
	BMenuField *menuField = (BMenuField*)arg;

	if (!menuField->LockLooper())
		return 0;

	menuField->fSelected = true;
	menuField->fTransition = true;
	menuField->Invalidate();

	menuField->UnlockLooper();

	bool tracking;

	do {
		snooze(20000);

		if (!menuField->LockLooper())
			return 0;

		tracking = menuField->fMenuBar->fTracking;

		menuField->UnlockLooper();
	} while (tracking);

	if (!menuField->LockLooper())
		return 0;

	menuField->fSelected = false;
	menuField->fTransition = true;
	menuField->Invalidate();

	menuField->UnlockLooper();
	return 0;
}

