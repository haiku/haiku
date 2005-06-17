// main.cpp

#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <String.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <TextView.h>

#include "ObjectView.h"
#include "States.h"

#include "ObjectWindow.h"

enum {
	MSG_SET_OBJECT_TYPE		= 'stot',
	MSG_SET_FILL_OR_STROKE	= 'stfs',
	MSG_SET_COLOR			= 'stcl',
	MSG_SET_PEN_SIZE		= 'stps',
	MSG_SET_DRAWING_MODE	= 'stdm',

	MSG_NEW_OBJECT			= 'nobj',

	MSG_UNDO				= 'undo',
	MSG_REDO				= 'redo',

	MSG_CLEAR				= 'clir',
};

// constructor
ObjectWindow::ObjectWindow(BRect frame, const char* name)
	: BWindow(frame, name, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			  B_ASYNCHRONOUS_CONTROLS)
//	: BWindow(frame, name, B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
//			  B_ASYNCHRONOUS_CONTROLS)
//	: BWindow(frame, name, B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
//			  B_ASYNCHRONOUS_CONTROLS)
//	: BWindow(frame, name, B_BORDERED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
//			  B_ASYNCHRONOUS_CONTROLS)
//	: BWindow(frame, name, B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
//			  B_ASYNCHRONOUS_CONTROLS)
{
	BRect b(Bounds());

	b.bottom = b.top + 8;
	BMenuBar* menuBar = new BMenuBar(b, "menu bar");
	AddChild(menuBar);

	BMenu* menu = new BMenu("File");
	menuBar->AddItem(menu);

	BMenuItem* menuItem = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
										'Q');
	menu->AddItem(menuItem);

	b = Bounds();
	b.top = menuBar->Bounds().bottom + 1;
	BBox* bg = new BBox(b, "bg box", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);

	AddChild(bg);
	bg->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	b = bg->Bounds();
	// object views occupies the right side of the window
	b.Set(ceilf((b.left + b.right) / 3.0) + 3.0, b.top + 5.0, b.right - 5.0, b.bottom - 5.0);
	fObjectView = new ObjectView(b, "object view", B_FOLLOW_ALL,
								 B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);

	bg->AddChild(fObjectView);

	b = bg->Bounds();
	// controls occupy the left side of the window
	b.Set(b.left + 5.0, b.top + 5.0, ceilf((b.left + b.right) / 3.0) - 2.0, b.bottom - 5.0);
	BBox* controlGroup = new BBox(b, "controls box", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,
								  B_WILL_DRAW, B_FANCY_BORDER);

	controlGroup->SetLabel("Controls");
	bg->AddChild(controlGroup);

	b = controlGroup->Bounds();
	b.top += 10.0;
	b.bottom = b.top + 25.0;
	b.InsetBy(5.0, 5.0);

	// new button
	fNewB = new BButton(b, "new button", "New Object", new BMessage(MSG_NEW_OBJECT));
	controlGroup->AddChild(fNewB);

	// clear button
	b.OffsetBy(0, fNewB->Bounds().Height() + 5.0);
	fClearB = new BButton(b, "clear button", "Clear", new BMessage(MSG_CLEAR));
	controlGroup->AddChild(fClearB);

	// object type radio buttons
	BMessage* message;
	BRadioButton* radioButton;

	b.OffsetBy(0, fClearB->Bounds().Height() + 5.0);
	message = new BMessage(MSG_SET_OBJECT_TYPE);
	message->AddInt32("type", OBJECT_LINE);
	radioButton = new BRadioButton(b, "radio 1", "Line", message);
	controlGroup->AddChild(radioButton);

	radioButton->SetValue(B_CONTROL_ON);

	b.OffsetBy(0, radioButton->Bounds().Height() + 5.0);
	message = new BMessage(MSG_SET_OBJECT_TYPE);
	message->AddInt32("type", OBJECT_RECT);
	radioButton = new BRadioButton(b, "radio 2", "Rect", message);
	controlGroup->AddChild(radioButton);

	b.OffsetBy(0, radioButton->Bounds().Height() + 5.0);
	message = new BMessage(MSG_SET_OBJECT_TYPE);
	message->AddInt32("type", OBJECT_ROUND_RECT);
	radioButton = new BRadioButton(b, "radio 3", "Round Rect", message);
	controlGroup->AddChild(radioButton);

	b.OffsetBy(0, radioButton->Bounds().Height() + 5.0);
	message = new BMessage(MSG_SET_OBJECT_TYPE);
	message->AddInt32("type", OBJECT_ELLIPSE);
	radioButton = new BRadioButton(b, "radio 4", "Ellipse", message);
	controlGroup->AddChild(radioButton);

	// drawing mode
	BPopUpMenu* popupMenu = new BPopUpMenu("<pick>");

	message = new BMessage(MSG_SET_DRAWING_MODE);
	message->AddInt32("mode", B_OP_COPY);
	popupMenu->AddItem(new BMenuItem("Copy", message));

	message = new BMessage(MSG_SET_DRAWING_MODE);
	message->AddInt32("mode", B_OP_OVER);
	popupMenu->AddItem(new BMenuItem("Over", message));

	message = new BMessage(MSG_SET_DRAWING_MODE);
	message->AddInt32("mode", B_OP_INVERT);
	popupMenu->AddItem(new BMenuItem("Invert", message));

	message = new BMessage(MSG_SET_DRAWING_MODE);
	message->AddInt32("mode", B_OP_BLEND);
	popupMenu->AddItem(new BMenuItem("Blend", message));

	message = new BMessage(MSG_SET_DRAWING_MODE);
	message->AddInt32("mode", B_OP_SELECT);
	popupMenu->AddItem(new BMenuItem("Select", message));

	message = new BMessage(MSG_SET_DRAWING_MODE);
	message->AddInt32("mode", B_OP_ERASE);
	popupMenu->AddItem(new BMenuItem("Erase", message));

	message = new BMessage(MSG_SET_DRAWING_MODE);
	message->AddInt32("mode", B_OP_ADD);
	popupMenu->AddItem(new BMenuItem("Add", message));

	message = new BMessage(MSG_SET_DRAWING_MODE);
	message->AddInt32("mode", B_OP_SUBTRACT);
	popupMenu->AddItem(new BMenuItem("Subtract", message));

	message = new BMessage(MSG_SET_DRAWING_MODE);
	message->AddInt32("mode", B_OP_MIN);
	popupMenu->AddItem(new BMenuItem("Min", message));

	message = new BMessage(MSG_SET_DRAWING_MODE);
	message->AddInt32("mode", B_OP_MAX);
	popupMenu->AddItem(new BMenuItem("Max", message));

	message = new BMessage(MSG_SET_DRAWING_MODE);
	message->AddInt32("mode", B_OP_ALPHA);
	popupMenu->AddItem(new BMenuItem("Alpha", message));

	b.OffsetBy(0, radioButton->Bounds().Height() + 5.0);
	fDrawingModeMF = new BMenuField(b, "drawing mode field", "Mode",
									popupMenu);

	controlGroup->AddChild(fDrawingModeMF);

	fDrawingModeMF->SetDivider(fDrawingModeMF->StringWidth(fDrawingModeMF->Label()) + 10.0);

	// red text control
	b.OffsetBy(0, fDrawingModeMF->Bounds().Height() + 5.0);
	fRedTC = new BTextControl(b, "red text control", "Red", "",
							  new BMessage(MSG_SET_COLOR));
	controlGroup->AddChild(fRedTC);

	// green text control
	b.OffsetBy(0, fRedTC->Bounds().Height() + 5.0);
	fGreenTC = new BTextControl(b, "green text control", "Green", "",
								new BMessage(MSG_SET_COLOR));
	controlGroup->AddChild(fGreenTC);

	// blue text control
	b.OffsetBy(0, fGreenTC->Bounds().Height() + 5.0);
	fBlueTC = new BTextControl(b, "blue text control", "Blue", "",
							   new BMessage(MSG_SET_COLOR));
	controlGroup->AddChild(fBlueTC);

	// alpha text control
	b.OffsetBy(0, fBlueTC->Bounds().Height() + 5.0);
	fAlphaTC = new BTextControl(b, "alpha text control", "Alpha", "",
								new BMessage(MSG_SET_COLOR));
	controlGroup->AddChild(fAlphaTC);
