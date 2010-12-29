/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "StyleView.h"

#include <new>

#if __HAIKU__
# include "GridLayout.h"
# include "GroupLayout.h"
# include "SpaceLayoutItem.h"
#endif
#include <Catalog.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Window.h>

#include "CommandStack.h"
#include "CurrentColor.h"
#include "GradientTransformable.h"
#include "GradientControl.h"
#include "SetColorCommand.h"
#include "SetGradientCommand.h"
#include "Style.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Icon-O-Matic-StyleTypes"


using std::nothrow;

enum {
	MSG_SET_COLOR			= 'stcl',

	MSG_SET_STYLE_TYPE		= 'stst',
	MSG_SET_GRADIENT_TYPE	= 'stgt',
};

enum {
	STYLE_TYPE_COLOR		= 0,
	STYLE_TYPE_GRADIENT,
};

// constructor
StyleView::StyleView(BRect frame)
	:
#ifdef __HAIKU__
	BView("style view", 0),
#else
	BView(frame, "style view", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_FRAME_EVENTS),
#endif
	fCommandStack(NULL),
	fCurrentColor(NULL),
	fStyle(NULL),
	fGradient(NULL),
	fIgnoreCurrentColorNotifications(false),
	fIgnoreControlGradientNotifications(false),
	fPreviousBounds(frame.OffsetToCopy(B_ORIGIN))
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// style type
	BMenu* menu = new BPopUpMenu(B_TRANSLATE("<unavailable>"));
	BMessage* message = new BMessage(MSG_SET_STYLE_TYPE);
	message->AddInt32("type", STYLE_TYPE_COLOR);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Color"), message));
	message = new BMessage(MSG_SET_STYLE_TYPE);
	message->AddInt32("type", STYLE_TYPE_GRADIENT);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Gradient"), message));

#ifdef __HAIKU__
	BGridLayout* layout = new BGridLayout(5, 5);
	SetLayout(layout);

	fStyleType = new BMenuField(B_TRANSLATE("Style type"), menu, NULL);

#else
	frame.OffsetTo(B_ORIGIN);
	frame.InsetBy(5, 5);
	frame.bottom = frame.top + 15;

	fStyleType = new BMenuField(frame, "style type", B_TRANSLATE("Style type"),
		menu, true);
	AddChild(fStyleType);

	float width;
	float height;
	fStyleType->MenuBar()->GetPreferredSize(&width, &height);
	fStyleType->MenuBar()->ResizeTo(width, height);
	fStyleType->ResizeTo(frame.Width(), height + 6);
	fStyleType->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
	fStyleType->MenuBar()->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
#endif // __HAIKU__

	// gradient type
	menu = new BPopUpMenu(B_TRANSLATE("<unavailable>"));
	message = new BMessage(MSG_SET_GRADIENT_TYPE);
	message->AddInt32("type", GRADIENT_LINEAR);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Linear"), message));
	message = new BMessage(MSG_SET_GRADIENT_TYPE);
	message->AddInt32("type", GRADIENT_CIRCULAR);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Radial"), message));
	message = new BMessage(MSG_SET_GRADIENT_TYPE);
	message->AddInt32("type", GRADIENT_DIAMOND);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Diamond"), message));
	message = new BMessage(MSG_SET_GRADIENT_TYPE);
	message->AddInt32("type", GRADIENT_CONIC);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Conic"), message));

#if __HAIKU__
	fGradientType = new BMenuField(B_TRANSLATE("Gradient type"), menu, NULL);
	fGradientControl = new GradientControl(new BMessage(MSG_SET_COLOR), this);

	layout->AddItem(BSpaceLayoutItem::CreateVerticalStrut(3), 0, 0, 4);
	layout->AddItem(BSpaceLayoutItem::CreateHorizontalStrut(3), 0, 1, 1, 3);

	layout->AddItem(fStyleType->CreateLabelLayoutItem(), 1, 1);
	layout->AddItem(fStyleType->CreateMenuBarLayoutItem(), 2, 1);

	layout->AddItem(fGradientType->CreateLabelLayoutItem(), 1, 2);
	layout->AddItem(fGradientType->CreateMenuBarLayoutItem(), 2, 2);

	layout->AddView(fGradientControl, 1, 3, 2);

	layout->AddItem(BSpaceLayoutItem::CreateHorizontalStrut(3), 3, 1, 1, 3);
	layout->AddItem(BSpaceLayoutItem::CreateVerticalStrut(3), 0, 4, 4);

