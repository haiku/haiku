/*
    OBOS ShowImage 0.1 - 17/02/2002 - 22:22 - Fernando Francisco de Oliveira
*/

#include <stdio.h>
#include <Bitmap.h>
#include <Message.h>
#include <ScrollBar.h>
#include <StopWatch.h>
#include <Alert.h>
#include <MenuBar.h>
#include <MenuItem.h>

#include "ShowImageConstants.h"
#include "ShowImageView.h"

ShowImageView::ShowImageView(BRect r, const char* name, uint32 resizingMode, uint32 flags)
	: BView(r, name, resizingMode, flags), m_pBitmap(NULL)
{
	Selecting 	= false;
	Selected 	= false;
	PointOn		= false;
	
	SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );
}

ShowImageView::~ShowImageView()
{
	delete m_pBitmap;
}

void ShowImageView::SetBitmap(BBitmap* pBitmap)
{
	m_pBitmap = pBitmap;
}

BBitmap *
ShowImageView::GetBitmap()
{
	return m_pBitmap;
}

void ShowImageView::AttachedToWindow()
{
	FixupScrollBars();
}

void ShowImageView::Draw(BRect updateRect)
{
	if ( m_pBitmap ) 
	{
		DrawBitmap( m_pBitmap, updateRect, updateRect );
		
		if ( Selected && PointOn ) {
			SetDrawingMode( B_OP_INVERT /*B_OP_ALPHA */); 
            
            StrokeRect( BRect( IniPoint, EndPoint ), B_MIXED_COLORS );
            
            SetDrawingMode(B_OP_COPY); 
        }
	}
}

void ShowImageView::FrameResized(float /* width */, float /* height */)
{
	FixupScrollBars();
}

void ShowImageView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	default:
		BView::MessageReceived(msg);
		break;
	}
}

void ShowImageView::FixupScrollBars()
{
	if (! m_pBitmap)
		return;

	BRect vBds = Bounds(), bBds = m_pBitmap->Bounds();
	float prop;
	float range;
	
	BScrollBar* sb = ScrollBar(B_HORIZONTAL);
	if (sb) {
		range = bBds.Width() - vBds.Width();
		if (range < 0) range = 0;
		prop = vBds.Width() / bBds.Width();
		if (prop > 1.0f) prop = 1.0f;
		sb->SetRange(0, range);
		sb->SetProportion(prop);
		sb->SetSteps(10, 100);
	}
	sb = ScrollBar(B_VERTICAL);
	if (sb) {
		range = bBds.Height() - vBds.Height();
		if (range < 0) range = 0;
		prop = vBds.Height() / bBds.Height();
		if (prop > 1.0f) prop = 1.0f;
		sb->SetRange(0, range);
		sb->SetProportion(prop);
		sb->SetSteps(10, 100);
	}
}

void ShowImageView::MouseDown( BPoint point )
{
	if ( Selected && SelectedRect.Contains( point ) ) {
		return;
	}
	
    uint32 buttons; 
   
    GetMouse(&point, &buttons); 
       
    if ( buttons == B_PRIMARY_MOUSE_BUTTON ) {
    
       if ( Selected ) {
          Selected = false;
          Invalidate( BRect( IniPoint, EndPoint ) );
       }

       BRect rect(point, point); 
       BeginRectTracking(rect, B_TRACK_RECT_CORNER); 

       IniPoint = point;
       do { 
           snooze(30 * 1000); 
           GetMouse(&point, &buttons); 
       } while ( buttons ); 
       
       EndRectTracking(); 
       
       Selected = true;
   
       rect.SetRightBottom(point); 
       
       EndPoint = point;
       
       SelectedRect = BRect( IniPoint, EndPoint );
       
       Invalidate( SelectedRect );
       
       PointOn = true;
       
       BMenuItem   * pMenuCopy;
       pMenuCopy 	= pBar->FindItem( B_COPY );
       pMenuCopy->SetEnabled( true );
    }
}

void ShowImageView::MouseUp( BPoint point )
{
}

void ShowImageView::MouseMoved( BPoint point, uint32 transit, const BMessage *message )
{	
}

void ShowImageView::Pulse(void)
{
		if ( Selected ) {
			PushState();
			
			PointOn = ! PointOn;
			
			SetDrawingMode( B_OP_INVERT ); 
            
            StrokeRect( SelectedRect, B_MIXED_COLORS );
            
            //SetDrawingMode(B_OP_COPY); 
            PopState();
        }
}
