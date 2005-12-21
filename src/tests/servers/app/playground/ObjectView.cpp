// ObjectView.cpp

#include <stdio.h>

#include <Bitmap.h>
#include <Message.h>
#include <Region.h>
#include <Shape.h>
#include <String.h>
#include <Window.h>

#include "States.h"

#include "ObjectView.h"

// constructor
ObjectView::ObjectView(BRect frame, const char* name,
					 uint32 resizeFlags, uint32 flags)
	: BView(frame, name, resizeFlags, flags),
	  fState(NULL),
	  fObjectType(OBJECT_LINE),
	  fStateList(20),
	  fColor((rgb_color){ 0, 80, 255, 100 }),
	  fDrawingMode(B_OP_ALPHA),
	  fFill(false),
	  fPenSize(10.0),
	  fScrolling(false),
	  fInitiatingDrag(false),
	  fLastMousePos(0.0, 0.0)
{

	SetLowColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_1_TINT));

//	BFont font;
//	GetFont(&font);
//	font.SetFamilyAndStyle("Bitstream Vera Serif", "Roman");
//	font.SetSize(20.0);
////	font.SetRotation(6.0);
//	SetFont(&font, B_FONT_FAMILY_AND_STYLE | B_FONT_ROTATION | B_FONT_SIZE);
}

// AttachedToWindow
void
ObjectView::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);
}

// Draw
void
ObjectView::Draw(BRect updateRect)
{
	FillRect(updateRect, B_SOLID_LOW);

	rgb_color noTint = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color shadow = tint_color(noTint, B_DARKEN_2_TINT);
	rgb_color light = tint_color(noTint, B_LIGHTEN_MAX_TINT);

	BRect r(Bounds());

/*	BeginLineArray(4);
	AddLine(BPoint(r.left, r.top),
			BPoint(r.right, r.top), shadow);
	AddLine(BPoint(r.right, r.top + 1),
			BPoint(r.right, r.bottom), light);
	AddLine(BPoint(r.right - 1, r.bottom),
			BPoint(r.left, r.bottom), light);
	AddLine(BPoint(r.left, r.bottom - 1),
			BPoint(r.left, r.top + 1), shadow);
	EndLineArray();*/

	SetHighColor(255, 0, 0, 128);

	const char* message = "Click and drag to draw an object";
	float width = StringWidth(message);

	BPoint p(r.Width() / 2.0 - width / 2.0,
			 r.Height() / 2.0);

Sync();
bigtime_t now = system_time();
	DrawString(message, p);

Sync();
printf("Drawing Text: %lld\n", system_time() - now);

	for (int32 i = 0; State* state = (State*)fStateList.ItemAt(i); i++)
		state->Draw(this);

}

// MouseDown
void
ObjectView::MouseDown(BPoint where)
{
	uint32 buttons;
	int32 clicks;
	Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);
	Window()->CurrentMessage()->FindInt32("clicks", &clicks);
//printf("ObjectView::MouseDown() - clicks: %ld\n", clicks);
	fScrolling = buttons & B_SECONDARY_MOUSE_BUTTON;
	fInitiatingDrag = !fScrolling && (buttons & B_TERTIARY_MOUSE_BUTTON);

	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

	if (fScrolling  || fInitiatingDrag) {
		fLastMousePos = where;
	} else {
		if (!fState)
			AddObject(State::StateFor(fObjectType, fColor, fDrawingMode,
									  fFill, fPenSize));
	
		if (fState) {
//			Invalidate(fState->Bounds());
			fState->MouseDown(where);
//			Invalidate(fState->Bounds());
		}
	}
}

// MouseUp
void
ObjectView::MouseUp(BPoint where)
{
	if (fScrolling) {
		fScrolling = false;
	} else {
		if (fState) {
			fState->MouseUp(where);
		}
	}
}

// MouseMoved
void
ObjectView::MouseMoved(BPoint where, uint32 transit,
					   const BMessage* dragMessage)
{

if (dragMessage) {
//printf("ObjectView::MouseMoved(BPoint(%.1f, %.1f)) - DRAG MESSAGE\n", where.x, where.y);
//Window()->CurrentMessage()->PrintToStream();
} else {
//printf("ObjectView::MouseMoved(BPoint(%.1f, %.1f))\n", where.x, where.y);
}

	if (fScrolling) {
		BPoint offset = fLastMousePos - where;
		ScrollBy(offset.x, offset.y);
		fLastMousePos = where + offset;
	} else if (fInitiatingDrag) {
		BPoint offset = fLastMousePos - where;
		if (sqrtf(offset.x * offset.x + offset.y * offset.y) > 5.0) {
			BMessage dragMessage('drag');
			BBitmap* dragBitmap = new BBitmap(BRect(0, 0, 20, 20), B_RGBA32);
			memset(dragBitmap->Bits(), 128, dragBitmap->BitsLength());
			
			DragMessage(&dragMessage, dragBitmap, B_OP_ALPHA, B_ORIGIN, this);
			fInitiatingDrag = false;
		}
	} else {
		if (fState && fState->IsTracking()) {
			BRect before = fState->Bounds();
	
			fState->MouseMoved(where);
	
			BRect after = fState->Bounds();
			BRect invalid(before | after);
			Invalidate(invalid);
		}
	}
}

// MessageReceived
void
ObjectView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case 'drag':
			printf("ObjectView::MessageReceived() - received drag message\n");
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}

// SetState
void
ObjectView::SetState(State* state)
{
	if (fState != state) {
		if (fState) {
			fState->SetEditing(false);
			Invalidate(fState->Bounds());
		}
	
		fState = state;
	
		if (fState) {
			fState->SetEditing(true);
			Invalidate(fState->Bounds());
		}
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

		BMessage message(MSG_OBJECT_ADDED);
		message.AddPointer("object", state);
		Window()->PostMessage(&message);

		SetState(state);
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

// SetStateDrawingMode
void
ObjectView::SetStateDrawingMode(drawing_mode mode)
{
	if (fDrawingMode != mode) {
		fDrawingMode = mode;
	
		if (fState) {
			fState->SetDrawingMode(fDrawingMode);
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
	
			fState->SetFill(fFill);
	
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
	
			fState->SetPenSize(fPenSize);
	
			BRect after = fState->Bounds();
			BRect invalid(before | after);
			Invalidate(invalid);
		}
	}
}
