// main.cpp

#include <stdio.h>

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <String.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>

class HelloView : public BView {
 public:
					HelloView(BRect frame, const char* name,
							  uint32 resizeFlags, uint32 flags);

	virtual	void	AttachedToWindow();

	virtual	void	Draw(BRect updateRect);

	virtual	void	FrameMoved(BPoint were);
	virtual	void	FrameResized(float width, float height);

	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseUp(BPoint where);
	virtual	void	MouseMoved(BPoint where, uint32 transit,
							   const BMessage* dragMessage);
 private:
			bool	fTracking;
			BPoint	fTrackingStart;
			BPoint	fLastMousePos;
};

// constructor
HelloView::HelloView(BRect frame, const char* name,
					 uint32 resizeFlags, uint32 flags)
	: BView(frame, name, resizeFlags, flags | B_FRAME_EVENTS),
	  fTracking(false),
	  fTrackingStart(-1.0, -1.0),
	  fLastMousePos(-1.0, -1.0)
{
// TODO: Find bug: this influences StringWidth(), but the font is not used
//	SetFontSize(9);
	rgb_color bg = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_1_TINT);
	SetViewColor(bg);
	SetLowColor(bg);

	BFont font;
	GetFont(&font);
	BString string("ß amM öo\t");
	int32 charCount = string.CountChars();
	float* escapements = new float[charCount];
	escapement_delta dummy;
	dummy.space = 0.0;
	dummy.nonspace = 0.0;
	font.GetEscapements(string.String(), charCount, &dummy, escapements);
	for (int32 i = 0; i < charCount; i++)
		printf("escapement: %.3f\n", escapements[i]);
	delete[] escapements;
}

// AttachedToWindow
void
HelloView::AttachedToWindow()
{
}

// Draw
void
HelloView::Draw(BRect updateRect)
{
	rgb_color noTint = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color shadow = tint_color(noTint, B_DARKEN_2_TINT);
	rgb_color light = tint_color(noTint, B_LIGHTEN_MAX_TINT);

	BRect r(Bounds());

	BeginLineArray(4);
	AddLine(BPoint(r.left, r.top),
			BPoint(r.right, r.top), shadow);
	AddLine(BPoint(r.right, r.top + 1),
			BPoint(r.right, r.bottom), light);
	AddLine(BPoint(r.right - 1, r.bottom),
			BPoint(r.left, r.bottom), light);
	AddLine(BPoint(r.left, r.bottom - 1),
			BPoint(r.left, r.top + 1), shadow);
	EndLineArray();

	SetHighColor(255, 0, 0, 128);

	const char* message = "Click and drag";
	float width = StringWidth(message);
	BPoint p(r.left + r.Width() / 2.0 - width / 2.0,
			 r.top + r.Height() / 2.0);

	DrawString(message, p);

	message = "to draw a line!";
	width = StringWidth(message);
	p.x = r.left + r.Width() / 2.0 - width / 2.0;
	p.y += 20;

	DrawString(message, p);

//						SetDrawingMode(B_OP_OVER);
SetDrawingMode(B_OP_ALPHA);
SetPenSize(10.0);
	SetHighColor(0, 80, 255, 100);
	StrokeLine(fTrackingStart, fLastMousePos);
// TODO: this should not be necessary with proper state stack
// (a new state is pushed before Draw() is called)
SetDrawingMode(B_OP_COPY);
}

// FrameMoved
void
HelloView::FrameMoved(BPoint were)
{
	printf("HelloView::FrameMoved()\n");
	if (Window()) {
		Window()->CurrentMessage()->PrintToStream();
	}
}

// FrameResized
void
HelloView::FrameResized(float width, float height)
{
	printf("HelloView::FrameResized()\n");
	if (Window()) {
		Window()->CurrentMessage()->PrintToStream();
	}
}

// MouseDown
void
HelloView::MouseDown(BPoint where)
{
	fTracking = true;
	fTrackingStart = fLastMousePos = where;
	Invalidate();
}

// MouseUp
void
HelloView::MouseUp(BPoint where)
{
	fTracking = false;
}

