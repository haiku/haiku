#include <Window.h>
#include "MyView.h"

#include "WinBorder.h"

#include <stdio.h>

extern BWindow* wind;

WinBorder::WinBorder(BRect frame, const char* name,
				uint32 rm, uint32 flags, rgb_color c)
	: Layer(frame, name, rm, flags, c)
{
	fColor.red = 255;
	fColor.green = 203;
	fColor.blue = 0;

	fRebuildDecRegion = true;
}

WinBorder::~WinBorder()
{
}

void WinBorder::MovedByHook(float dx, float dy)
{
	fDecRegion.OffsetBy(dx, dy);
}

void WinBorder::ResizedByHook(float dx, float dy, bool automatic)
{
	fRebuildDecRegion = true;
}

void WinBorder::set_decorator_region(BRect bounds)
{
	fRebuildDecRegion = false;

	fDecRegion.MakeEmpty();
	// NOTE: frame is in screen coords
	fDecRegion.Include(BRect(bounds.left-4, bounds.top-4, bounds.right+4, bounds.top-1));
	fDecRegion.Include(BRect(bounds.left-4, bounds.bottom+1, bounds.right+4, bounds.bottom+4));
	fDecRegion.Include(BRect(bounds.left-4, bounds.top, bounds.left-1, bounds.bottom));
	fDecRegion.Include(BRect(bounds.right+1, bounds.top, bounds.right+4, bounds.bottom));

	// tab
	fDecRegion.Include(BRect(bounds.left-4, bounds.top-4-10, bounds.left+bounds.Width()/2, bounds.top-4));

	// resize rect
	fDecRegion.Include(BRect(bounds.right-10, bounds.bottom-10, bounds.right, bounds.bottom));
}

bool WinBorder::alter_visible_for_children(BRegion &region)
{
	region.Exclude(&fDecRegion);
	return true;
}

void WinBorder::get_user_regions(BRegion &reg)
{
	if (fRebuildDecRegion)
	{
		set_decorator_region(Bounds());
		ConvertToScreen2(&fDecRegion);
	}

	BRect			screenFrame(Bounds());
	ConvertToScreen2(&screenFrame);
	reg.Set(screenFrame);

wind->Lock();
	BRegion			screenReg(GetRootLayer()->Bounds());
wind->Unlock();
	reg.IntersectWith(&screenReg);

	reg.Include(&fDecRegion);
}