#else // !__HAIKU__
	frame.OffsetBy(0, fStyleType->Frame().Height() + 6);
	fGradientType = new BMenuField(frame, "gradient type", 
		B_TRANSLATE("Gradient type"), menu, true);
	AddChild(fGradientType);

	fGradientType->MenuBar()->GetPreferredSize(&width, &height);
	fGradientType->MenuBar()->ResizeTo(width, height);
	fGradientType->ResizeTo(frame.Width(), height + 6);
	fGradientType->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
	fGradientType->MenuBar()->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);

	// create gradient control
	frame.top = fGradientType->Frame().bottom + 8;
	frame.right = Bounds().right - 5;
	fGradientControl = new GradientControl(new BMessage(MSG_SET_COLOR),
										   this);

	width = frame.Width();
	height = max_c(fGradientControl->Frame().Height(), 30);

	fGradientControl->ResizeTo(width, height);
	fGradientControl->FrameResized(width, height);
	fGradientControl->MoveTo(frame.left, frame.top);
	fGradientControl->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);

	AddChild(fGradientControl);

	// align label divider
	float divider = fGradientType->StringWidth(fGradientType->Label());
	divider = max_c(divider, fStyleType->StringWidth(fStyleType->Label()));
	fGradientType->SetDivider(divider + 8);
	fStyleType->SetDivider(divider + 8);

#endif // __HAIKU__

	fStyleType->SetEnabled(false);
	fGradientType->SetEnabled(false);
	fGradientControl->SetEnabled(false);
	fGradientControl->Gradient()->AddObserver(this);
}

// destructor
StyleView::~StyleView()
{
	SetStyle(NULL);
	SetCurrentColor(NULL);
	fGradientControl->Gradient()->RemoveObserver(this);
}

// AttachedToWindow
void
StyleView::AttachedToWindow()
{
	fStyleType->Menu()->SetTargetForItems(this);
	fGradientType->Menu()->SetTargetForItems(this);
}

// FrameResized
void
StyleView::FrameResized(float width, float height)
{
	BRect bounds = Bounds();

#ifndef __HAIKU__
	// Grrr, babysit the menubars...
	BRect dirty = bounds;
	dirty.left = min_c(bounds.right, fPreviousBounds.right) - 25;
	fStyleType->ConvertFromParent(&dirty);
	fStyleType->MenuBar()->ConvertFromParent(&dirty);
	fStyleType->MenuBar()->Invalidate(dirty);
	fGradientType->MenuBar()->Invalidate(dirty);
#endif // !__HAIKU__

	fPreviousBounds = bounds;
}

// MessageReceived
void
StyleView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SET_STYLE_TYPE: {
			int32 type;
			if (message->FindInt32("type", &type) == B_OK)
				_SetStyleType(type);
			break;
		}
		case MSG_SET_GRADIENT_TYPE: {
			int32 type;
			if (message->FindInt32("type", &type) == B_OK)
				_SetGradientType(type);
			break;
		}
		case MSG_SET_COLOR:
		case MSG_GRADIENT_CONTROL_FOCUS_CHANGED:
			_TransferGradientStopColor();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}

#if __HAIKU__

// MinSize
BSize
StyleView::MinSize()
{
	BSize minSize = BView::MinSize();
	minSize.width = fGradientControl->MinSize().width + 10;
	return minSize;
}

#endif // __HAIKU__

// #pragma mark -

// ObjectChanged
void
StyleView::ObjectChanged(const Observable* object)
{
	if (!fStyle)
		return;

	Gradient* controlGradient = fGradientControl->Gradient();

	// NOTE: it is important to compare the gradients
	// before assignment, or we will get into an endless loop
	if (object == controlGradient) {
		if (!fGradient)
			return;
		if (fIgnoreControlGradientNotifications)
			return;
		fIgnoreControlGradientNotifications = true;

		if (!fGradient->ColorStepsAreEqual(*controlGradient)) {
			// Make sure we never apply the transformation from the control
			// gradient to the style gradient. Setting this here would cause to
			// re-enter ObjectChanged(), which is prevented to cause harm via
			// fIgnoreControlGradientNotifications.
			controlGradient->SetTransform(*fGradient);
			if (fCommandStack) {
				fCommandStack->Perform(
					new (nothrow) SetGradientCommand(
						fStyle, controlGradient));
			} else {
				*fGradient = *controlGradient;
			}
			// transfer the current gradient color to the current color
			_TransferGradientStopColor();
		}

		fIgnoreControlGradientNotifications = false;
	} else if (object == fGradient) {
		if (!fGradient->ColorStepsAreEqual(*controlGradient)) {
			fGradientControl->SetGradient(fGradient);
			_MarkType(fGradientType->Menu(), fGradient->Type());
			// transfer the current gradient color to the current color
			_TransferGradientStopColor();
		}
	} else if (object == fStyle) {
		// maybe the gradient was added or removed
		// or the color changed
		_SetGradient(fStyle->Gradient(), false, true);
		if (fCurrentColor && !fStyle->Gradient())
			fCurrentColor->SetColor(fStyle->Color());
	} else if (object == fCurrentColor) {
		// NOTE: because of imprecisions in converting
		// RGB<->HSV, the current color can be different
		// even if we just set it to the current
		// gradient stop color, that's why we ignore
		// notifications caused by this situation
		if (!fIgnoreCurrentColorNotifications)
			_AdoptCurrentColor(fCurrentColor->Color());
	}
}

