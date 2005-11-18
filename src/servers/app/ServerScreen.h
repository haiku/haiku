/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Adi Oanca <adioanca@myrealbox.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus, <superstippi@gmx.de>
 */
#ifndef _SCREEN_H_
#define _SCREEN_H_


#include <Accelerant.h>
#include <Point.h>


class DrawingEngine;
class HWInterface;

class Screen {
 public:
								Screen(::HWInterface *interface, int32 id);
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

	inline	DrawingEngine*		GetDrawingEngine() const
									{ return fDriver; }
	inline	::HWInterface*		HWInterface() const
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
			DrawingEngine*		fDriver;
			::HWInterface*		fHWInterface;
};

#endif	/* _SCREEN_H_ */
