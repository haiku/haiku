#include "Decorator.h"

Decorator::Decorator(Layer *lay, uint32 windowflags, window_look wlook) 
{
	layer=lay;
	flags=windowflags;
	minsize.Set(0,0); 
	bsize.Set(1,1,1,1);
	look=wlook;
	focused=true;
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
	return BRect(0,0,0,0);
}

BPoint Decorator::GetMinimumSize(void)
{
	return BPoint(0,0);
}

void Decorator::SetFlags(uint32 flags)
{
}

uint32 Decorator::Flags(void)
{
	return flags;
}

void Decorator::SetLook(window_look wlook)
{
}

uint32 Decorator::Look(void)
{
	return look;
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
}

void Decorator::SetZoomButton(bool down)
{
}

void Decorator::SetMinimizeButton(bool down)
{
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
