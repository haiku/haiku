//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc. All rights reserved.
//  Distributed under the terms of the MIT license.
//
//	File Name:		ServerScreen.h
//	Author:			Adi Oanca <adioanca@myrealbox.com>
//					Axel Dörfler, axeld@pinc-software.de
//					Stephan Aßmus, <superstippi@gmx.de>
//	Description:	Handles individual screens
//
//------------------------------------------------------------------------------
#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <Accelerant.h>
#include <Point.h>

class DisplayDriver;
class HWInterface;

class Screen {
 public:
								Screen(HWInterface *interface, int32 id);
								Screen();
	virtual						~Screen();

			status_t			Initialize();
			void				Shutdown();

			void				SetID(int32 ID)
									{ fID = ID; }

			status_t			SetMode(display_mode mode);
			status_t			SetMode(uint16 width,
										uint16 height,
										uint32 colorspace,
										float frequency);
			void				GetMode(display_mode* mode) const;
			void				GetMode(uint16 &width,
										uint16 &height,
										uint32 &colorspace,
										float &frequency) const;
			BRect				Frame() const;

	inline	int32				ScreenNumber() const
									{ return fID; }

	inline	DisplayDriver*		GetDisplayDriver() const
									{ return fDriver; }
	inline	HWInterface*		GetHWInterface() const
									{ return fHWInterface; }

 private:
			status_t			_FindMode(uint16 width,
										  uint16 height,
										  uint32 colorspace,
										  float frequency,
										  display_mode* mode) const;

			int32				_FindMode(const display_mode* modeList,
										  uint32 count, uint16 width, uint16 height, uint32 colorspace,
										  float frequency,
										  bool ignoreFrequency = false) const;

			int32				fID;
			DisplayDriver*		fDriver;
			HWInterface*		fHWInterface;
};

#endif	/* _SCREEN_H_ */
