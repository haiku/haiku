/*
	Copyright 1993-1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "tsb.h"

#include <Debug.h>

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <OS.h>
#include <Roster.h>
#include <ScrollView.h>

#include <math.h>
#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Mandelbrot"


/* Those are the menu item id's of the main window */
#define	P1		0x60
#define	P2		0x61
#define	P3		0x62
#define	P4		0x63

class TMainWindow : public BWindow {
	public:
		TMainWindow(BRect bound, const char* name, window_type type,
			long flags);
		virtual ~TMainWindow();

		virtual void MessageReceived(BMessage* message);
		virtual void FrameResized(float width, float height);
		virtual bool QuitRequested();

		void UpdateScrollBars();

	private:
		TShowBit*	fView;
};


TMainWindow::TMainWindow(BRect bound, const char* name, window_type type,
	long flags)
	: BWindow(bound, name, type, flags)
{
	BMenuBar* menuBar = new BMenuBar(BRect(0, 0, 1000, 15), "MB");
	BMenuItem* item;
	BMenu* menu;

	menu = new BMenu(B_TRANSLATE("File"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	menuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Palette"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Palette 1"), new BMessage(P1)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Palette 2"), new BMessage(P2)));
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Palette 3"),
		new BMessage(P3)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Palette 4"), new BMessage(P4)));
	menuBar->AddItem(menu);

	item->SetMarked(true);
	menu->SetRadioMode(true);

	menu = new BMenu(B_TRANSLATE("Iterations"));
	menu->AddItem(new BMenuItem("128", new BMessage(128)));
	menu->AddItem(item = new BMenuItem("256", new BMessage(256)));
	menu->AddItem(new BMenuItem("384", new BMessage(384)));
	menu->AddItem(new BMenuItem("512", new BMessage(512)));
	menu->AddItem(new BMenuItem("768", new BMessage(768)));
	menu->AddItem(new BMenuItem("1024", new BMessage(1024)));
	menu->AddItem(new BMenuItem("2048", new BMessage(2048)));
	menu->AddItem(new BMenuItem("4096", new BMessage(4096)));
	menu->AddItem(new BMenuItem("8192", new BMessage(8192)));
	menuBar->AddItem(menu);

	item->SetMarked(true);
	menu->SetRadioMode(true);

	AddChild(menuBar);
	float barHeight = menuBar->Bounds().Height();

	fView = new TShowBit(BRect(0, barHeight + 1, 188 - B_V_SCROLL_BAR_WIDTH,
		188 - B_H_SCROLL_BAR_HEIGHT), B_FOLLOW_ALL, B_WILL_DRAW);
	BScrollView	*scrollView = new BScrollView("scroll view", fView,
		B_FOLLOW_ALL, B_WILL_DRAW, true, true, B_NO_BORDER);
	AddChild(scrollView);

	SetSizeLimits(80, size_x + 13, 80 + 20, size_y + barHeight + 1 + 13);
	ResizeTo(size_x + 13, size_y + barHeight + 1 + 13);
	SetPulseRate(150000);
	UpdateScrollBars();
}


TMainWindow::~TMainWindow()
{
}


void
TMainWindow::UpdateScrollBars()
{
	BScrollView*	scrollview;
	BScrollBar*		scrollbar;
	BRect			visible_extent;
	BRect			total_extent;
	BRect			bound;
	BRect			my_bounds;
	long			max;

	Lock();
	if ((scrollview = (BScrollView*)FindView("scroll view"))) {
		bound.Set(0, 0, size_x, size_y);
		my_bounds = Bounds();

		visible_extent = bound & my_bounds;
		total_extent = bound | my_bounds;

		scrollbar = scrollview->ScrollBar(B_HORIZONTAL);
		max = (long) (bound.Width() - my_bounds.Width());
		if (max < 0)
			max = 0;
		scrollbar->SetRange(0, max);
		scrollbar->SetProportion(visible_extent.Width() / total_extent.Width());

		scrollbar = scrollview->ScrollBar(B_VERTICAL);
		max = (long) (bound.Height() - my_bounds.Height());
		if (max < 0)
			max = 0;
		scrollbar->SetRange(0, max);
		scrollbar->SetProportion(visible_extent.Height() / total_extent.Height());
	}
	Unlock();
}


void
TMainWindow::FrameResized(float, float)
{
	UpdateScrollBars();
}


bool
TMainWindow::QuitRequested()
{
	if (fView->busy) {
		fView->exit_now = true;
		PostMessage(B_QUIT_REQUESTED);
		return false;
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
TMainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case P1:
		case P2:
		case P3:
		case P4:
			fView->set_palette(message->what - P1);
			break;

		case 128:
		case 256:
		case 384:
		case 512:
		case 768:
		case 1024:
		case 2048:
		case 4096:
		case 8192:
			fView->set_iter(message->what);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


//	#pragma mark -


int
main(int, char**)
{
	BApplication* app = new BApplication("application/x-vnd.Haiku-Mandelbrot");

	BWindow* window = new TMainWindow(BRect(100, 100, 288, 288),
		B_TRANSLATE_SYSTEM_NAME("Mandelbrot"), B_DOCUMENT_WINDOW,
		B_WILL_ACCEPT_FIRST_CLICK);
	window->Show();

	app->Run();
	delete app;
	return 0;
}
