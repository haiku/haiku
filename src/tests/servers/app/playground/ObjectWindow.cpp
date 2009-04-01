// main.cpp

#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Alert.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <ColorControl.h>
#include <ListItem.h>
#include <ListView.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <Slider.h>
#include <String.h>
#include <RadioButton.h>
#include <Region.h>
#include <TabView.h>
#include <TextControl.h>
#include <TextView.h>

#include "ObjectView.h"
#include "States.h"
//#include "StatusView.h"

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

	MSG_OBJECT_SELECTED		= 'obsl',
	MSG_REMOVE_OBJECT		= 'rmob',
};

// ObjectItem
class ObjectItem : public BStringItem {
 public:
			ObjectItem(const char* name, State* object)
				: BStringItem(name),
				  fObject(object)
			{
			}

	State*	Object() const
			{ return fObject; }

 private:
	State*	fObject;
};

// ObjectListView
class ObjectListView : public BListView {
 public:
			ObjectListView(BRect frame, const char* name, list_view_type listType)
				: BListView(frame, name, listType)
			{
			}

	virtual	void KeyDown(const char* bytes, int32 numBytes)
			{
				switch (*bytes) {
					case B_DELETE:
						Window()->PostMessage(MSG_REMOVE_OBJECT);
						break;
					default:
						BListView::KeyDown(bytes, numBytes);
				}
			}

	virtual bool InitiateDrag(BPoint point, int32 itemIndex, bool wasSelected)
			{
				printf("InitiateDrag(BPoint(%.1f, %.1f), itemIndex: %ld, wasSelected: %d)\n",
						point.x, point.y, itemIndex, wasSelected);
				SwapItems(itemIndex, itemIndex + 1);
				return true;
			}

	virtual void SelectionChanged()
			{
//				printf("SelectionChanged() - first selected: %ld\n", CurrentSelection(0));
			}
};

// #pragma mark -

class TestView : public BView {
public:
	TestView(BRect frame, const char* name, uint32 resizeMode, uint32 flags)
		: BView(frame, name, resizeMode, flags)
	{
	}

	void AttachedToWindow()
	{
		SetViewColor(255, 0, 0);
	}
};

