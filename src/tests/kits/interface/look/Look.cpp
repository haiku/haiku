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
#include <MenuItem.h>
#include <OptionControl.h>
#include <OutlineListView.h>
#include <RadioButton.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <Slider.h>
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

	BGridView* view = new BGridView(5.0f, 5.0f);
	BGridLayout* layout = view->GridLayout();
	layout->SetInsets(5, 5, 5, 5);
	view->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	int32 row = 0;
	add_controls<BButton>(layout, row);
	add_controls<BCheckBox>(layout, row);
	add_controls<BRadioButton>(layout, row);

	window->AddChild(view);

	window->Show();
	app.Run();
	return 0;
}

