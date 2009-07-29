// license: public domain
// authors: Jonas Sundström, jonas@kirilla.com


#include "ZipOMaticView.h"


ZippoView::ZippoView(BRect rect)
	:
	BBox(rect, "zipomatic_view", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER),
	fStopButton(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fStopButton = new BButton(
		BRect(rect.right-82, rect.bottom-32, rect.right-12, rect.bottom-12),
		"stop_button", "Stop", new BMessage(B_QUIT_REQUESTED), B_FOLLOW_RIGHT);
	fStopButton->SetEnabled(false);
	AddChild(fStopButton);

	BRect activity_rect(rect.left+14, rect.top+15, rect.right-15, rect.top+31);
	fActivityView = new Activity(activity_rect, "activity_view",
		B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS);
	AddChild(fActivityView);

	BRect archive_rect(rect.left+14, rect.top+30, rect.right-15, rect.top+47);
	fArchiveNameView = new BStringView(archive_rect, "archive_text", " ",
		B_FOLLOW_LEFT_RIGHT);
	AddChild(fArchiveNameView);

	BRect ouput_rect(rect.left+14, rect.top+47, rect.right-15, rect.top+66);
	fZipOutputView = new BStringView(ouput_rect, "output_text",
		"Drop files to zip.", B_FOLLOW_LEFT_RIGHT);
	AddChild(fZipOutputView);
}


void ZippoView::Draw(BRect rect)
{
	BBox::Draw(rect);

	const rgb_color grey_accent = {128,128,128,255};
	const rgb_color darker_grey = {108,108,108,255};
	const rgb_color plain_white = {255,255,255,255};
	const rgb_color black = {0, 0, 0, 255};

	// "preflet" bottom, right grey accent lines
	SetHighColor(grey_accent);

	StrokeLine(BPoint(Bounds().right, Bounds().top), 
				BPoint(Bounds().right, Bounds().bottom));
				
	StrokeLine(BPoint(Bounds().left, Bounds().bottom), 
				BPoint(Bounds().right, Bounds().bottom));				

	// divider 
	SetHighColor(darker_grey);
	StrokeLine(BPoint(Bounds().left+15, Bounds().top+71), 
				BPoint(Bounds().right-16, Bounds().top+71));
	
	SetHighColor(plain_white);
	StrokeLine(BPoint(Bounds().left+15, Bounds().top+72), 
				BPoint(Bounds().right-16, Bounds().top+72));
	
	SetHighColor(black);
}


void
ZippoView::FrameMoved(BPoint point)
{
	Invalidate();
}


void
ZippoView::FrameResized(float width, float height)
{
	Invalidate();
}

