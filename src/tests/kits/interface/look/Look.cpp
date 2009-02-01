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
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
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


template <class ControlType>
void
add_controls(BGridLayout* layout, int32& row)
{
	ControlType* control1 = new ControlType("Enabled", NULL);
	ControlType* control2 = new ControlType("Disabled", NULL);
	control2->SetEnabled(false);
	ControlType* control3 = new ControlType("On", NULL);
	control3->SetValue(B_CONTROL_ON);

	layout->AddView(control1, 0, row);
	layout->AddView(control2, 1, row);
	layout->AddView(control3, 2, row);

	row++;
}


void
add_menu_fields(BGridLayout* layout, int32& row)
{
	BPopUpMenu* menu1 = new BPopUpMenu("Selection");
	BMenuField* control1 = new BMenuField("Enabled", menu1, NULL);
	BPopUpMenu* menu2 = new BPopUpMenu("Selection");
	BMenuField* control2 = new BMenuField("Disabled", menu2, NULL);
	control2->SetEnabled(false);

	layout->AddView(BGroupLayoutBuilder(B_HORIZONTAL, 5)
		.Add(control1)
		.Add(control2), 0, row, 3);

	row++;
}


void
add_text_controls(BGridLayout* layout, int32& row)
{
	BTextControl* control1 = new BTextControl("Enabled", "Some Text", NULL);
	BTextControl* control2 = new BTextControl("Disabled", "More Text", NULL);
	control2->SetEnabled(false);

	layout->AddView(BGroupLayoutBuilder(B_HORIZONTAL, 5)
		.Add(control1)
		.Add(control2), 0, row, 3);

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

	layout->AddView(BGroupLayoutBuilder(B_HORIZONTAL, 5)
		.Add(control1)
		.Add(control2), 0, row, 3);

	row++;

	control1 = new BSlider("slider 3", "Enabled", NULL, 1, 100,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	control2 = new BSlider("slider 4", "Disabled", NULL, 1, 100,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	control2->SetEnabled(false);

	control1->SetLimitLabels("Min", "Max");
	control2->SetLimitLabels("1", "100");

	layout->AddView(BGroupLayoutBuilder(B_HORIZONTAL, 5)
		.Add(control1)
		.Add(control2), 0, row, 3);

	row++;
}


int
main(int argc, char** argv)
{
	BApplication app("application/x-vnd.haiku-look");

	BWindow* window = new BWindow(BRect(50, 50, 100, 100),
		"Look at these pretty controls!", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_QUIT_ON_WINDOW_CLOSE);

	window->SetLayout(new BGroupLayout(B_HORIZONTAL));

	// create some controls

	BListView* listView = new BListView();
	listView->AddItem(new BStringItem("List Item 1"));
	listView->AddItem(new BStringItem("List Item 2"));
	BScrollView* scrollView = new BScrollView("scroller", listView, 0,
		true, true);
	scrollView->SetExplicitMinSize(BSize(300, 80));

	BGridView* controls = new BGridView(5.0f, 5.0f);
	BGridLayout* layout = controls->GridLayout();
	controls->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	int32 row = 0;
	add_controls<BButton>(layout, row);
	add_controls<BCheckBox>(layout, row);
	add_controls<BRadioButton>(layout, row);
	add_menu_fields(layout, row);
	add_text_controls(layout, row);
	add_sliders(layout, row);

	BStatusBar* statusBar = new BStatusBar("status bar", "Status",
		"Completed");
	statusBar->SetMaxValue(100);
	statusBar->SetTo(40);
	statusBar->SetBarHeight(12);
	layout->AddView(statusBar, 0, row, 3);

	row++;

	BColorControl* colorControl = new BColorControl(B_ORIGIN, B_CELLS_32x8,
		8.0f, "color control");
	layout->AddView(colorControl, 0, row, 3);

	BTabView* tabView = new BTabView("tab view", B_WIDTH_FROM_WIDEST);
	BView* content = BGroupLayoutBuilder(B_VERTICAL, 5)
		.Add(scrollView)
		.Add(controls)
		.SetInsets(5, 5, 5, 5);

	content->SetName("Tab 1");

	tabView->AddTab(content);
	tabView->AddTab(new BView("Tab 2", 0));
	tabView->AddTab(new BView("Tab 3", 0));

	BMenuBar* menuBar = new BMenuBar("menu bar");
	BMenu* menu = new BMenu("File");
	BMenuItem* item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED));
	menu->AddItem(item);
	menuBar->AddItem(menu);
	menu = new BMenu("Edit");
	menu->SetEnabled(false);
	menu->AddItem(new BMenuItem("Cut", NULL));
	menu->AddItem(new BMenuItem("Copy", NULL));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Past", NULL));
	menuBar->AddItem(menu);

	BButton* okButton = new BButton("Ok", new BMessage(B_QUIT_REQUESTED));

	window->AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(menuBar)
		.Add(BGroupLayoutBuilder(B_VERTICAL, 5)
			.Add(tabView)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, 5)
				.Add(new BButton("Revert", NULL))
				.Add(BSpaceLayoutItem::CreateGlue())
				.Add(new BButton("Cancel", NULL))
				.Add(okButton)
			)
			.SetInsets(5, 5, 5, 5)
		)
	);

	window->SetDefaultButton(okButton);

	window->Show();
	app.Run();
	return 0;
}

