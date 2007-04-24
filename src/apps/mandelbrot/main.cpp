//******************************************************************************
//
//	File:		main.cpp
//
//******************************************************************************

/*
	Copyright 1993-1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Debug.h>

#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _ROSTER_H
#include <Roster.h>
#endif
#ifndef	_BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _MENU_H
#include <Menu.h>
#endif
#ifndef _MENU_BAR_H
#include <MenuBar.h>
#endif
#ifndef	_SCROLL_VIEW_H
#include <ScrollView.h>
#endif
#ifndef	_ALERT_H
#include <Alert.h>
#endif

#include <stdio.h>


#include "tsb.h"

#include <math.h>

/*------------------------------------------------------------*/
/* Those are the menu item id's of the main window	      */

#define	P1		0x60
#define	P2		0x61
#define	P3		0x62
#define	P4		0x63

/*------------------------------------------------------------*/

class	TMainWindow : public BWindow {
public:
					TMainWindow(BRect bound, char *name, window_type type,
				    			long flags);
virtual		void	MessageReceived(BMessage *an_event);
virtual		void	FrameResized(float, float);
virtual		bool	QuitRequested();

virtual		void	UpdateScrollBars();

private:
		TShowBit	*the_view;
};

/*------------------------------------------------------------*/

TMainWindow::TMainWindow(BRect bound, char *name, window_type type, long flags)
	: BWindow(bound, name, type, flags)
{
	BMenu		*a_menu;
	BRect		a_rect;
	BScrollView	*scroll_view;
	BMenuBar	*menubar;
	TShowBit	*my_view;
	BMenuItem	*item;

	Lock();

	a_rect.Set( 0, 0, 1000, 15);
	menubar = new BMenuBar(a_rect, "MB");
	
	a_menu = new BMenu("File");
	a_menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	menubar->AddItem(a_menu);

	a_menu = new BMenu("Palette");
	a_menu->AddItem(new BMenuItem("Palette1", new BMessage(P1)));
	a_menu->AddItem(new BMenuItem("Palette2", new BMessage(P2)));
	a_menu->AddItem(item = new BMenuItem("Palette3", new BMessage(P3)));
	a_menu->AddItem(new BMenuItem("Palette4", new BMessage(P4)));
	menubar->AddItem(a_menu);

	item->SetMarked(TRUE);
	a_menu->SetRadioMode(TRUE);
	
	a_menu = new BMenu("Iterations");
	a_menu->AddItem(new BMenuItem("128", new BMessage(128)));
	a_menu->AddItem(item = new BMenuItem("256", new BMessage(256)));
	a_menu->AddItem(new BMenuItem("384", new BMessage(384)));
	a_menu->AddItem(new BMenuItem("512", new BMessage(512)));
	a_menu->AddItem(new BMenuItem("768", new BMessage(768)));
	a_menu->AddItem(new BMenuItem("1024", new BMessage(1024)));
	menubar->AddItem(a_menu);

	item->SetMarked(TRUE);
	a_menu->SetRadioMode(TRUE);

	AddChild(menubar);
	float mb_height = menubar->Bounds().Height();

	a_rect.Set(0, mb_height + 1, 188 - B_V_SCROLL_BAR_WIDTH,
									 188 - B_H_SCROLL_BAR_HEIGHT);
	the_view = my_view = new TShowBit(a_rect, B_FOLLOW_ALL | B_WILL_DRAW);
	scroll_view = new BScrollView("scroll view", my_view, B_FOLLOW_ALL, B_WILL_DRAW,
		TRUE, TRUE, B_NO_BORDER);
//	my_view->ResizeBy(-1, -1);
	AddChild(scroll_view);
	SetSizeLimits(80, size_x + 13, 80 + 20, size_y + mb_height + 1 + 13);
	ResizeTo(size_x + 13, size_y + mb_height + 1 + 13);
	SetPulseRate(150000);
	UpdateScrollBars();
	Show();
	Unlock();
}

/*------------------------------------------------------------*/

void TMainWindow::UpdateScrollBars()
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

//------------------------------------------------------------
void TMainWindow::FrameResized(float, float)
{
	UpdateScrollBars();
}

/*------------------------------------------------------------*/

bool	TMainWindow::QuitRequested()
{
	if (the_view->busy) {
		the_view->exit_now = TRUE;
		PostMessage(B_QUIT_REQUESTED);
		return FALSE;
	}
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}

/*------------------------------------------------------------*/

void	TMainWindow::MessageReceived(BMessage *an_event)
{

	switch(an_event->what) {
		case	P1 :
		case	P2 :
		case	P3 :
		case	P4 :
			the_view->set_palette(an_event->what - P1);
			break;

		case	128  :
		case	256  :
		case	384  :
		case	512  :
		case	768  :
		case	1024 :
			the_view->set_iter(an_event->what);
			break;
		
		default:
			inherited::MessageReceived(an_event);
			break;
	}
}

/*------------------------------------------------------------*/

int main(int, char**)
{
	BApplication	*my_app;
	TMainWindow	*a_window;
	BRect		a_rect;

	set_thread_priority(find_thread(NULL), B_DISPLAY_PRIORITY);
	
	my_app = new BApplication("application/x-vnd.Be-MAND");

	a_rect.Set(100, 100, 288, 288);
	a_window = new TMainWindow(a_rect, "Mandelbrot", B_DOCUMENT_WINDOW,
		B_WILL_ACCEPT_FIRST_CLICK);
	my_app->Run();

	delete my_app;
}