// constructor
ObjectWindow::ObjectWindow(BRect frame, const char* name)
	: BWindow(frame, name, B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			  B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	BRect b(Bounds());

	b.bottom = b.top + 8;
	BMenuBar* menuBar = new BMenuBar(b, "menu bar");
	AddChild(menuBar);

	BMenu* menu = new BMenu("File");
	menuBar->AddItem(menu);

	menu->AddItem(new BMenu("Submenu"));

	BMenuItem* menuItem = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
										'Q');
	menu->AddItem(menuItem);

	b = Bounds();
	b.top = menuBar->Bounds().bottom + 1;
	b.right = ceilf((b.left + b.right) / 2.0);
	BBox* bg = new BBox(b, "bg box", B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW,
		B_PLAIN_BORDER);

	AddChild(bg);
	bg->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// object view occupies the right side of the window
	b.left = b.right + 1.0;
	b.right = Bounds().right - B_V_SCROLL_BAR_WIDTH;
	b.bottom -= B_H_SCROLL_BAR_HEIGHT;
	fObjectView = new ObjectView(b, "object view", B_FOLLOW_ALL,
								 B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	// wrap a scroll view around the object view
	BScrollView* scrollView = new BScrollView("object scroller", fObjectView,
		B_FOLLOW_ALL, 0, true, true, B_NO_BORDER);

	if (BScrollBar* scrollBar = fObjectView->ScrollBar(B_VERTICAL)) {
		scrollBar->SetRange(0.0, fObjectView->Bounds().Height());
		scrollBar->SetProportion(0.5);
	}
	if (BScrollBar* scrollBar = fObjectView->ScrollBar(B_HORIZONTAL)) {
		scrollBar->SetRange(0.0, fObjectView->Bounds().Width());
		scrollBar->SetProportion(0.5);
	}
	AddChild(scrollView);

	b = bg->Bounds();
	// controls occupy the left side of the window
	b.InsetBy(5.0, 5.0);
	BBox* controlGroup = new BBox(b, "controls box",
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW, B_FANCY_BORDER);

	controlGroup->SetLabel("Controls");
	bg->AddChild(controlGroup);

	b = controlGroup->Bounds();
	b.top += controlGroup->InnerFrame().top;
	b.bottom = b.top + 25.0;
	b.InsetBy(10.0, 10.0);
	b.right = b.left + b.Width() / 2.0 - 5.0;

	// new button
	fNewB = new BButton(b, "new button", "New Object",
		new BMessage(MSG_NEW_OBJECT));
	controlGroup->AddChild(fNewB);
	SetDefaultButton(fNewB);

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
	BMenuItem* item = new BMenuItem("Alpha", message);
	item->SetMarked(true);
	popupMenu->AddItem(item);

	b.OffsetBy(0, radioButton->Bounds().Height() + 10.0);
	fDrawingModeMF = new BMenuField(b, "drawing mode field", "Mode:",
		popupMenu);

	controlGroup->AddChild(fDrawingModeMF);

	fDrawingModeMF->SetDivider(fDrawingModeMF->StringWidth(
		fDrawingModeMF->Label()) + 10.0);
	
	// color control
	b.OffsetBy(0, fDrawingModeMF->Bounds().Height() + 10.0);
	fColorControl = new BColorControl(b.LeftTop(), B_CELLS_16x16, 8,
		"color control", new BMessage(MSG_SET_COLOR));
	controlGroup->AddChild(fColorControl);
	
	// alpha text control
	b.OffsetBy(0, fColorControl-> Bounds().Height() + 5.0);
	fAlphaTC = new BTextControl(b, "alpha text control", "Alpha:", "",
		new BMessage(MSG_SET_COLOR));
	controlGroup->AddChild(fAlphaTC);

	// divide text controls the same
    float mWidth = fDrawingModeMF->StringWidth(fDrawingModeMF->Label());
    float aWidth = fAlphaTC->StringWidth(fAlphaTC->Label());
    
    float width = max_c(mWidth, aWidth) + 20.0;
	fDrawingModeMF->SetDivider(width);
	fAlphaTC->SetDivider(width);

	// fill check box
	b.OffsetBy(0, fAlphaTC->Bounds().Height() + 5.0);
	fFillCB = new BCheckBox(b, "fill check box", "Fill",
		new BMessage(MSG_SET_FILL_OR_STROKE));
	controlGroup->AddChild(fFillCB);

	// pen size text control
	b.OffsetBy(0, radioButton->Bounds().Height() + 5.0);
	b.bottom = b.top + 10.0;//35;
	fPenSizeS = new BSlider(b, "width slider", "Width:", NULL, 1, 100,
		B_TRIANGLE_THUMB);
	fPenSizeS->SetLimitLabels("1", "100");
	fPenSizeS->SetModificationMessage(new BMessage(MSG_SET_PEN_SIZE));
	fPenSizeS->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fPenSizeS->SetHashMarkCount(10);

	controlGroup->AddChild(fPenSizeS);

	// list view with objects
	b = controlGroup->Bounds();
	b.top += controlGroup->InnerFrame().top;
	b.InsetBy(10.0, 10.0);
	b.left = b.left + b.Width() / 2.0 + 6.0;
	b.right -= B_V_SCROLL_BAR_WIDTH;
    b.bottom = fDrawingModeMF->Frame().top - 10.0;

	fObjectLV = new ObjectListView(b, "object list", B_SINGLE_SELECTION_LIST);
	fObjectLV->SetSelectionMessage(new BMessage(MSG_OBJECT_SELECTED));

	// wrap a scroll view around the list view
	scrollView = new BScrollView("list scroller", fObjectLV,
		B_FOLLOW_NONE, 0, false, true, B_FANCY_BORDER);
	controlGroup->AddChild(scrollView);

	// enforce some size limits
	float minWidth = controlGroup->Frame().Width() + 30.0;
	float minHeight = fPenSizeS->Frame().bottom
		+ menuBar->Bounds().Height() + 15.0;
	float maxWidth = minWidth * 4.0;
	float maxHeight = minHeight + 100;
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
		case MSG_OBJECT_ADDED: {
			State* object;
			if (message->FindPointer("object", (void**)&object) >= B_OK) {
				fObjectLV->AddItem(new ObjectItem("Object", object));
			}
			// fall through
		}
		case MSG_OBJECT_COUNT_CHANGED:
			fClearB->SetEnabled(fObjectView->CountObjects() > 0);
			break;
		case MSG_OBJECT_SELECTED:
			if (ObjectItem* item = (ObjectItem*)fObjectLV->ItemAt(fObjectLV->CurrentSelection(0))) {
				fObjectView->SetState(item->Object());
				fObjectView->SetStateColor(item->Object()->Color());
				_UpdateControls();
			} else
				fObjectView->SetState(NULL);
			break;
		case MSG_REMOVE_OBJECT:
			while (ObjectItem* item = (ObjectItem*)fObjectLV->ItemAt(fObjectLV->CurrentSelection(0))) {
				fObjectView->RemoveObject(item->Object());
				fObjectLV->RemoveItem(item);
				delete item;
			}
			break;
		case MSG_NEW_OBJECT:
			fObjectView->SetState(NULL);
			break;
		case MSG_CLEAR: {
			BAlert *alert = new BAlert("Playground",
									   "Do you really want to clear all drawing objects?",
									   "No", "Yes");
			if (alert->Go() == 1) {
				fObjectView->MakeEmpty();
				fObjectLV->MakeEmpty();
			}
			break;
		}
		case MSG_SET_PEN_SIZE:
			fObjectView->SetStatePenSize((float)fPenSizeS->Value());
			break;
		case MSG_SET_DRAWING_MODE: {
			drawing_mode mode;
			if (message->FindInt32("mode", (int32*)&mode) >= B_OK) {
				fObjectView->SetStateDrawingMode(mode);
			}
			break;
		}
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

	sprintf(string, "%d", c.alpha);
	fAlphaTC->SetText(string);

	fColorControl->SetValue(c);
}

// _GetColor
rgb_color
ObjectWindow::_GetColor() const
{
	rgb_color c;
	
	c = fColorControl->ValueAsColor();
	c.alpha = max_c(0, min_c(255, atoi(fAlphaTC->Text())));
	
	return c;
}