// MouseMoved
void
HelloView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fTracking) {
		BRect invalidBefore(min_c(fTrackingStart.x, fLastMousePos.x),
							min_c(fTrackingStart.y, fLastMousePos.y),
							max_c(fTrackingStart.x, fLastMousePos.x),
							max_c(fTrackingStart.y, fLastMousePos.y));
		fLastMousePos = where;
		BRect invalidAfter(min_c(fTrackingStart.x, fLastMousePos.x),
							min_c(fTrackingStart.y, fLastMousePos.y),
							max_c(fTrackingStart.x, fLastMousePos.x),
							max_c(fTrackingStart.y, fLastMousePos.y));
		BRect invalid(invalidBefore | invalidAfter);
		invalid.InsetBy(-10.0, -10.0);
		Invalidate(invalid);
	}
}


// show_window
void
show_window(BRect frame, const char* name)
{
//	BRect f(0, 0, frame.Width(), frame.Height());
	BRect f(frame);
	BWindow* window = new BWindow(f, name,
								  B_TITLED_WINDOW,
								  B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BRect b(window->Bounds());

	b.bottom = b.top + 8;
	BMenuBar* menuBar = new BMenuBar(b, "menu bar");
	window->AddChild(menuBar);

	BMenu* menu = new BMenu("Menus");
	menuBar->AddItem(menu);

	BMenuItem* menuItem = new BMenuItem("Quit", NULL, 'Q');
	menu->AddItem(menuItem);

	menuBar->AddItem(new BMenu("don't"));
	menuBar->AddItem(new BMenu("work!"));
	menuBar->AddItem(new BMenu("(yet)"));

	b = window->Bounds();
	b.top = menuBar->Bounds().bottom + 1;
	BBox* bg = new BBox(b, "box", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);

	window->AddChild(bg);

	b = bg->Bounds();
	// occupy the right side of the window
	b.Set((b.left + b.right) / 2.0 + 3.0, b.top + 5.0, b.right - 5.0, b.bottom - 5.0);
	BView* view = new HelloView(b, "hello view", B_FOLLOW_ALL,
								B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);

	bg->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	bg->AddChild(view);

	b = bg->Bounds();
	// occupy the left side of the window
	b.Set(b.left + 5.0, b.top + 5.0, (b.left + b.right) / 2.0 - 2.0, b.bottom - 5.0);
	BBox* controlGroup = new BBox(b, "controls box", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,
								  B_WILL_DRAW, B_FANCY_BORDER);

	controlGroup->SetLabel("Controls");
	bg->AddChild(controlGroup);

	b = controlGroup->Bounds();
	b.top += 15.0;
	b.bottom = b.top + 25.0;
	b.InsetBy(5.0, 5.0);
	BRadioButton* radioButton1 = new BRadioButton(b, "radio 1", "Choice One", NULL);
	controlGroup->AddChild(radioButton1);

	b.OffsetBy(0, radioButton1->Bounds().Height() + 5.0);
	BRadioButton* radioButton2 = new BRadioButton(b, "radio 2", "Choice Two", NULL);
	controlGroup->AddChild(radioButton2);

	b.OffsetBy(0, radioButton2->Bounds().Height() + 5.0);
	BRadioButton* radioButton3 = new BRadioButton(b, "radio 3", "Choice Three", NULL);
	controlGroup->AddChild(radioButton3);

	radioButton1->SetValue(B_CONTROL_ON);

	// button
	b.OffsetBy(0, radioButton3->Bounds().Height() + 5.0);
	BButton* button = new BButton(b, "button", "Button", NULL);
	controlGroup->AddChild(button);

	// text control
	b.OffsetBy(0, button->Bounds().Height() + 5.0);
	BTextControl* textControl = new BTextControl(b, "text control", "Text", "<enter text here>", NULL);
	textControl->SetDivider(textControl->StringWidth(textControl->Label()) + 10.0);
	controlGroup->AddChild(textControl);

/*	BRect textRect(b);
	textRect.OffsetTo(0.0, 0.0);
	BTextView* textView = new BTextView(b, "text view", textRect,
										B_FOLLOW_ALL, B_WILL_DRAW);
	view->AddChild(textView);
	textView->SetText("This is a BTextView.");
	textView->MakeFocus(true);*/

	// check box
	b.OffsetBy(0, textControl->Bounds().Height() + 5.0);
	BCheckBox* checkBox = new BCheckBox(b, "check box", "CheckBox", NULL);
	controlGroup->AddChild(checkBox);

//	window->MoveTo(frame.left, frame.top);
	window->Show();
}

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x.vnd-Haiku.windows_test");

	BRect frame(80.0, 100.0, 450.0, 400.0);
	show_window(frame, "Playground");

//	frame.Set(50.0, 50.0, 200.0, 250.0);
//	show_window(frame, "Window #2");

	app->Run();

	delete app;
	return 0;
}
