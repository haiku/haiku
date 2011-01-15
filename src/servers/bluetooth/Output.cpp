/*
 * Copyright 2011 Hamish Morrison, hamish@lavabit.com
 * Copyright 2010 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2010 Dan-Matei Epure, mateiepure@gmail.com
 * Copyright BeNet Team (Original Project)
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _Output_h
#include "Output.h"
#endif

#include <Looper.h>
#include <String.h>

#include <stdio.h>

/*
#ifndef _Preferences_h
#include "Preferences.h"
#endif
*/


OutputView::OutputView(const char* name)
	:
	BGroupView(name, B_VERTICAL, B_WILL_DRAW)
{
	rgb_color textColor = {255, 255, 255};
	fTextView = new BTextView("Output", NULL, &textColor, B_WILL_DRAW);
	fTextView->SetViewColor(0, 0, 100);
	fTextView->MakeEditable(false);
	BScrollView* scrollView = new BScrollView("ScrollView", fTextView,
		0, false, true);
	
	BLayoutBuilder::Group<>(this).Add(scrollView);
}


// Singleton implementation
Output* Output::sInstance = 0;


Output::Output()
	:
	BWindow(BRect(200, 200, 800, 800), "Output",
		B_TITLED_WINDOW, B_NOT_ZOOMABLE)
{
	fOutputViewsList = new BList();

	fReset = new BButton("reset all", "Clear", 
		new BMessage(kMsgOutputReset), B_WILL_DRAW);
	fResetAll = new BButton("reset", "Clear all",
		new BMessage(kMsgOutputResetAll), B_WILL_DRAW);
	
	fTabView = new BTabView("tab_view", B_WIDTH_FROM_LABEL,
		B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW | B_NAVIGABLE_JUMP);

	fTabView->AddTab(fAll = new OutputView("All"));
	
	BLayoutBuilder::Group<>(this, B_VERTICAL, 5)
		.Add(fTabView)
		.AddGroup(B_HORIZONTAL, 0)
			.Add(fReset)
			.AddGlue()
			.Add(fResetAll)
		.End()
		.SetInsets(5, 5, 5, 5);
	
	/*
	MoveTo(	Preferences::Instance()->OutputX(),
			Preferences::Instance()->OutputY());
	*/
}


void
Output::AddTab(const char* text, uint32 index)
{
	OutputView* customOutput;

	Lock();
	fTabView->AddTab(customOutput = new OutputView(text));
	fTabView->Invalidate();
	Unlock();

	fOutputViewsList->AddItem(customOutput, index);
}


Output::~Output()
{
/*	BWindow::~BWindow();*/
	delete fOutputViewsList;
}


Output*
Output::Instance()
{
	if (!sInstance)
		sInstance = new Output;
	return sInstance;
}


bool
Output::QuitRequested()
{
	Hide();
	return false;
}


OutputView*
Output::_OutputViewForTab(int32 index)
{
	return (OutputView*)fTabView->TabAt(index)->View();
}


void
Output::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case kMsgOutputReset:
			_OutputViewForTab(fTabView->Selection())->Clear();
			break;
		case kMsgOutputResetAll:
			for (int32 index = 0; index < fTabView->CountTabs(); ++index)
				_OutputViewForTab(index)->Clear();
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


void
Output::FrameMoved(BPoint point)
{
/*	Preferences::Instance()->OutputX(point.x);
	Preferences::Instance()->OutputY(point.y);
*/
}


int
Output::Postf(uint32 index, const char* format, ...)
{
	char string[200];
	va_list arg;
	int done;

	va_start (arg, format);
	done = vsnprintf(string, 200, format, arg);
	va_end (arg);

	Post(string, index);

	return done;
}


void
Output::Post(const char* text, uint32 index)
{
	Lock();
	OutputView* view = (OutputView*)fOutputViewsList->ItemAt(index);

	if (view != NULL)
		_Add(text, view);
	else
		// Note that the view should be added before this!
		// Dropping twice to the main
		_Add(text, fAll);
	
	Unlock();
}


void
Output::_Add(const char* text, OutputView* view)
{	
	view->Add(text);
	fAll->Add(text);
}
