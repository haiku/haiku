/*
	Copyright 2007, Francois Revol.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <OS.h>
#include <Debug.h>
#include <View.h>

#if DEBUG
//#include <File.h>
#include <Alert.h>
#include <Button.h>
#include <TextView.h>
#include <StringIO.h>
#include "DumpMessage.h"
#endif

#include "PenInputInkWindow.h"
#include "PenInputLooper.h"
#include "PenInputStrings.h"

const rgb_color kInkColor = { 100, 100, 255, 0 };

class PenInputInkView : public BView {
public:
	PenInputInkView(BRect frame, PenInputInkWindow *window);
	virtual ~PenInputInkView();
	virtual void Draw(BRect udpateRect);
private:
	PenInputInkWindow *fWindow;
};

PenInputInkView::PenInputInkView(BRect frame, PenInputInkWindow *window)
	: BView(frame, "PenInkView", B_FOLLOW_ALL, B_WILL_DRAW),
	  fWindow(window)
{
	
}

PenInputInkView::~PenInputInkView()
{
}
void PenInputInkView::Draw(BRect udpateRect)
{
	/* ugh ? */
	if (!fWindow)
		return;
	
	StrokeShape(&fWindow->fStrokes);
	//DEBUG:StrokeRect(fWindow->fStrokes.Bounds());
}



PenInputInkWindow::PenInputInkWindow(BRect frame)
	: BWindow(frame, "PenInputInkWindow",
			  B_NO_BORDER_WINDOW_LOOK,
			  B_FLOATING_ALL_WINDOW_FEEL,
			  B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | 
			  B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_AVOID_FOCUS,
			  B_ALL_WORKSPACES),
	  fStrokeCount(0)
{
	PRINT(("%s\n", __FUNCTION__));
	fInkView = new PenInputInkView(Bounds(), this);
	fInkView->SetViewColor(B_TRANSPARENT_32_BIT);
	fInkView->SetLowColor(B_TRANSPARENT_32_BIT);
	//fInkView->SetHighColor(0,255,0);
	fInkView->SetHighColor(kInkColor);
	fInkView->SetPenSize(2.0);
	AddChild(fInkView);
}

PenInputInkWindow::~PenInputInkWindow()
{
	PRINT(("%s\n", __FUNCTION__));
}

void PenInputInkWindow::MessageReceived(BMessage *message)
{
	BPoint where;
	switch (message->what) {
	case MSG_BEGIN_INK:
		fStrokeCount = 0;
		fStrokes.Clear();
		// fall through
	case MSG_SHOW_WIN:
		if (IsHidden())
			Show();
		break;
	case MSG_END_INK:
	case MSG_HIDE_WIN:
		if (!IsHidden())
			Hide();
		break;
	case B_MOUSE_MOVED:
		if (message->FindPoint("where", &where) == B_OK) {
			fStrokes.LineTo(where);
			fStrokeCount++;
			//fInkView->Invalidate(fStrokes.Bounds()); // XXX
			fInkView->Invalidate(Bounds()); // XXX
		}
		break;
	case B_MOUSE_UP:
		if (message->FindPoint("where", &where) == B_OK) {
			fStrokes.LineTo(where);
			fStrokeCount++;
			//fInkView->Invalidate(fStrokes.Bounds()); // XXX
			fInkView->Invalidate(Bounds()); // XXX
		}
		break;
	case B_MOUSE_DOWN:
		if (message->FindPoint("where", &where) == B_OK) {
			fStrokes.MoveTo(where);
			//fStrokeCount++;
			//fInkView->Invalidate(fStrokes.Bounds()); // XXX
			fInkView->Invalidate(Bounds()); // XXX
		}
		break;
	default:
		BWindow::MessageReceived(message);
		return;
	}
}

