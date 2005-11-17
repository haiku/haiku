/*
 * Copyright 2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>
#include <Box.h>
#include <CheckBox.h>
#include <String.h>
#include <TextControl.h>
#include <Window.h>

#include <stdio.h>


static const uint32 kMsgPointerEvents = 'pevt';
static const rgb_color kPermanentColor = { 130, 130, 220};
static const rgb_color kPressedColor = { 220, 120, 120};


class PositionView : public BBox {
	public:
		PositionView(BRect rect, uint32 options);
		virtual ~PositionView();

		void SetPermanent(bool permanent);

		virtual void Draw(BRect updateRect);
		virtual void MouseDown(BPoint where);
		virtual void MouseUp(BPoint where);
		virtual void MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage);

	private:
		bool	fPermanent;
		uint32	fOptions;
		BPoint	fPosition;
};


PositionView::PositionView(BRect rect, uint32 options)
	: BBox(rect, "event mask", B_FOLLOW_ALL, B_WILL_DRAW),
	fPermanent(false),
	fOptions(options),
	fPosition(-1, -1)
{
}


PositionView::~PositionView()
{
}


void
PositionView::SetPermanent(bool permanent)
{
	if (permanent) {
		SetEventMask(B_POINTER_EVENTS, 0);
		SetViewColor(kPermanentColor);
	} else {
		SetEventMask(0, 0);
		SetViewColor(Parent()->ViewColor());
	}

	fPermanent = permanent;

	SetLowColor(ViewColor());
	Invalidate();
}


void
PositionView::Draw(BRect updateRect)
{
	BBox::Draw(updateRect);

	BString type;
	if (fOptions & B_SUSPEND_VIEW_FOCUS) {
		type << "suspend";
		if (fOptions & ~B_SUSPEND_VIEW_FOCUS)
			type << " & ";
	}
	if (fOptions & B_LOCK_WINDOW_FOCUS)
		type << "lock";
	if (fOptions == 0)
		type << "none";

	DrawString(type.String(), BPoint(6, 14));

	BString position;
	if (fPosition.x >= 0)
		position << (int32)fPosition.x;
	else
		position << "-";

	position << " : ";

	if (fPosition.y >= 0)
		position << (int32)fPosition.y;
	else
		position << "-";

	DrawString(position.String(), BPoint(6, 28));
}


void
PositionView::MouseDown(BPoint where)
{
	if (fOptions != 0)
		SetMouseEventMask(B_POINTER_EVENTS, fOptions);

	SetViewColor(kPressedColor);
	SetLowColor(ViewColor());
	fPosition = where;
	Invalidate();
}


void
PositionView::MouseUp(BPoint where)
{
	if (fPermanent) {
		SetViewColor(kPermanentColor);
	} else {
		SetViewColor(Parent()->ViewColor());
	}

	SetLowColor(ViewColor());
	fPosition = where;
	Invalidate();
}


void
PositionView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	fPosition = where;
	
	BRect rect = Bounds().InsetByCopy(5, 5);
	rect.bottom = 33;
	Invalidate(rect);
}


//	#pragma mark -


class Window : public BWindow {
	public:
		Window();
		virtual ~Window();
		
		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage* message);

	private:
		PositionView* fViews[4];
};


Window::Window()
	: BWindow(BRect(100, 100, 590, 260), "EventMask-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BView* view = new BView(Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	BTextControl* textControl = new BTextControl(BRect(10, 10, 290, 30),
		"text", "Type to test focus suspending:", "", NULL, B_FOLLOW_LEFT_RIGHT);
	textControl->SetDivider(textControl->StringWidth(textControl->Label()) + 8);
	view->AddChild(textControl);

	textControl = new BTextControl(BRect(300, 10, 420, 30),
		"all", "All keys:", "", NULL, B_FOLLOW_LEFT_RIGHT);
	textControl->SetDivider(textControl->StringWidth(textControl->Label()) + 8);
	view->AddChild(textControl);
	textControl->TextView()->SetEventMask(B_KEYBOARD_EVENTS, 0);

	BRect rect(10, 40, 120, 120);
	for (int32 i = 0; i < 4; i++) {
		uint32 options = 0;
		switch (i) {
			case 1:
				options = B_SUSPEND_VIEW_FOCUS;
				break;
			case 2:
				options = B_LOCK_WINDOW_FOCUS;
				break;
			case 3:
				options = B_SUSPEND_VIEW_FOCUS | B_LOCK_WINDOW_FOCUS;
				break;
		}

		fViews[i] = new PositionView(rect, options);
		view->AddChild(fViews[i]);

		rect.OffsetBy(120, 0);
	}

	BCheckBox* checkBox = new BCheckBox(BRect(10, 130, 200, 160), "permanent", "Get all pointer events", new BMessage(kMsgPointerEvents));
	view->AddChild(checkBox);
}


Window::~Window()
{
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
Window::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgPointerEvents:
		{
			bool selected = message->FindInt32("be:value") != 0;

			for (int32 i = 0; i < 4; i++) {
				fViews[i]->SetPermanent(selected);
			}
			break;
		}
		
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


//	#pragma mark -


class Application : public BApplication {
	public:
		Application();

		virtual void ReadyToRun(void);
};


Application::Application()
	: BApplication("application/x-vnd.haiku-view_state")
{
}


void
Application::ReadyToRun(void)
{
	Window *window = new Window();
	window->Show();
}


//	#pragma mark -


int 
main(int argc, char **argv)
{
	Application app;// app;

	app.Run();
	return 0;
}
