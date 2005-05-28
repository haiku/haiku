#include <Window.h>
#include "MyView.h"

#include "WinBorder.h"

extern BWindow* wind;

WinBorder::WinBorder(BRect frame, const char* name,
				uint32 rm, uint32 flags, rgb_color c)
	: Layer(frame, name, rm, flags, c)
{
	fColor.red = 255;
	fColor.green = 203;
	fColor.blue = 0;
}

WinBorder::~WinBorder()
{
}

void WinBorder::set_decorator_region(BRect bounds)
{
	// NOTE: frame is in screen coords
	fDecRegion.Include(BRect(bounds.left-3, bounds.top-3, bounds.right+3, bounds.top));
	fDecRegion.Include(BRect(bounds.left-3, bounds.bottom, bounds.right+3, bounds.bottom+3));
	fDecRegion.Include(BRect(bounds.left-3, bounds.top, bounds.left, bounds.bottom));
	fDecRegion.Include(BRect(bounds.right, bounds.top, bounds.right+3, bounds.bottom));

	fDecRegion.Include(BRect(bounds.left-3, bounds.top-3-10, bounds.left+bounds.Width()/2, bounds.top-3));
	// fDecRegion
}

void WinBorder::operate_on_visible(BRegion &region)
{
	
}

void WinBorder::set_user_regions(BRegion &reg)
{
	set_decorator_region(Bounds());
	ConvertToScreen2(&fDecRegion);

	BRect			screenFrame(Bounds());
	ConvertToScreen2(&screenFrame);
	reg.Set(screenFrame);

wind->Lock();
	BRegion			screenReg(GetRootLayer()->Bounds());
wind->Unlock();
	reg.IntersectWith(&screenReg);

	reg.Include(&fDecRegion);

//reg.PrintToStream();
}
