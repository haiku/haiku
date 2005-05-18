// ObjectView.cpp

#include <stdio.h>

#include <Message.h>
#include <Window.h>

#include "States.h"

#include "ObjectView.h"

// constructor
ObjectView::ObjectView(BRect frame, const char* name,
					 uint32 resizeFlags, uint32 flags)
	: BView(frame, name, resizeFlags, flags | B_FRAME_EVENTS),
	  fState(NULL),
	  fObjectType(OBJECT_LINE),
	  fStateList(20),
	  fColor((rgb_color){ 0, 80, 255, 100 }),
	  fFill(false),
	  fPenSize(10.0)
{
	rgb_color bg = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_1_TINT);
	SetViewColor(bg);
	SetLowColor(bg);
}

// AttachedToWindow
void
ObjectView::AttachedToWindow()
{
}

// Draw
void
ObjectView::Draw(BRect updateRect)
{
	rgb_color noTint = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color shadow = tint_color(noTint, B_DARKEN_2_TINT);
	rgb_color light = tint_color(noTint, B_LIGHTEN_MAX_TINT);

	BRect r(Bounds());

	BeginLineArray(4);
	AddLine(BPoint(r.left, r.top),
			BPoint(r.right, r.top), shadow);
	AddLine(BPoint(r.right, r.top + 1),
			BPoint(r.right, r.bottom), light);
	AddLine(BPoint(r.right - 1, r.bottom),
			BPoint(r.left, r.bottom), light);
	AddLine(BPoint(r.left, r.bottom - 1),
			BPoint(r.left, r.top + 1), shadow);
	EndLineArray();

	SetHighColor(255, 0, 0, 128);

	const char* message = "Click and drag";
	float width = StringWidth(message);
	BPoint p(r.left + r.Width() / 2.0 - width / 2.0,
			 r.top + r.Height() / 2.0);

	DrawString(message, p);

	message = "to draw an object!";
	width = StringWidth(message);
	p.x = r.left + r.Width() / 2.0 - width / 2.0;
	p.y += 20;

	DrawString(message, p);

	SetDrawingMode(B_OP_ALPHA);
	for (int32 i = 0; State* state = (State*)fStateList.ItemAt(i); i++)
		state->Draw(this);

// TODO: this should not be necessary with proper state stack
// (a new state is pushed before Draw() is called)
SetPenSize(1.0);
SetDrawingMode(B_OP_COPY);
}

// MouseDown
void
ObjectView::MouseDown(BPoint where)
{
	if (!fState)
		SetState(State::StateFor(fObjectType, fColor, fFill, fPenSize));

	if (fState) {
		Invalidate(fState->Bounds());
		fState->MouseDown(where);
		Invalidate(fState->Bounds());
	}
}

// MouseUp
void
ObjectView::MouseUp(BPoint where)
{
	if (fState) {
		fState->MouseUp(where);
	}
}

// MouseMoved
void
ObjectView::MouseMoved(BPoint where, uint32 transit,
					   const BMessage* dragMessage)
{
	if (fState && fState->IsTracking()) {
		BRect before = fState->Bounds();

		fState->MouseMoved(where);

		BRect after = fState->Bounds();
		BRect invalid(before | after);
		Invalidate(invalid);
	}
}

// SetState
void
ObjectView::SetState(State* state)
{
	fState = state;

	if (fState) {
		Invalidate(fState->Bounds());
		AddObject(fState);
	}
}

// SetObjectType
void
ObjectView::SetObjectType(int32 type)
{
	if (type != fObjectType) {
		fObjectType = type;
		SetState(NULL);
	}
}

// AddObject
void
ObjectView::AddObject(State* state)
{
	if (state) {
		fStateList.AddItem((void*)state);
		Window()->PostMessage(MSG_OBJECT_COUNT_CHANGED);
	}
}

// CountObjects
int32
ObjectView::CountObjects() const
{
	return fStateList.CountItems();
}

// MakeEmpty
void
ObjectView::MakeEmpty()
{
	for (int32 i = 0; State* state = (State*)fStateList.ItemAt(i); i++)
		delete state;
	fStateList.MakeEmpty();

	Window()->PostMessage(MSG_OBJECT_COUNT_CHANGED);

	fState = NULL;

	Invalidate();
}

// SetStateColor
void
ObjectView::SetStateColor(rgb_color color)
{
	if (color.red != fColor.red ||
		color.green != fColor.green ||
		color.blue != fColor.blue ||
		color.alpha != fColor.alpha) {

		fColor = color;
	
		if (fState) {
			fState->SetColor(fColor);
			Invalidate(fState->Bounds());
		}
	}
}

// SetStateFill
void
ObjectView::SetStateFill(bool fill)
{
	if (fFill != fill) {
		fFill = fill;
	
		if (fState) {
			BRect before = fState->Bounds();
	
			fState->SetFill(fill);
	
			BRect after = fState->Bounds();
			BRect invalid(before | after);
			Invalidate(invalid);
		}
	}
}

// SetStatePenSize
void
ObjectView::SetStatePenSize(float penSize)
{
	if (fPenSize != penSize) {
		fPenSize = penSize;
	
		if (fState) {
			BRect before = fState->Bounds();
	
			fState->SetPenSize(penSize);
	
			BRect after = fState->Bounds();
			BRect invalid(before | after);
			Invalidate(invalid);
		}
	}
}
