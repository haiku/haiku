
#include <Accelerant.h>
#include <Point.h>
#include <GraphicsDefs.h>
#include <stdio.h>

#include "ServerScreen.h"
#include "DisplayDriver.h"

Screen::Screen(DisplayDriver *dDriver, BPoint res, uint32 colorspace,
				const int32 &ID)
{
	fID			= ID;
	fDriver		= dDriver;
	SetResolution(res, colorspace);
}

Screen::Screen(void)
{
}

Screen::~Screen(void){
//printf("~Screen( %ld )\n", fID);
}

bool Screen::SupportsResolution(const BPoint &res, const uint32 &colorspace)
{
	display_mode	*dm=NULL;
	uint32			count;

	fDriver->GetModeList(&dm, &count);

	for (uint32 i=0; i < count; i++)
	{
		if (dm[i].virtual_width == (uint16)res.x &&
			dm[i].virtual_height == (uint16)res.y &&
			dm[i].space == colorspace)
			return true;
	}

	delete dm;
	return false;
}

bool Screen::SetResolution(const BPoint &res, const uint32 &colorspace)
{
	if (!SupportsResolution(res, colorspace))
		return false;

	display_mode		mode;

	mode.virtual_width	= (uint16)res.x;
	mode.virtual_height	= (uint16)res.y;
	mode.space			= colorspace;

	fDriver->SetMode(mode);

	return true;
}

BPoint Screen::Resolution() const{
	display_mode		mode;

	fDriver->GetMode(&mode);

	return BPoint(mode.virtual_width, mode.virtual_height);
}

int32 Screen::ScreenNumber(void) const{
	return fID;
}