/*
// TODO: while this block of code works in the Haiku app_server running under R5,
// it crashes pretty badly under Haiku. I have no idea why this happens, because
// I was doing the same thing before at other places.
	// divide text controls the same
	float rWidth = fRedTC->StringWidth(fRedTC->Label());
	float gWidth = fGreenTC->StringWidth(fGreenTC->Label());
	float bWidth = fBlueTC->StringWidth(fBlueTC->Label());
	float aWidth = fAlphaTC->StringWidth(fAlphaTC->Label());

	float width = max_c(rWidth, max_c(gWidth, max_c(bWidth, aWidth))) + 10.0;
	fRedTC->SetDivider(width);
	fGreenTC->SetDivider(width);
	fBlueTC->SetDivider(width);
	fAlphaTC->SetDivider(width);*/

	// fill check box
	b.OffsetBy(0, fAlphaTC->Bounds().Height() + 5.0);
	fFillCB = new BCheckBox(b, "fill check box", "Fill",
							new BMessage(MSG_SET_FILL_OR_STROKE));
	controlGroup->AddChild(fFillCB);

	// pen size text control
	b.OffsetBy(0, radioButton->Bounds().Height() + 5.0);
	b.bottom = b.top + 10.0;//35;
	fPenSizeS = new BSlider(b, "width slider", "Width",
							NULL, 1, 100, B_TRIANGLE_THUMB);
	fPenSizeS->SetLimitLabels("1", "100");
	fPenSizeS->SetModificationMessage(new BMessage(MSG_SET_PEN_SIZE));
	fPenSizeS->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fPenSizeS->SetHashMarkCount(10);

	controlGroup->AddChild(fPenSizeS);

	// enforce some size limits
	float minWidth = controlGroup->Frame().Width() + 30.0;
	float minHeight = fPenSizeS->Frame().bottom +
					  menuBar->Bounds().Height() + 15.0;
	float maxWidth = minWidth * 4.0;
	float maxHeight = minHeight;
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	ResizeTo(max_c(frame.Width(), minWidth), max_c(frame.Height(), minHeight));

	_UpdateControls();
}

