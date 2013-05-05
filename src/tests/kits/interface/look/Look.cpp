/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <ChannelSlider.h>
#include <CheckBox.h>
#include <ColorControl.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <ControlLook.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <OptionControl.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <Slider.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TabView.h>
#include <TextControl.h>
#include <TextView.h>
#include <Window.h>


static const float kInset = 8.0f;


template <class ControlType>
void
add_controls(BGridLayout* layout, int32& row)
{
	ControlType* control1 = new ControlType("Enabled", NULL);
	ControlType* control2 = new ControlType("Disabled", NULL);
	control2->SetEnabled(false);
	ControlType* control3 = new ControlType("Enabled", NULL);
	control3->SetValue(B_CONTROL_ON);
	ControlType* control4 = new ControlType("Disabled", NULL);
	control4->SetValue(B_CONTROL_ON);
	control4->SetEnabled(false);

	control1->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	control2->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	control3->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	control4->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	layout->AddView(control1, 0, row);
	layout->AddView(control2, 1, row);
	layout->AddView(control3, 2, row);
	layout->AddView(control4, 3, row);

	row++;
}


#define USE_LAYOUT_ITEMS 0


void
add_menu_fields(BGridLayout* layout, int32& row)
{
	BPopUpMenu* menu1 = new BPopUpMenu("Selection");
	BMenuField* control1 = new BMenuField("Enabled", menu1);
	BPopUpMenu* menu2 = new BPopUpMenu("Selection");
	BMenuField* control2 = new BMenuField("Disabled", menu2);
	control2->SetEnabled(false);

#if USE_LAYOUT_ITEMS
	layout->AddItem(control1->CreateLabelLayoutItem(), 0, row);
	layout->AddItem(control1->CreateMenuBarLayoutItem(), 1, row);
	layout->AddItem(control2->CreateLabelLayoutItem(), 2, row);
	layout->AddItem(control2->CreateMenuBarLayoutItem(), 3, row);
#else
	layout->AddView(control1, 0, row, 2);
	layout->AddView(control2, 2, row, 2);
#endif

	row++;
}


void
add_text_controls(BGridLayout* layout, int32& row)
{
	BTextControl* control1 = new BTextControl("Enabled", "Some text", NULL);
	BTextControl* control2 = new BTextControl("Disabled", "More text", NULL);
	control2->SetEnabled(false);

#if USE_LAYOUT_ITEMS
	layout->AddItem(control1->CreateLabelLayoutItem(), 0, row);
	layout->AddItem(control1->CreateTextViewLayoutItem(), 1, row);
	layout->AddItem(control2->CreateLabelLayoutItem(), 2, row);
	layout->AddItem(control2->CreateTextViewLayoutItem(), 3, row);
#else
	layout->AddView(control1, 0, row, 2);
	layout->AddView(control2, 2, row, 2);
#endif

	row++;
}


