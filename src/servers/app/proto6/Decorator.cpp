#include "Decorator.h"

Decorator::Decorator(Layer *lay, uint32 windowflags, uint32 wlook) 
{
	layer=lay;
	dflags=windowflags;
	minsize.Set(0,0); 
	bsize.Set(1,1,1,1);
	dlook=wlook;
	focused=false;
	driver=get_gfxdriver();
}

Decorator::~Decorator(void)
{
}

Layer* Decorator::GetLayer(void)
{
	return layer;
}

click_type Decorator::Clicked(BPoint pt, uint32 buttons)
{
	return CLICK_NONE;
}

void Decorator::Resize(BRect rect)
{
}

void Decorator::MoveBy(BPoint pt)
{
}

BRect Decorator::GetBorderSize(void)
{
	return bsize;
}

BPoint Decorator::GetMinimumSize(void)
{
	return BPoint(0,0);
}

BRegion * Decorator::GetFootprint(void)
{
	return NULL;
}

void Decorator::SetFlags(uint32 flags)
{
	dflags=flags;
}

uint32 Decorator::Flags(void)
{
	return dflags;
}

void Decorator::SetLook(uint32 wlook)
{
	dlook=(uint32)wlook;
}

uint32 Decorator::Look(void)
{
	return dlook;
}

void Decorator::SetFeel(uint32 wfeel)
{
	dfeel=(uint32)wfeel;
}

uint32 Decorator::Feel(void)
{
	return dfeel;
}

void Decorator::UpdateTitle(const char *string)
{
}

void Decorator::UpdateFont(void)
{
}

void Decorator::SetFocus(bool focused)
{
}

void Decorator::SetCloseButton(bool down)
{
	closestate=down;
}

void Decorator::SetZoomButton(bool down)
{
	zoomstate=down;
}

void Decorator::SetMinimizeButton(bool down)
{
	minstate=down;
}

bool Decorator::GetCloseButton(void) const
{
	return closestate;
}

bool Decorator::GetZoomButton(void) const
{
	return zoomstate;
}

bool Decorator::GetMinimizeButton(void) const
{
	return minstate;
}

void Decorator::Draw(BRect update)
{
}

void Decorator::Draw(void)
{
}

void Decorator::DrawTab(void)
{
}

void Decorator::DrawFrame(void)
{
}

void Decorator::DrawZoom(BRect r)
{
}

void Decorator::DrawClose(BRect r)
{
}

void Decorator::DrawMinimize(BRect r)
{
}

void Decorator::CalculateBorders(void)
{
}
