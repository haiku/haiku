//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ServerScreen.cpp
//	Author:			Adi Oanca <adioanca@myrealbox.com>
//	Description:	Handles individual screens
//  
//------------------------------------------------------------------------------
#include <Accelerant.h>
#include <Point.h>
#include <GraphicsDefs.h>
#include <stdio.h>

#include "ServerScreen.h"
#include "DisplayDriver.h"

Screen::Screen(DisplayDriver *dDriver, BPoint res, uint32 colorspace, const int32 &ID)
{
	fID			= ID;
	fDDriver	= dDriver;
	SetResolution(res, colorspace);
}

Screen::~Screen(void)
{
//printf("~Screen( %ld )\n", fID);
}

bool Screen::SupportsResolution(BPoint res, uint32 colorspace)
{
	// TODO: remove/improve
	return true;
	display_mode	*dm = NULL;
	uint32			count;

	status_t err=fDDriver->GetModeList(&dm, &count);
	
	if(err==B_UNSUPPORTED)
	{
		// We've run into quite a problem here! This is a function which is a requirement
		// for a graphics module. The best thing that we can hope for is 640x480x8 without
		// knowing anything else. While even this seems like insanity to assume that we
		// can support this, the only lower mode supported is 640x400, but we shouldn't even
		// bother with such a pathetic possibility.
		if( (uint16)res.x == 640 && (uint16)res.y ==480 && colorspace==B_CMAP8)
			return true;
		else
			return false;
	}

	for (uint32 i=0; i < count; i++)
	{
		if (dm[i].virtual_width == (uint16)res.x &&
			dm[i].virtual_height == (uint16)res.y &&
			dm[i].space == colorspace)
			return true;
	}

	delete [] dm;
	return false;
}

bool Screen::SetResolution(BPoint res, uint32 colorspace)
{
	if (!SupportsResolution(res, colorspace))
		return false;

	display_mode		mode;

	mode.virtual_width	= (uint16)res.x;
	mode.virtual_height	= (uint16)res.y;
	mode.space			= colorspace;

	fDDriver->SetMode(mode);

	return true;
}

BPoint Screen::Resolution() const
{
	display_mode		mode;

	fDDriver->GetMode(&mode);

	return BPoint(mode.virtual_width, mode.virtual_height);
}

int32 Screen::ScreenNumber(void) const
{
	return fID;
}

