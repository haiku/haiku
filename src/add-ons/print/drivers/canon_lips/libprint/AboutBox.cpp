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

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

class AboutBoxView : public BView {
public:
	AboutBoxView(BRect frame, const char *driver_name, const char *version, const char *copyright);
	virtual void Draw(BRect);
	virtual void AttachedToWindow();

private:
	string __driver_name;
	string __version;
	string __copyright;
};

AboutBoxView::AboutBoxView(BRect rect, const char *driver_name, const char *version, const char *copyright)
	: BView(rect, "", B_FOLLOW_ALL, B_WILL_DRAW)
{
	__driver_name = driver_name;
	__version     = version;
	__copyright   = copyright;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetDrawingMode(B_OP_SELECT);
}

void AboutBoxView::Draw(BRect)
{
	SetHighColor(0, 0, 0);
	DrawString(__driver_name.c_str(), BPoint(10.0f, 16.0f));
	DrawString(" Driver for ");
	SetHighColor(0, 0, 0xff);
	DrawString("B");
	SetHighColor(0xff, 0, 0);
	DrawString("e");
	SetHighColor(0, 0, 0);
	DrawString("OS  Version ");
	DrawString(__version.c_str());
	DrawString(__copyright.c_str(), BPoint(10.0f, 30.0f));
}

#define M_OK 1

void AboutBoxView::AttachedToWindow()
{
	BRect rect;
	rect.Set(110, 50, 175, 55);
	BButton *button = new BButton(rect, "", "OK", new BMessage(M_OK));
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
	: BWindow(frame, "", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	char title[256];
	sprintf(title, "About %s Driver", driver_name);
	SetTitle(title);
	AddChild(new AboutBoxView(Bounds(), driver_name, version, copyright));
}

void AboutBoxWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case M_OK:
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
