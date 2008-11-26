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

/****************************************************************/
/*	OutputView													*/
/****************************************************************/

OutputView::OutputView(BRect frame) :
	BView(frame, "OutputView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(216,216,216);
	rgb_color color = {255,255,255};

	BRect b = Bounds();
	AddChild(m_pTextView = new BTextView(BRect(b.left+5,b.top+5,b.right-B_V_SCROLL_BAR_WIDTH-6,b.bottom-5), "Output", BRect(b.left+5,b.top+5,b.right-B_V_SCROLL_BAR_WIDTH-6,b.bottom-5), NULL, &color, B_FOLLOW_ALL_SIDES, B_WILL_DRAW));
	m_pTextView->SetViewColor(0,0,100);
	m_pTextView->MakeEditable(false);

	AddChild(m_pScrollBar = new BScrollBar(BRect(b.right-B_V_SCROLL_BAR_WIDTH-5,b.top+5,b.right-5, b.bottom-5), "outputScroll", m_pTextView, 0, 0, B_VERTICAL));
}


void
OutputView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	m_pTextView->SetTextRect(BRect(0,0,width,height));
}


/****************************************************************/
/*	Output														*/
/****************************************************************/

// Singleton implementation
Output* Output::m_instance = 0;

Output::Output() :
	BWindow(BRect(200,200,600,600), "Output", B_TITLED_WINDOW, B_NOT_ZOOMABLE)
{
	BRect b = Bounds();

	fTabsList = new BList(20);
	fOutputViewsList = new BList(20);

	BView* resetView = new BView(BRect(b.left,b.bottom-25,b.right,b.bottom), "resetView", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT , B_WILL_DRAW);
	resetView->SetViewColor(216,216,216);
	resetView->AddChild(m_pReset = new BButton(BRect(1,1,61,20), "reset all", "Clear", new BMessage(MSG_OUTPUT_RESET), B_FOLLOW_BOTTOM));
	resetView->AddChild(m_pResetAll = new BButton(BRect(70,1,130,20), "reset", "Clear all", new BMessage(MSG_OUTPUT_RESET_ALL), B_FOLLOW_BOTTOM));
	AddChild(resetView);

	fTabView = new BTabView(BRect(b.left,b.top,b.right,b.bottom-25), "tab_view", B_WIDTH_FROM_LABEL ,B_FOLLOW_ALL, B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW | B_NAVIGABLE_JUMP );
	fTabView->SetViewColor(216,216,216); 

	fBounds = fTabView->Bounds(); 
	fBounds.bottom -= fTabView->TabHeight(); 

	fTabView->AddTab(m_pAll = new OutputView(fBounds), m_pAllTab = new BTab()); 
	m_pAllTab->SetLabel("All"); 

	AddChild(fTabView);
	/*
	MoveTo(	Preferences::Instance()->OutputX(),
			Preferences::Instance()->OutputY());
	*/
}

void
Output::AddTab(const char* text, int32 index) 
{
		OutputView* customOutput;
		BTab*		customTab;
		
		Lock();
		fTabView->AddTab(customOutput = new OutputView(fBounds), customTab = new BTab());
		customTab->SetLabel( text );
		fTabView->Invalidate();
		Unlock();		
		
		fTabsList->AddItem(customTab, index);
		fOutputViewsList->AddItem(customOutput, index);
		
}

Output::~Output()
{
/*	BWindow::~BWindow();*/
	
	delete fTabsList;
	delete fOutputViewsList;
}


Output*
Output::Instance()
{
	if (!m_instance)
		m_instance = new Output;
	return m_instance;
}


bool
Output::QuitRequested()
{
	Hide();
	return false;
}


void
Output::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
	case MSG_OUTPUT_RESET:
	
		for (int32 index = 0 ; index<fTabsList->CountItems() ; index++) {
			if (fTabsList->ItemAt(index) != NULL && ((BTab*)fTabsList->ItemAt(index))->IsSelected() )	
				((OutputView*)fOutputViewsList->ItemAt(index))->Clear();
	
	    }
	
		break;
	case MSG_OUTPUT_RESET_ALL:
		m_pAll->Clear();

		for (int32 index = 0 ; index<fTabsList->CountItems() ; index++) {
			if (fTabsList->ItemAt(index) != NULL )	
				((OutputView*)fOutputViewsList->ItemAt(index))->Clear();	
	    }
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
Output::Postf(uint32 index, const char *format, ...)
{
	char string[200];
	va_list arg;
	int done;

	va_start (arg, format);
	done = vsprintf (string, format, arg);
	va_end (arg);

	Post(string, index);

	return done;
}


void
Output::Post(const char* text, uint32 index)
{
	Lock();
	OutputView* view = (OutputView*) fOutputViewsList->ItemAt(index);

	if (view != NULL)
		Add(text, view );
	else
		// Note that the view should be added before this!
		//  Dropping twice to the main
		Add(text, m_pAll );
	
	Unlock();
}


void
Output::Add(const char* text, OutputView* view)
{	
	view->Add(text);
	m_pAll->Add(text);
}
