#include <Handler.h>
#include <Messenger.h>
#include <Rect.h>

#include "TerminalTextView.h"
#include "Constants.h"

TerminalTextView::TerminalTextView(BRect viewFrame, BRect textBounds, BHandler *handler)
	: BTextView(viewFrame, "textview", textBounds, 
			    B_FOLLOW_ALL, B_FRAME_EVENTS|B_WILL_DRAW)
{ 
	fHandler = handler;
	fMessenger = new BMessenger(handler);
	SetWordWrap(false);
}

TerminalTextView::~TerminalTextView(){
}
	
void
TerminalTextView::Select(int32 start, int32 finish)
{
	if (start == finish) {
		fMessenger->SendMessage(DISABLE_ITEMS);
	} else {
		fMessenger->SendMessage(ENABLE_ITEMS);
	}
	BTextView::Select(start, finish);
}

void
TerminalTextView::FrameResized(float width, float height)
{
	BRect oldText = TextRect();
	
	BTextView::FrameResized(width, height);
	
	BRect textRect;
	textRect = Bounds();
	textRect.InsetBy(TEXT_INSET,TEXT_INSET);
	SetTextRect(textRect);
}
