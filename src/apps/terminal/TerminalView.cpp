#include <Message.h>
#include <Messenger.h>
#include <Rect.h>
#include <Region.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <Node.h>

#include "TerminalView.h"
#include "Constants.h"

TerminalView::TerminalView(BRect viewFrame, BRect textBounds, BHandler *handler)
	: BTextView(viewFrame, "textview", textBounds, 
			    B_FOLLOW_ALL, B_FRAME_EVENTS|B_WILL_DRAW)
{ 
	fHandler= handler;
	fMessenger= new BMessenger(handler);
}

TerminalView::~TerminalView(){
}
	
void
TerminalView::Select(int32 start, int32 finish)
{
	if(start==finish)
		fChangeMessage= new BMessage(DISABLE_ITEMS);
	else
		fChangeMessage= new BMessage(ENABLE_ITEMS);
	
	fMessenger->SendMessage(fChangeMessage);
	
	BTextView::Select(start, finish);
}

