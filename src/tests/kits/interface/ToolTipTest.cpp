/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Box.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <StringView.h>
#include <Window.h>

#include <ToolTip.h>

#include <stdio.h>


class CustomToolTip : public BToolTip {
public:
	CustomToolTip()
	{
		fView = new BStringView("", "Custom tool tip!");
		fView->SetFont(be_bold_font);
		fView->SetHighColor(255, 0, 0);
	}

	virtual ~CustomToolTip()
	{
		delete fView;
	}

	virtual BView* View() const
	{
		return fView;
	}

private:
	BStringView*	fView;
};


class ChangingView : public BStringView {
public:
	ChangingView()
		:
		BStringView("changing", ""),
		fNumber(0)
	{
	}

	virtual ~ChangingView()
	{
	}

	virtual void AttachedToWindow()
	{
		BStringView::AttachedToWindow();

		BMessage update('updt');
		fRunner = new BMessageRunner(this, &update, 100000);
	}

	virtual void DetachedFromWindow()
	{
		delete fRunner;
	}

	virtual void MessageReceived(BMessage* message)
	{
		if (message->what == 'updt') {
			char text[42];
			snprintf(text, sizeof(text), "%d", ++fNumber);
			SetText(text);
		}
	}

private:
	BMessageRunner*	fRunner;
	int				fNumber;
};


class ChangingToolTip : public BToolTip {
public:
	ChangingToolTip()
	{
		fView = new ChangingView();
		fView->SetFont(be_bold_font);
	}

	virtual ~ChangingToolTip()
	{
		delete fView;
	}

	virtual BView* View() const
	{
		return fView;
	}

private:
	BStringView*	fView;
};


class MouseView : public BStringView {
public:
	MouseView()
		:
		BStringView("mouse", "x:y")
	{
	}

	virtual ~MouseView()
	{
	}

	virtual void AttachedToWindow()
	{
		BStringView::AttachedToWindow();
		SetEventMask(B_POINTER_EVENTS, 0);
	}

	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
	{
		ConvertToScreen(&where);

		char text[42];
		snprintf(text, sizeof(text), "%g:%g", where.x, where.y);
		SetText(text);
	}
};


class MouseToolTip : public BToolTip {
public:
	MouseToolTip()
	{
		fView = new MouseView();
		SetSticky(true);
	}

	virtual ~MouseToolTip()
	{
		delete fView;
	}

	virtual BView* View() const
	{
		return fView;
	}

private:
	BStringView*	fView;
};


class ImmediateView : public BStringView {
public:
	ImmediateView(const char* name, const char* label)
		:
		BStringView(name, label)
	{
		SetToolTip("Easy but immediate!");
		ToolTip()->SetSticky(true);
	}

	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
	{
		if (transit == B_ENTERED_VIEW)
			ShowToolTip(ToolTip());
	}
};


class Window : public BWindow {
public:
							Window();

	virtual	bool			QuitRequested();
};


class Application : public BApplication {
public:
							Application();

	virtual	void			ReadyToRun();
};


//	#pragma mark -


Window::Window()
	:
	BWindow(BRect(100, 100, 520, 430), "ToolTip-Test",
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BView* simple = new BStringView("1", "Simple Tool Tip");
	simple->SetToolTip("This is a really\nsimple tool tip!");

	BView* custom = new BStringView("2", "Custom Tool Tip");
	custom->SetToolTip(new CustomToolTip());

	BView* changing = new BStringView("3", "Changing Tool Tip");
	changing->SetToolTip(new ChangingToolTip());

	BView* mouse = new BStringView("3", "Mouse Tool Tip (sticky)");
	mouse->SetToolTip(new MouseToolTip());

	BView* immediate = new ImmediateView("3", "Immediate Tool Tip (sticky)");

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(simple)
		.Add(custom)
		.Add(changing)
		.Add(mouse)
		.Add(immediate);
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark -


Application::Application()
	: BApplication("application/x-vnd.haiku-tooltiptest")
{
}


void
Application::ReadyToRun()
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

