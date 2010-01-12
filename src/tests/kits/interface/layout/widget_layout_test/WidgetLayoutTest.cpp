/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Window.h>

#include "BoxTest.h"
#include "ButtonTest.h"
#include "CheckBox.h"
#include "CheckBoxTest.h"
#include "GroupView.h"
#include "ListViewTest.h"
#include "MenuBarTest.h"
#include "MenuFieldTest.h"
#include "MenuTest.h"
#include "RadioButtonTest.h"
#include "ScrollBarTest.h"
#include "SliderTest.h"
#include "StringView.h"
#include "Test.h"
#include "TextControlTest.h"
#include "TextViewTest.h"
#include "TwoDimensionalSliderView.h"
#include "View.h"
#include "ViewContainer.h"
#include "WrapperView.h"


// internal messages
enum {
	MSG_2D_SLIDER_VALUE_CHANGED		= '2dsv',
	MSG_UNLIMITED_MAX_SIZE_CHANGED	= 'usch',
};


struct test_info {
	const char*	name;
	Test*		(*create)();
};

const test_info kTestInfos[] = {
	{ "BBox",			BoxTest::CreateTest },
	{ "BButton",		ButtonTest::CreateTest },
	{ "BCheckBox",		CheckBoxTest::CreateTest },
	{ "BListView",		ListViewTest::CreateTest },
	{ "BMenu",			MenuTest::CreateTest },
	{ "BMenuBar",		MenuBarTest::CreateTest },
	{ "BMenuField",		MenuFieldTest::CreateTest },
	{ "BRadioButton",	RadioButtonTest::CreateTest },
	{ "BScrollBar",		ScrollBarTest::CreateTest },
	{ "BSlider",		SliderTest::CreateTest },
	{ "BTextControl",	TextControlTest::CreateTest },
	{ "BTextView",		TextViewTest::CreateTest },
	{ NULL, NULL }
};


// helpful operator
BPoint
operator+(const BPoint& p, const BSize& size)
{
	return BPoint(p.x + size.width, p.y + size.height);
}


// TestWindow
class TestWindow : public BWindow {
public:
	TestWindow(Test* test)
		: BWindow(BRect(50, 50, 750, 550), "Widget Layout",
			B_TITLED_WINDOW,
			B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE | B_NOT_RESIZABLE
			| B_NOT_ZOOMABLE),
		  fTest(test)
	{
		fViewContainer = new ViewContainer(Bounds());
		fViewContainer->View::SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		AddChild(fViewContainer);

		// container view for the tested BView
		BRect rect(10, 10, 400, 400);
		View* view = new View(rect);
		fViewContainer->View::AddChild(view);
		view->SetViewColor((rgb_color){200, 200, 240, 255});

		// container for the test's controls
		fTestControlsView = new View(BRect(410, 10, 690, 400));
		fTestControlsView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fViewContainer->View::AddChild(fTestControlsView);

		// wrapper view
		fWrapperView = new WrapperView(fTest->GetView());
		fWrapperView->SetLocation(BPoint(10, 10));
		view->AddChild(fWrapperView);
		fWrapperView->SetSize(fWrapperView->PreferredSize());

		// slider view
		fSliderView = new TwoDimensionalSliderView(
			new BMessage(MSG_2D_SLIDER_VALUE_CHANGED), this);
		fSliderView->SetLocationRange(BPoint(0, 0), BPoint(0, 0));
		view->AddChild(fSliderView);

		_UpdateSliderConstraints();

		// size views
		GroupView* sizeViewsGroup = new GroupView(B_VERTICAL, 6);
		sizeViewsGroup->SetSpacing(0, 8);
		fViewContainer->View::AddChild(sizeViewsGroup);

		// min
		_CreateSizeViews(sizeViewsGroup, "min:  ", fMinWidthView,
			fMinHeightView);

		// max (with checkbox)
		fUnlimitedMaxSizeCheckBox = new LabeledCheckBox("override",
			new BMessage(MSG_UNLIMITED_MAX_SIZE_CHANGED), this);

		_CreateSizeViews(sizeViewsGroup, "max:  ", fMaxWidthView,
			fMaxHeightView, fUnlimitedMaxSizeCheckBox);

		// preferred
		_CreateSizeViews(sizeViewsGroup, "preferred:  ", fPreferredWidthView,
			fPreferredHeightView);

		// current
		_CreateSizeViews(sizeViewsGroup, "current:  ", fCurrentWidthView,
			fCurrentHeightView);

		sizeViewsGroup->SetFrame(BRect(BPoint(rect.left, rect.bottom + 10),
			sizeViewsGroup->PreferredSize()));

		_UpdateSizeViews();

		// activate test
		AddHandler(fTest);
		fTest->ActivateTest(fTestControlsView);
	}

