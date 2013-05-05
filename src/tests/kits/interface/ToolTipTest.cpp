/*
 * Copyright 2009-2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Box.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <String.h>
#include <StringView.h>
#include <Window.h>

#include <ToolTip.h>

#include <stdio.h>


class CustomToolTip : public BToolTip {
public:
	CustomToolTip(const char* text)
	{
		fView = new BStringView("", text);
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


class PulseStringView : public BStringView {
public:
	PulseStringView(const char* name, const char* label)
		:
		BStringView(name, label, B_WILL_DRAW | B_PULSE_NEEDED)
	{
	}

	virtual void Pulse()
	{
		char buffer[256];
		time_t now = time(NULL);
		strftime(buffer, sizeof(buffer), "%X", localtime(&now));
		SetToolTip(buffer);
	}
};


class PulseToolTipView : public BStringView {
public:
	PulseToolTipView(const char* name, const char* label)
		:
		BStringView(name, label, B_WILL_DRAW | B_PULSE_NEEDED),
		fToolTip(NULL),
		fCounter(0)
	{
	}

	virtual void Pulse()
	{
		if (fToolTip != NULL)
			fToolTip->ReleaseReference();

		BString text;
		text.SetToFormat("New tool tip every second! (%d)", fCounter++);

		fToolTip = new CustomToolTip(text.String());
		SetToolTip(fToolTip);
	}

private:
			BToolTip*		fToolTip;
			int				fCounter;
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
	custom->SetToolTip(new CustomToolTip("Custom tool tip!"));

	BView* changing = new BStringView("3", "Changing Tool Tip");
	changing->SetToolTip(new ChangingToolTip());

	BView* mouse = new BStringView("4", "Mouse Tool Tip (sticky)");
	mouse->SetToolTip(new MouseToolTip());

	BView* immediate = new ImmediateView("5", "Immediate Tool Tip (sticky)");

	BView* pulseString = new PulseStringView("pulseString",
		"Periodically changing tool tip text");

	BView* pulseToolTip = new PulseToolTipView("pulseToolTip",
		"Periodically changing tool tip");

	BGroupView* nested = new BGroupView();
	nested->SetViewColor(50, 50, 90);
	nested->GroupLayout()->SetInsets(30);
	nested->SetToolTip("The outer view has a tool tip,\n"
		"the inner one doesn't.");
	nested->AddChild(new BGroupView("inner"));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL)
			.Add(simple)
			.Add(custom)
			.Add(changing)
			.Add(mouse)
			.Add(immediate)
			.End()
		.AddGroup(B_VERTICAL)
			.Add(pulseString)
			.Add(pulseToolTip)
			.Add(nested);

	SetPulseRate(1000000LL);
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark -


Application::Application()
	:
	BApplication("application/x-vnd.haiku-tooltiptest")
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

