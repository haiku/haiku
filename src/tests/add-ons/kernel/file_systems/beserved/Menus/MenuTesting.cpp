// File Sharing Preferences Application
//
// FileSharing.cpp
//

// TODO:
// 1.  Make share properties panel modal but still work with file panel

#include "Application.h"
#include "Window.h"
#include "Alert.h"
#include "CheckBox.h"
#include "Button.h"
#include "TextControl.h"
#include "Menu.h"
#include "MenuBar.h"
#include "PopUpMenu.h"
#include "MenuItem.h"
#include "MenuField.h"
#include "Point.h"
#include "Mime.h"
#include "Roster.h"
#include "Resources.h"

#define MENU_BAR_HEIGHT		18.0

#define MSG_TEST_CANCEL		'TCan'
#define MSG_FILE_NEW		'FNew'
#define MSG_FILE_OPEN		'FOpn'
#define MSG_FILE_SAVE		'FSav'


class TestMenuItem : public BMenuItem
{
	public:
		TestMenuItem(char *title, BMessage *msg)
			: BMenuItem(title, msg)
		{
		}

		~TestMenuItem()
		{
		}

		void DrawContent()
		{
			Highlight(IsSelected());
		}

		void Highlight(bool flag)
		{
			rgb_color white = { 255, 255, 255, 255 };
			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
			rgb_color yellow = ui_color(B_WINDOW_TAB_COLOR);

			BPoint point = ContentLocation();
			BMenu *menu = Menu();
			BRect frame = Frame();

			if (!flag)
			{
				frame.right = 18;
				menu->SetLowColor(gray);
				menu->FillRect(frame, B_SOLID_LOW);
				frame = Frame();
				frame.left = 18;
			}
			else
			{
//				menu->SetLowColor(darkYellow);
//				menu->StrokeRect(frame, B_SOLID_LOW);
//				frame.InsetBy(1, 1);
			}

			menu->SetLowColor(flag ? yellow : white);
			menu->FillRect(frame, B_SOLID_LOW);
			menu->MovePenTo(point.x + 7, point.y + 10);
			menu->DrawString(Label());
		}
};

// ----- TestView ---------------------------------------------------------------

class TestView : public BView
{
	public:
		TestView(BRect rect)
			: BView(rect, "TestView", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
		{
			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
			SetViewColor(gray);

			BRect r;
			r.Set(337, 52, 412, 72);
			AddChild(new BButton(r, "CancelBtn", "Close", new BMessage(MSG_TEST_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));
		}

		~TestView()
		{
		}

		void Draw(BRect rect)
		{
			BRect r = Bounds();
			rgb_color black = { 0, 0, 0, 255 };
			rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);

			SetViewColor(gray);
			SetLowColor(gray);
			FillRect(r, B_SOLID_LOW);

			SetHighColor(black);
			SetFont(be_plain_font);
			SetFontSize(10);
			MovePenTo(15, 15);
			DrawString("");
		}

	private:
		BButton *deployBtn;
};

// ----- TestWindow -------------------------------------------------------------------

class TestWindow : public BWindow
{
	public:
		TestWindow()
		: BWindow(BRect(50, 50, 475, 460), "Menu Testing", B_TITLED_WINDOW, 0)
		{
			BRect r = Bounds();

			r = Bounds();
			testView = new TestView(r);
			AddChild(testView);

			r.bottom = MENU_BAR_HEIGHT;
			menuBar = new BMenuBar(r, "MenuBar");
			AddChild(menuBar);

			BMenu *menu = new BMenu("File");
			menuBar->AddItem(menu);

			menu->AddItem(new TestMenuItem("New", new BMessage(MSG_FILE_NEW)));
			menu->AddItem(new TestMenuItem("Open", new BMessage(MSG_FILE_OPEN)));
			menu->AddItem(new TestMenuItem("Save", new BMessage(MSG_FILE_SAVE)));

			Show();
		}

		// ~TestWindow()
		//
		~TestWindow()
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
		}

		// MessageReceived()
		//
		void MessageReceived(BMessage *msg)
		{
			switch (msg->what)
			{
				case MSG_TEST_CANCEL:
					Quit();
					break;

				default:
					BWindow::MessageReceived(msg);
					break;
			}
		}

	private:
		TestView *testView;
		BMenuBar *menuBar;
};


// ----- Application -------------------------------------------------------------------------

class TestApp : public BApplication
{
	public:
		TestApp()
		: BApplication("application/x-vnd.Teldar-MenuTesting")
		{
			win = new TestWindow;
		}

		~TestApp()
		{
		}

	private:
		TestWindow *win;
};

// main()
//
int main()
{
	new TestApp;
	be_app->Run();
	delete be_app;
}
