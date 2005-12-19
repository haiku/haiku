// license: public domain
// authors: jonas.sundstrom@kirilla.com

#include "ZipOMaticActivity.h"
#include "ZipOMaticView.h"

ZippoView::ZippoView (BRect a_rect)
:	BBox (a_rect, "zipomatic_view", B_FOLLOW_ALL, 
			B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
			B_PLAIN_BORDER),
	m_stop_button	(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// "Stop" button
	BRect	button_rect (a_rect.right-82, a_rect.bottom-32, a_rect.right-12, a_rect.bottom-12);
	m_stop_button = new BButton(button_rect,"stop_button","Stop", new BMessage(B_QUIT_REQUESTED), B_FOLLOW_RIGHT);
	m_stop_button->SetEnabled(false);
	AddChild(m_stop_button);

	// activity view
	BRect	activity_rect (a_rect.left+14, a_rect.top+15, a_rect.right-15, a_rect.top+31);
	m_activity_view = new Activity (activity_rect, "activity_view", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);
	AddChild(m_activity_view);

	// text views
	BRect	archive_rect (a_rect.left+14, a_rect.top+30, a_rect.right-15, a_rect.top+47);
	m_archive_name_view	= new BStringView (archive_rect, "archive_text", " ", B_FOLLOW_LEFT_RIGHT);
	AddChild (m_archive_name_view);

	BRect	ouput_rect (a_rect.left+14, a_rect.top+47, a_rect.right-15, a_rect.top+66);
	m_zip_output_view	= new BStringView (ouput_rect, "output_text", " ", B_FOLLOW_LEFT_RIGHT);
	AddChild (m_zip_output_view);

}

void ZippoView::Draw (BRect a_update_rect)
{
	BBox::Draw(a_update_rect);
	// fBox->DrawBitmap(fIconBitmap,BPoint(178,26));

	const rgb_color  grey_accent  =  {128,128,128,255};
	const rgb_color  darker_grey  =  {108,108,108,255};
	const rgb_color  plain_white  =  {255,255,255,255};
	const rgb_color  deep_black  =  {255,255,255,255};

	// "preflet" bottom, right grey accent lines

	SetHighColor(grey_accent);

	StrokeLine (BPoint(Bounds().right, Bounds().top), 
				BPoint(Bounds().right, Bounds().bottom));
				
	StrokeLine (BPoint(Bounds().left, Bounds().bottom), 
				BPoint(Bounds().right, Bounds().bottom));				

	// divider 
	SetHighColor(darker_grey);
	StrokeLine (BPoint(Bounds().left+15, Bounds().top+71), 
				BPoint(Bounds().right-16, Bounds().top+71));
	
	SetHighColor(plain_white);
	StrokeLine (BPoint(Bounds().left+15, Bounds().top+72), 
				BPoint(Bounds().right-16, Bounds().top+72));
	
	// text
	SetHighColor(deep_black);
	
	
}

void ZippoView::AllAttached (void)
{
	
}

void ZippoView::FrameMoved (BPoint a_point)
{
	Invalidate();
}

void ZippoView::FrameResized (float a_width, float a_height)
{
	Invalidate();
}