// #pragma mark -

// StyleView
void
StyleView::SetStyle(Style* style)
{
	if (fStyle == style)
		return;

	if (fStyle) {
		fStyle->RemoveObserver(this);
		fStyle->Release();
	}

	fStyle = style;

	Gradient* gradient = NULL;
	if (fStyle) {
		fStyle->Acquire();
		fStyle->AddObserver(this);
		gradient = fStyle->Gradient();

		if (fCurrentColor && !gradient)
			fCurrentColor->SetColor(fStyle->Color());

		fStyleType->SetEnabled(true);
	} else
		fStyleType->SetEnabled(false);

	_SetGradient(gradient, true);
}

// SetCommandStack
void
StyleView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}

// SetCurrentColor
void
StyleView::SetCurrentColor(CurrentColor* color)
{
	if (fCurrentColor == color)
		return;

	if (fCurrentColor)
		fCurrentColor->RemoveObserver(this);

	fCurrentColor = color;

	if (fCurrentColor)
		fCurrentColor->AddObserver(this);
}

// #pragma mark -

// _SetGradient
void
StyleView::_SetGradient(Gradient* gradient, bool forceControlUpdate, bool sendMessage)
{
	if (!forceControlUpdate && fGradient == gradient)
		return;

	if (fGradient)
		fGradient->RemoveObserver(this);

	fGradient = gradient;

	if (fGradient)
		fGradient->AddObserver(this);

	if (fGradient) {
		fGradientControl->SetEnabled(true);
		fGradientControl->SetGradient(fGradient);
		fGradientType->SetEnabled(true);
		_MarkType(fStyleType->Menu(), STYLE_TYPE_GRADIENT);
		_MarkType(fGradientType->Menu(), fGradient->Type());
	} else {
		fGradientControl->SetEnabled(false);
		fGradientType->SetEnabled(false);
		_MarkType(fStyleType->Menu(), STYLE_TYPE_COLOR);
		_MarkType(fGradientType->Menu(), -1);
	}

	if (sendMessage) {
		BMessage message(MSG_STYLE_TYPE_CHANGED);
		message.AddPointer("style", fStyle);
		Window()->PostMessage(&message);
	}
}

// _MarkType
void
StyleView::_MarkType(BMenu* menu, int32 type) const
{
	for (int32 i = 0; BMenuItem* item = menu->ItemAt(i); i++) {
		BMessage* message = item->Message();
		int32 t;
		if (message->FindInt32("type", &t) == B_OK && t == type) {
			if (!item->IsMarked())
				item->SetMarked(true);
			return;
		}
	}
}

// _SetStyleType
void
StyleView::_SetStyleType(int32 type)
{
	if (!fStyle)
		return;

	if (type == STYLE_TYPE_COLOR) {
		if (fCommandStack) {
			fCommandStack->Perform(
				new (nothrow) SetGradientCommand(fStyle, NULL));
		} else {
			fStyle->SetGradient(NULL);
		}
	} else if (type == STYLE_TYPE_GRADIENT) {
		if (fCommandStack) {
			Gradient gradient(true);
			gradient.AddColor(fStyle->Color(), 0);
			gradient.AddColor(fStyle->Color(), 1);
			fCommandStack->Perform(
				new (nothrow) SetGradientCommand(fStyle, &gradient));
		} else {
			fStyle->SetGradient(fGradientControl->Gradient());
		}
	}
}

// _SetGradientType
void
StyleView::_SetGradientType(int32 type)
{
	fGradientControl->Gradient()->SetType((gradients_type)type);
}

// _AdoptCurrentColor
void
StyleView::_AdoptCurrentColor(rgb_color color)
{
	if (!fStyle)
		return;

	if (fGradient) {
		// set the focused gradient color stop
		if (fGradientControl->IsFocus()) {
			fGradientControl->SetCurrentStop(color);
		}
	} else {
		if (fCommandStack) {
			fCommandStack->Perform(
				new (nothrow) SetColorCommand(fStyle, color));
		} else {
			fStyle->SetColor(color);
		}
	}
}

// _TransferGradientStopColor
void
StyleView::_TransferGradientStopColor()
{
	if (fCurrentColor && fGradientControl->IsFocus()) {
		rgb_color color;
		if (fGradientControl->GetCurrentStop(&color)) {
			fIgnoreCurrentColorNotifications = true;
			fCurrentColor->SetColor(color);
			fIgnoreCurrentColorNotifications = false;
		}
	}
}