// destructor
ObjectWindow::~ObjectWindow()
{
}

// QuitRequested
bool
ObjectWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

// MessageReceived
void
ObjectWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SET_OBJECT_TYPE: {
			int32 type;
			if (message->FindInt32("type", &type) >= B_OK) {
				fObjectView->SetObjectType(type);
				fFillCB->SetEnabled(type != OBJECT_LINE);
				if (!fFillCB->IsEnabled())
					fPenSizeS->SetEnabled(true);
				else
					fPenSizeS->SetEnabled(fFillCB->Value() == B_CONTROL_OFF);
			}
			break;
		}
		case MSG_SET_FILL_OR_STROKE: {
			int32 value;
			if (message->FindInt32("be:value", &value) >= B_OK) {
				fObjectView->SetStateFill(value);
				fPenSizeS->SetEnabled(value == B_CONTROL_OFF);
			}
			break;
		}
		case MSG_SET_COLOR:
			fObjectView->SetStateColor(_GetColor());
			_UpdateColorControls();
			break;
		case MSG_OBJECT_COUNT_CHANGED:
			fClearB->SetEnabled(fObjectView->CountObjects() > 0);
			break;
		case MSG_NEW_OBJECT:
			fObjectView->SetState(NULL);
			break;
		case MSG_CLEAR: {
			BAlert *alert = new BAlert("Playground", "Do you really want to clear all drawing objects?", "No", "Yes");
			if (alert->Go() == 1) {
				fObjectView->MakeEmpty();
			}
			break;
		}
		case MSG_SET_PEN_SIZE:
			fObjectView->SetStatePenSize((float)fPenSizeS->Value());
			break;
		default:
			BWindow::MessageReceived(message);
	}
}

// _UpdateControls
void
ObjectWindow::_UpdateControls() const
{
	_UpdateColorControls();

	// update buttons
	fClearB->SetEnabled(fObjectView->CountObjects() > 0);

	fFillCB->SetEnabled(fObjectView->ObjectType() != OBJECT_LINE);

	// pen size
	fPenSizeS->SetValue((int32)fObjectView->StatePenSize());

	// disable penSize if fill is on
	if (!fFillCB->IsEnabled())
		fPenSizeS->SetEnabled(true);
	else
		fPenSizeS->SetEnabled(fFillCB->Value() == B_CONTROL_OFF);
}

// _UpdateColorControls
void
ObjectWindow::_UpdateColorControls() const
{
	// update color
	rgb_color c = fObjectView->StateColor();
	char string[32];

	sprintf(string, "%d", c.red);
	fRedTC->SetText(string);

	sprintf(string, "%d", c.green);
	fGreenTC->SetText(string);

	sprintf(string, "%d", c.blue);
	fBlueTC->SetText(string);

	sprintf(string, "%d", c.alpha);
	fAlphaTC->SetText(string);
}

// _GetColor
rgb_color
ObjectWindow::_GetColor() const
{
	rgb_color c;
	c.red	= max_c(0, min_c(255, atoi(fRedTC->Text())));
	c.green	= max_c(0, min_c(255, atoi(fGreenTC->Text())));
	c.blue	= max_c(0, min_c(255, atoi(fBlueTC->Text())));
	c.alpha	= max_c(0, min_c(255, atoi(fAlphaTC->Text())));

	return c;
}