	virtual void DispatchMessage(BMessage* message, BHandler* handler)
	{
		switch (message->what) {
			case B_LAYOUT_WINDOW:
				if (!fWrapperView->GetView()->IsLayoutValid()) {
					_UpdateSliderConstraints();
					_UpdateSizeViews();
				}
				break;
		}

		BWindow::DispatchMessage(message, handler);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_2D_SLIDER_VALUE_CHANGED:
			{
				BPoint sliderLocation(fSliderView->Location());
				BPoint wrapperLocation(fWrapperView->Location());
				BSize size(sliderLocation.x - wrapperLocation.x - 1,
					sliderLocation.y - wrapperLocation.y - 1);
				fWrapperView->SetSize(size);
				_UpdateSizeViews();
				break;
			}

			case MSG_UNLIMITED_MAX_SIZE_CHANGED:
				if (fUnlimitedMaxSizeCheckBox->IsSelected())  {
					fWrapperView->GetView()->SetExplicitMaxSize(
						BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
				} else {
					fWrapperView->GetView()->SetExplicitMaxSize(
						BSize(B_SIZE_UNSET, B_SIZE_UNSET));
				}
				break;

			default:
				BWindow::MessageReceived(message);
				break;
		}
	}

private:
	void _UpdateSliderConstraints()
	{
		BPoint wrapperLocation(fWrapperView->Location());
		BSize minWrapperSize(fWrapperView->MinSize());
		BSize maxWrapperSize(fWrapperView->MaxSize());

		BPoint minSliderLocation = wrapperLocation + minWrapperSize
			+ BPoint(1, 1);
		BPoint maxSliderLocation = wrapperLocation + maxWrapperSize
			+ BPoint(1, 1);

		BSize sliderSize(fSliderView->Size());
		BSize parentSize(fSliderView->Parent()->Size());
		if (maxSliderLocation.x + sliderSize.width > parentSize.width)
			maxSliderLocation.x = parentSize.width - sliderSize.width;
		if (maxSliderLocation.y + sliderSize.height > parentSize.height)
			maxSliderLocation.y = parentSize.height - sliderSize.height;

		fSliderView->SetLocationRange(minSliderLocation, maxSliderLocation);
	}

	void _CreateSizeViews(View* group, const char* labelText,
		StringView*& widthView, StringView*& heightView,
		View* additionalView = NULL)
	{
		// label
		StringView* label = new StringView(labelText);
		label->SetAlignment(B_ALIGN_RIGHT);
		group->AddChild(label);

		// width view
		widthView = new StringView("9999999999.9");
		widthView->SetAlignment(B_ALIGN_RIGHT);
		group->AddChild(widthView);
		widthView->SetExplicitMinSize(widthView->PreferredSize());

		// "," label
		StringView* labelView = new StringView(",  ");
		group->AddChild(labelView);

		// height view
		heightView = new StringView("9999999999.9");
		heightView->SetAlignment(B_ALIGN_RIGHT);
		group->AddChild(heightView);
		heightView->SetExplicitMinSize(heightView->PreferredSize());

		// spacing
		group->AddChild(new HStrut(20));

		// glue or unlimited max size check box
		if (additionalView)
			group->AddChild(additionalView);
		else
			group->AddChild(new Glue());
	}

	void _UpdateSizeView(StringView* view, float size)
	{
		char buffer[32];
		if (size < B_SIZE_UNLIMITED) {
			sprintf(buffer, "%.1f", size);
			view->SetString(buffer);
		} else
			view->SetString("unlimited");
	}

	void _UpdateSizeViews(StringView* widthView, StringView* heightView,
		BSize size)
	{
		_UpdateSizeView(widthView, size.width);
		_UpdateSizeView(heightView, size.height);
	}

	void _UpdateSizeViews()
	{
		_UpdateSizeViews(fMinWidthView, fMinHeightView,
			fWrapperView->GetView()->MinSize());
		_UpdateSizeViews(fMaxWidthView, fMaxHeightView,
			fWrapperView->GetView()->MaxSize());
		_UpdateSizeViews(fPreferredWidthView, fPreferredHeightView,
			fWrapperView->GetView()->PreferredSize());
		_UpdateSizeViews(fCurrentWidthView, fCurrentHeightView,
			fWrapperView->GetView()->Frame().Size());
	}

private:
	Test*						fTest;
	ViewContainer*				fViewContainer;
	View*						fTestControlsView;
	WrapperView*				fWrapperView;
	TwoDimensionalSliderView*	fSliderView;
	StringView*					fMinWidthView;
	StringView*					fMinHeightView;
	StringView*					fMaxWidthView;
	StringView*					fMaxHeightView;
	StringView*					fPreferredWidthView;
	StringView*					fPreferredHeightView;
	StringView*					fCurrentWidthView;
	StringView*					fCurrentHeightView;
	LabeledCheckBox*			fUnlimitedMaxSizeCheckBox;
};


static void
print_test_list(bool error)
{
	FILE* out = (error ? stderr : stdout);

	fprintf(out, "available tests:\n");

	for (int32 i = 0; kTestInfos[i].name; i++)
		fprintf(out, "  %s\n", kTestInfos[i].name);
}


int
main(int argc, const char* const* argv)
{
	// get test name
	const char* testName;
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <test name>\n", argv[0]);
		print_test_list(true);
		exit(1);
	}
	testName = argv[1];

	// create app
	BApplication app("application/x-vnd.haiku.widget-layout-test");

	// find and create the test
	Test* test = NULL;
	for (int32 i = 0; kTestInfos[i].name; i++) {
		if (strcmp(testName, kTestInfos[i].name) == 0) {
			test = (kTestInfos[i].create)();
			break;
		}
	}

	if (!test) {
		fprintf(stderr, "Error: Invalid test name: \"%s\"\n", testName);
		print_test_list(true);
		exit(1);
	}

	// show test window
	BWindow* window = new TestWindow(test);
	window->Show();

	app.Run();

	return 0;
}
