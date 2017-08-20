/*
 * Copyright 2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#include <Application.h>
#include <ControlLook.h>
#include <View.h>
#include <Window.h>


class View : public BView {
	public:
						View(BRect r);
				void	Draw(BRect update);
				void	DrawButtonFrames(BRect r, BRect update);

};


View::View(BRect r)
	: BView(r, "test", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
View::Draw(BRect update)
{
	// - DrawButtonFrame (rounded + square)
	// - DrawMenuFieldFrame + Background
	// - DrawActiveTab
	// - DrawSliderBar
	
	MovePenTo(20, 20);
	DrawString("TEST");

	// Reference line
	BRect r(20, 30, 80, 60);
	DrawButtonFrames(r, update);

	PushState();

	// Positive translation
	TranslateBy(0, 35);
	DrawButtonFrames(r, update);

	// Null, then negative translations
	for (int i = 0; i < 10; i++) {
		r.OffsetBy(0, 70);
		TranslateBy(0, -35);
		DrawButtonFrames(r, update);
	}

	PopState();
	r = BRect(20, 30, 80, 60);

	// Scale
	PushState();
	ScaleBy(2, 2);
	TranslateBy(350 / 2, 0);
	DrawButtonFrames(r, update);

	PopState();

	// Rotation
	TranslateBy(420, 110);
	RotateBy(M_PI / 4);
	DrawButtonFrames(r, update);
}


void
View::DrawButtonFrames(BRect r, BRect update)
{
	BRect r1 = r;
	be_control_look->DrawButtonFrame(this, r1, update,
		ui_color(B_PANEL_BACKGROUND_COLOR),
		ui_color(B_PANEL_BACKGROUND_COLOR));
	be_control_look->DrawButtonBackground(this, r1, update,
		ui_color(B_PANEL_BACKGROUND_COLOR));

	MovePenTo(r1.left + 5, r1.top + 15);
	DrawString("BUTTON");

	r.OffsetBy(70, 0);
	r1 = r;
	be_control_look->DrawButtonFrame(this, r1, update,
		ui_color(B_PANEL_BACKGROUND_COLOR),
		ui_color(B_PANEL_BACKGROUND_COLOR), BControlLook::B_DISABLED);
	be_control_look->DrawButtonBackground(this, r1, update,
		ui_color(B_PANEL_BACKGROUND_COLOR), BControlLook::B_DISABLED);

	MovePenTo(r1.left + 5, r1.top + 15);
	DrawString("Disabled");

	r.OffsetBy(70, 0);
	r1 = r;
	be_control_look->DrawButtonFrame(this, r1, update,
		ui_color(B_PANEL_BACKGROUND_COLOR),
		ui_color(B_PANEL_BACKGROUND_COLOR), BControlLook::B_ACTIVATED);
	be_control_look->DrawButtonBackground(this, r1, update,
		ui_color(B_PANEL_BACKGROUND_COLOR), BControlLook::B_ACTIVATED);

	MovePenTo(r1.left + 5, r1.top + 15);
	DrawString("Active");

	r.OffsetBy(70, 0);
	r1 = r;
	be_control_look->DrawButtonFrame(this, r1, update,
		ui_color(B_PANEL_BACKGROUND_COLOR),
		ui_color(B_PANEL_BACKGROUND_COLOR),
		BControlLook::B_DISABLED | BControlLook:: B_ACTIVATED);
	be_control_look->DrawButtonBackground(this, r1, update,
		ui_color(B_PANEL_BACKGROUND_COLOR),
		BControlLook::B_DISABLED | BControlLook::B_ACTIVATED);

	MovePenTo(r1.left + 5, r1.top + 15);
	DrawString("Act+Disa");

	r.OffsetBy(70, 0);
	r1 = r;
	be_control_look->DrawButtonFrame(this, r1, update, 8,
		ui_color(B_PANEL_BACKGROUND_COLOR),
		ui_color(B_PANEL_BACKGROUND_COLOR), 0);
	be_control_look->DrawButtonBackground(this, r1, update, 8,
		ui_color(B_PANEL_BACKGROUND_COLOR), 0);

	MovePenTo(r1.left + 5, r1.top + 15);
	DrawString("Rounded");

	/* TODO test various border sets
	 * TODO test with multiple radius on each corner (including 0 on some)
	 */
}


class Window : public BWindow {
	public:
						Window();

		virtual	bool	QuitRequested();
};


Window::Window()
	: BWindow(BRect(50, 100, 1150, 500), "ControlLook-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	AddChild(new View(Bounds()));

	// TODO Add transformed variations:
	// - Normal
	// - Translated
	// - Scaled
	// - Rotated
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
	: BApplication("application/x-vnd.haiku-test-controllook")
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

