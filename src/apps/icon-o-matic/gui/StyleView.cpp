/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "StyleView.h"

#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>

#include "CommandStack.h"
#include "Gradient.h"
#include "GradientControl.h"
#include "SetGradientCommand.h"
#include "Style.h"

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
	: BView(frame, "style view", B_FOLLOW_LEFT | B_FOLLOW_TOP, 0),
	  fCommandStack(NULL),
	  fStyle(NULL),
	  fGradient(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	frame.OffsetTo(B_ORIGIN);
	frame.InsetBy(5, 5);
	frame.bottom = frame.top + 15;

	// style type
	BMenu* menu = new BPopUpMenu("<unavailable>");
	BMessage* message = new BMessage(MSG_SET_STYLE_TYPE);
	message->AddInt32("type", STYLE_TYPE_COLOR);
	menu->AddItem(new BMenuItem("Color", message));
	message = new BMessage(MSG_SET_STYLE_TYPE);
	message->AddInt32("type", STYLE_TYPE_GRADIENT);
	menu->AddItem(new BMenuItem("Gradient", message));
	fStyleType = new BMenuField(frame, "style type", "Style Type",
								menu, true);
	AddChild(fStyleType);

	float width;
	float height;
	fStyleType->MenuBar()->GetPreferredSize(&width, &height);
	fStyleType->MenuBar()->ResizeTo(width, height);
	fStyleType->ResizeTo(frame.Width(), height + 6);


	// gradient type
	menu = new BPopUpMenu("<unavailable>");
	message = new BMessage(MSG_SET_GRADIENT_TYPE);
	message->AddInt32("type", GRADIENT_LINEAR);
	menu->AddItem(new BMenuItem("Linear", message));
	message = new BMessage(MSG_SET_GRADIENT_TYPE);
	message->AddInt32("type", GRADIENT_CIRCULAR);
	menu->AddItem(new BMenuItem("Radial", message));
	message = new BMessage(MSG_SET_GRADIENT_TYPE);
	message->AddInt32("type", GRADIENT_DIAMONT);
	menu->AddItem(new BMenuItem("Diamont", message));
	message = new BMessage(MSG_SET_GRADIENT_TYPE);
	message->AddInt32("type", GRADIENT_CONIC);
	menu->AddItem(new BMenuItem("Conic", message));

	frame.OffsetBy(0, fStyleType->Frame().Height() + 6);
	fGradientType = new BMenuField(frame, "gradient type", "Gradient Type",
								   menu, true);
	AddChild(fGradientType);

	fGradientType->MenuBar()->GetPreferredSize(&width, &height);
	fGradientType->MenuBar()->ResizeTo(width, height);
	fGradientType->ResizeTo(frame.Width(), height + 6);

	// create gradient control
	frame.top = fGradientType->Frame().bottom + 8;
	frame.right = Bounds().right - 5;
	fGradientControl = new GradientControl(new BMessage(MSG_SET_COLOR),
										   this);

	width = max_c(fGradientControl->Frame().Width(), frame.Width());
	height = max_c(fGradientControl->Frame().Height(), 30);

	fGradientControl->ResizeTo(width, height);
	fGradientControl->FrameResized(width, height);
	fGradientControl->MoveTo(frame.left, frame.top);

	AddChild(fGradientControl);
	fGradientControl->SetEnabled(false);

	fGradientControl->Gradient()->AddObserver(this);
}

// destructor
StyleView::~StyleView()
{
	SetStyle(NULL);
	fGradientControl->Gradient()->RemoveObserver(this);
}

// AttachedToWindow
void
StyleView::AttachedToWindow()
{
	fStyleType->Menu()->SetTargetForItems(this);
	fGradientType->Menu()->SetTargetForItems(this);
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

		default:
			BView::MessageReceived(message);
			break;
	}
}

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
		if (fGradient && *fGradient != *controlGradient) {
			if (fCommandStack) {
				fCommandStack->Perform(
					new (nothrow) SetGradientCommand(
						fStyle, controlGradient));
			} else {
				*fGradient = *controlGradient;
			}
		}
	} else if (object == fGradient) {
		if (*fGradient != *controlGradient) {
			fGradientControl->SetGradient(fGradient);
			_MarkType(fGradientType->Menu(), fGradient->Type());
		}
	} else if (object == fStyle) {
		// maybe the gradient was added or removed
		_SetGradient(fStyle->Gradient());
	}
}

// #pragma mark -

// StyleView
void
StyleView::SetStyle(Style* style)
{
	if (fStyle == style)
		return;

	if (fStyle)
		fStyle->RemoveObserver(this);

	fStyle = style;

	Gradient* gradient = NULL;
	if (fStyle) {
		fStyle->AddObserver(this);
		gradient = fStyle->Gradient();
	}

	_SetGradient(gradient);
}

// SetCommandStack
void
StyleView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}

// #pragma mark -

// _SetGradient
void
StyleView::_SetGradient(Gradient* gradient)
{
	if (fGradient == gradient)
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
}

// _MarkType
void
StyleView::_MarkType(BMenu* menu, int32 type) const
{
	for (int32 i = 0; BMenuItem* item = menu->ItemAt(i); i++) {
		BMessage* message = item->Message();
		int32 t;
		if (message->FindInt32("type", &t) == B_OK && t == type) {
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
			fCommandStack->Perform(
				new (nothrow) SetGradientCommand(
					fStyle, fGradientControl->Gradient()));
		} else {
			fStyle->SetGradient(fGradientControl->Gradient());
		}
	}
}

// _SetGradientType
void
StyleView::_SetGradientType(int32 type)
{
	fGradientControl->Gradient()->SetType((gradient_type)type);
}
