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
	SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );
}

ShowImageView::~ShowImageView()
{
	delete m_pBitmap;
	m_pBitmap = NULL;
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