void
add_sliders(BGridLayout* layout, int32& row)
{
	BSlider* control1 = new BSlider("slider 1", "Enabled", NULL, 1, 100,
		B_HORIZONTAL);
	BSlider* control2 = new BSlider("slider 2", "Disabled", NULL, 1, 100,
		B_HORIZONTAL);
	control2->SetEnabled(false);

	control1->SetHashMarkCount(10);
	control1->SetHashMarks(B_HASH_MARKS_BOTTOM);
	control2->SetHashMarkCount(10);
	control2->SetHashMarks(B_HASH_MARKS_BOTTOM);

	layout->AddView(control1, 0, row, 2);
	layout->AddView(control2, 2, row, 2);

	row++;

	control1 = new BSlider("slider 3", "Enabled", NULL, 1, 100,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	control2 = new BSlider("slider 4", "Disabled", NULL, 1, 100,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	control2->SetEnabled(false);

	rgb_color fillColor = (rgb_color){ 255, 115, 0, 255 };

	control1->SetLimitLabels("Min", "Max");
	control1->UseFillColor(true, &fillColor);
	control1->SetValue(20);

	control2->SetLimitLabels("1", "100");
	control2->UseFillColor(true, &fillColor);
	control2->SetValue(10);

	layout->AddView(control1, 0, row, 2);
	layout->AddView(control2, 2, row, 2);

	row++;
}


void
add_status_bars(BGridLayout* layout, int32& row)
{
	BBox* box = new BBox(B_FANCY_BORDER, NULL);
	box->SetLabel("Info");

	BGroupLayout* boxLayout = new BGroupLayout(B_VERTICAL, kInset);
	boxLayout->SetInsets(kInset, kInset + box->TopBorderOffset(), kInset,
		kInset);
	box->SetLayout(boxLayout);

	BStatusBar* statusBar = new BStatusBar("status bar", "Status",
		"Completed");
	statusBar->SetMaxValue(100);
	statusBar->SetTo(0);
	statusBar->SetBarHeight(12);
	boxLayout->AddView(statusBar);

	statusBar = new BStatusBar("status bar", "Progress",
		"Completed");
	statusBar->SetMaxValue(100);
	statusBar->SetTo(40);
	statusBar->SetBarHeight(12);
	boxLayout->AddView(statusBar);

	statusBar = new BStatusBar("status bar", "Lifespan of capitalism",
		"Completed");
	statusBar->SetMaxValue(100);
	statusBar->SetTo(100);
	statusBar->SetBarHeight(12);
	boxLayout->AddView(statusBar);

	layout->AddView(box, 0, row, 4);

	row++;
}


enum {
	MSG_TEST_OPEN_FILE_PANEL = 'tofp',
	MSG_TEST_SAVE_FILE_PANEL = 'tsfp',
	MSG_TOGGLE_LOOK = 'tggl'
};


class Window : public BWindow {
public:
	Window(BRect frame, const char* title, window_type type, uint32 flags)
		: BWindow(frame, title, type, flags)
	{
		fControlLook = NULL;
	}
	~Window()
	{
		if (fControlLook != NULL)
			be_control_look = fControlLook;
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
		case MSG_TEST_OPEN_FILE_PANEL:
			{
				BFilePanel* panel = new BFilePanel();
				panel->Show();
			}
			break;
		case MSG_TEST_SAVE_FILE_PANEL:
			{
				BFilePanel* panel = new BFilePanel(B_SAVE_PANEL);
				panel->Show();
			}
			break;
		case MSG_TOGGLE_LOOK:
			{
				BControlLook* temp = fControlLook;
				fControlLook = be_control_look;
				be_control_look = temp;
				_InvalidateChildrenAndView(ChildAt(0));
			}
			break;

		default:
			BWindow::MessageReceived(message);
		}
	}

private:
	void _InvalidateChildrenAndView(BView* view)
	{
		for (int32 i = 0; BView* child = view->ChildAt(i); i++)
			_InvalidateChildrenAndView(child);
		view->Invalidate();
	}

private:
	BControlLook* fControlLook;
};


int
main(int argc, char** argv)
{
	BApplication app("application/x-vnd.haiku-look");

	BWindow* window = new Window(BRect(50, 50, 100, 100),
		"Look at these pretty controls!", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_QUIT_ON_WINDOW_CLOSE);

	window->SetLayout(new BGroupLayout(B_HORIZONTAL));

	// create some controls

	// BListView
	BListView* listView = new BListView();
	for (int32 i = 0; i < 20; i++) {
		BString itemLabel("List item ");
		itemLabel << i + 1;
		listView->AddItem(new BStringItem(itemLabel.String()));
	}
	BScrollView* scrollView = new BScrollView("scroller", listView, 0,
		true, true);
	scrollView->SetExplicitMinSize(BSize(300, 140));

	// BColumnListView
	BColumnListView* columnListView = new BColumnListView("clv", 0,
		B_FANCY_BORDER);
//		B_PLAIN_BORDER);
//		B_NO_BORDER);

	columnListView->AddColumn(new BTitledColumn("Short",
		150, 50, 500, B_ALIGN_LEFT), 0);
	columnListView->AddColumn(new BTitledColumn("Medium Length",
		100, 50, 500, B_ALIGN_CENTER), 1);
	columnListView->AddColumn(new BTitledColumn("Some Long Column Name",
		130, 50, 500, B_ALIGN_RIGHT), 2);

//	for (int32 i = 0; i < 20; i++) {
//		BString itemLabel("List Item ");
//		itemLabel << i + 1;
//		columnListView->AddItem(new BStringItem(itemLabel.String()));
//	}


	BGridView* controls = new BGridView(kInset, kInset);
	BGridLayout* layout = controls->GridLayout();
	controls->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	int32 row = 0;
	add_controls<BButton>(layout, row);
	add_controls<BCheckBox>(layout, row);
	add_controls<BRadioButton>(layout, row);
	add_menu_fields(layout, row);
	add_text_controls(layout, row);
	add_sliders(layout, row);
	add_status_bars(layout, row);

	BColorControl* colorControl = new BColorControl(B_ORIGIN, B_CELLS_32x8,
		8.0f, "color control");
	layout->AddView(colorControl, 0, row, 4);

	BTabView* tabView = new BTabView("tab view", B_WIDTH_FROM_WIDEST);
	BGroupView* content = new BGroupView(B_VERTICAL, kInset);
	BLayoutBuilder::Group<>(content)
		.Add(scrollView)
		.Add(columnListView)
		.Add(controls)
		.SetInsets(kInset, kInset, kInset, kInset);

	content->SetName("Tab 1");

	tabView->AddTab(content);
	BView* tab2 = new BView("Tab 2", 0);
	tab2->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	tabView->AddTab(tab2);
	tabView->AddTab(new BView("Tab 3", 0));

	BMenuBar* menuBar = new BMenuBar("menu bar");
	BMenu* menu = new BMenu("File");
	menu->AddItem(new BMenuItem("Test Open BFilePanel",
		new BMessage(MSG_TEST_OPEN_FILE_PANEL)));
	menu->AddItem(new BMenuItem("Test Save BFilePanel",
		new BMessage(MSG_TEST_SAVE_FILE_PANEL)));
	menu->AddItem(new BMenuItem("Click me!", NULL));
	menu->AddItem(new BMenuItem("Another option", NULL));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED)));
	menuBar->AddItem(menu);
	menu = new BMenu("Edit");
	menu->SetEnabled(false);
	menu->AddItem(new BMenuItem("Cut", NULL));
	menu->AddItem(new BMenuItem("Copy", NULL));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Past", NULL));
	menuBar->AddItem(menu);
	menu = new BMenu("One Item");
	menu->AddItem(new BMenuItem("Only", NULL));
	menuBar->AddItem(menu);
	menu = new BMenu("Sub Menu");
	BMenu* subMenu = new BMenu("Click me");
	subMenu->AddItem(new BMenuItem("Either", NULL));
	subMenu->AddItem(new BMenuItem("Or", NULL));
	subMenu->SetRadioMode(true);
	menu->AddItem(subMenu);
	menuBar->AddItem(menu);

	BButton* okButton = new BButton("OK", new BMessage(B_QUIT_REQUESTED));

	BLayoutBuilder::Group<>(window, B_VERTICAL, 0)
		.Add(menuBar)
		.AddGroup(B_VERTICAL, kInset)
			.SetInsets(kInset, kInset, kInset, kInset)
			.Add(tabView)
			.AddGroup(B_HORIZONTAL, kInset)
				.Add(new BButton("Revert", new BMessage(MSG_TOGGLE_LOOK)))
				.AddGlue()
				.Add(new BButton("Cancel", NULL))
				.Add(okButton);

	window->SetDefaultButton(okButton);

	window->Show();
	app.Run();
	return 0;
}

