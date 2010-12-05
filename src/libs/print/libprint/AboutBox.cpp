/*
 * AboutBox.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <cstdio>
#include <string>

#include <Window.h>
#include <View.h>
#include <Button.h>

#include "AboutBox.h"


using namespace std;


enum {
	kMsgOK = 'AbOK'
};


class AboutBoxView : public BView {
public:
	AboutBoxView(BRect frame, const char *driver_name, const char *version, const char *copyright);
	virtual void Draw(BRect);
	virtual void AttachedToWindow();

private:
	string fDriverName;
	string fVersion;
	string fCopyright;
};

AboutBoxView::AboutBoxView(BRect rect, const char *driver_name, const char *version, const char *copyright)
	: BView(rect, "", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fDriverName = driver_name;
	fVersion     = version;
	fCopyright   = copyright;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetDrawingMode(B_OP_SELECT);
}

void AboutBoxView::Draw(BRect)
{
	SetHighColor(0, 0, 0);
	DrawString(fDriverName.c_str(), BPoint(10.0f, 16.0f));
	DrawString(" Driver for ");
	SetHighColor(0, 0, 0xff);
	DrawString("B");
	SetHighColor(0xff, 0, 0);
	DrawString("e");
	SetHighColor(0, 0, 0);
	DrawString("OS  Version ");
	DrawString(fVersion.c_str());
	DrawString(fCopyright.c_str(), BPoint(10.0f, 30.0f));
}

void AboutBoxView::AttachedToWindow()
{
	BRect rect;
	rect.Set(110, 50, 175, 55);
	BButton *button = new BButton(rect, "", "OK", new BMessage(kMsgOK));
	AddChild(button);
	button->MakeDefault(true);
}

class AboutBoxWindow : public BWindow {
public:
	AboutBoxWindow(BRect frame, const char *driver_name, const char *version, const char *copyright); 
	virtual void MessageReceived(BMessage *msg);
	virtual	bool QuitRequested();
};

AboutBoxWindow::AboutBoxWindow(BRect frame, const char *driver_name, const char *version, const char *copyright)
	: BWindow(frame, "", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_CLOSE_ON_ESCAPE)
{
	char title[256];
	sprintf(title, "About %s Driver", driver_name);
	SetTitle(title);
	AddChild(new AboutBoxView(Bounds(), driver_name, version, copyright));
}

void AboutBoxWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kMsgOK:
		be_app->PostMessage(B_QUIT_REQUESTED);
		break;
	}
}

bool AboutBoxWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

AboutBox::AboutBox(const char *signature, const char *driver_name, const char *version, const char *copyright)
	: BApplication(signature)
{
	BRect rect;
	rect.Set(100, 80, 400, 170);
	AboutBoxWindow *window = new AboutBoxWindow(rect, driver_name, version, copyright);
	window->Show();
}
