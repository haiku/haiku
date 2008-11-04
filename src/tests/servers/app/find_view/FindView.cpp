/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Window.h>
#include <View.h>

#include <stdio.h>


class View : public BView {
public:
	View(BRect rect, const char* name, uint32 followMode,
			uint8 red, uint8 green, uint8 blue)
		: BView(rect, name, followMode, 0)
	{
		SetViewColor(red, green, blue);
	}
};


class TestView : public View {
public:
	TestView(BRect rect, const char* name, uint32 followMode,
			uint8 red, uint8 green, uint8 blue)
		: View(rect, name, followMode, red, green, blue)
	{
		SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	}

	virtual	void MouseMoved(BPoint where, uint32 transit,
			const BMessage* dragMessage)
	{
		ConvertToScreen(&where);
		where -= Window()->Frame().LeftTop();
		BView* view = Window()->FindView(where);
		printf("View at (%.1f, %.1f): %s\n", where.x, where.y,
			view ? view->Name() : "NULL");
	}
};


//	#pragma mark -


int 
main(int argc, char **argv)
{
	BApplication app("application/x-vnd.haiku-find_view");

	BWindow* window = new BWindow(BRect(100, 100, 400, 400),
		"ViewTransit-Test", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	// TestView
	BRect frame = window->Bounds();
	View* testView = new TestView(frame, "Test View", B_FOLLOW_ALL, 255, 0, 0);
	window->AddChild(testView);

	// View 1
	frame.InsetBy(20, 20);
	frame.right /= 2;
	View* view1 = new View(frame, "View 1",
		B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT, 0, 255, 0);
	testView->AddChild(view1);

	// View 2
	frame.left = frame.right + 1;
	frame.right = window->Bounds().right - 20;
	View* view2 = new View(frame, "View 2",
		B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT, 0, 0, 255);
	testView->AddChild(view2);
	view2->Hide();


	window->Show();

	app.Run();
	return 0;
}
