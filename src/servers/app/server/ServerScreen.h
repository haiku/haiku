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
//	File Name:		ServerScreen.h
//	Author:			Adi Oanca <adioanca@myrealbox.com>
//	Description:	Handles individual screens
//  
//------------------------------------------------------------------------------
#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <Point.h>
class DisplayDriver;

class Screen
{
public:
	Screen(DisplayDriver *dDriver, BPoint res, uint32 colorspace, const int32 &ID);
	Screen(){ ; }
	~Screen(void);
	
	void				SetID(int32 ID){ fID = ID; }

	// TODO: get/set prototype methods for graphic card features
	bool				SupportsResolution(BPoint res, uint32 colorspace);
	bool				SetResolution(BPoint res, uint32 colorspace);
	BPoint				Resolution() const;
	
	int32				ScreenNumber(void) const;
	DisplayDriver*		DDriver() const { return fDDriver; }

private:
	// TODO: members in which we should store data.
	int32				fID;
	DisplayDriver		*fDDriver;
};

#endif
