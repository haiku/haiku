/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Window.h>
#include <Slider.h>
#include <StringView.h>

#include <stdio.h>


class Window : public BWindow {
	public:
		Window();

		virtual bool QuitRequested();
};


void
downBy(BRect &rect, BView* view)
{
	rect.bottom = rect.top + view->Bounds().Height() + 10;
	rect.OffsetBy(0, rect.Height());
}


void
rightBy(BRect &rect, BView* view)
{
	rect.right = rect.left + view->Bounds().Width() + 10;
	rect.OffsetBy(rect.Width(), 0);
}


//	#pragma mark -


Window::Window()
	: BWindow(BRect(100, 100, 600, 400), "Slider-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BRect rect = Bounds();
	BView* view = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	// horizontal sliders

	rect.InsetBy(10, 10);
	rect.right = rect.left + 250;
	rect.bottom = rect.top + 30;

	BSlider* slider = new BSlider(rect, "Slider1", "Test 1", NULL, 0, 100);
	slider->ResizeToPreferred();
	view->AddChild(slider);

	downBy(rect, slider);

	slider = new BSlider(rect, "Slider2", "Test 2", NULL, 0, 100, B_TRIANGLE_THUMB);
	slider->ResizeToPreferred();
	view->AddChild(slider);
	
	downBy(rect, slider);

	slider = new BSlider(rect, "Slider3", "Test 3", NULL, 0, 100);
	rgb_color color = { 200, 80, 80, 255 };
	slider->UseFillColor(true, &color);
	slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	slider->SetHashMarkCount(11);
	slider->SetLimitLabels("0", "100");
	slider->SetBarThickness(12.0);
	slider->ResizeToPreferred();
	view->AddChild(slider);

	downBy(rect, slider);

	slider = new BSlider(rect, "Slider4", "Test 4", NULL, 0, 100, B_TRIANGLE_THUMB);
	slider->SetLimitLabels("0", "100");
	slider->UseFillColor(true, &color);
	slider->SetBarThickness(20.0);
	slider->SetHashMarks(B_HASH_MARKS_BOTH);
	slider->SetHashMarkCount(21);
	slider->ResizeToPreferred();
	view->AddChild(slider);

	downBy(rect, slider);
	rect.right = rect.left + 100;
	rect.bottom = rect.top + 100;

	slider = new BSlider(rect, "SliderA", "Test A", NULL, 0, 100);
	slider->SetLimitLabels("0", "100");
	slider->UseFillColor(true, &color);
	slider->SetHashMarks(B_HASH_MARKS_BOTH);
	slider->SetHashMarkCount(5);
	view->AddChild(slider);

	// vertical sliders

	rect.left = 270;
	rect.right = rect.left + 30;
	rect.top = 10;
	rect.bottom = view->Bounds().Height() - 20;

	slider = new BSlider(rect, "Slider5", "Test 5", NULL, 0, 100);
	slider->SetOrientation(B_VERTICAL);
	slider->SetBarThickness(12.0);
	slider->SetHashMarks(B_HASH_MARKS_RIGHT);
	slider->SetHashMarkCount(5);
	slider->ResizeToPreferred();
	view->AddChild(slider);

	rightBy(rect, slider);

	slider = new BSlider(rect, "Slider6", "Test 6", NULL, 0, 100, B_TRIANGLE_THUMB);
	slider->SetOrientation(B_VERTICAL);
	slider->ResizeToPreferred();
	view->AddChild(slider);
	
	rightBy(rect, slider);

	slider = new BSlider(rect, "Slider7", "Test 7", NULL, 0, 100);
	slider->SetOrientation(B_VERTICAL);
	slider->UseFillColor(true, &color);
	slider->SetHashMarks(B_HASH_MARKS_BOTH);
	slider->SetBarThickness(6.0);
	slider->SetHashMarkCount(11);
	slider->SetLimitLabels("0", "100");
	slider->ResizeToPreferred();
	view->AddChild(slider);

	rightBy(rect, slider);

	slider = new BSlider(rect, "Slider8", "Test 8", NULL, 0, 100, B_TRIANGLE_THUMB);
	slider->SetOrientation(B_VERTICAL);
	slider->UseFillColor(true, &color);
	slider->SetBarThickness(20.0);
	slider->SetHashMarks(B_HASH_MARKS_BOTH);
	slider->SetHashMarkCount(21);
	slider->SetLimitLabels("0", "100");
	slider->ResizeToPreferred();
	view->AddChild(slider);
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark -


class Application : public BApplication {
	public:
		Application();

		virtual void ReadyToRun(void);
};


Application::Application()
	: BApplication("application/x-vnd.haiku-test")
{
}


void
Application::ReadyToRun(void)
{
	BWindow *window = new Window();
	window->Show();
}


//	#pragma mark -


int 
main(int argc, char **argv)
{
	Application app;

	app.Run();
	return 0;
}

