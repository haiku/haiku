#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <Point.h>
//#include "DisplayDriver.h"
class DisplayDriver;

class Screen{
public:
								Screen(DisplayDriver *dDriver, BPoint res,
									uint32 colorspace, const int32 &ID);
								Screen(){ ; }
								~Screen(void);

			void				SetColorSpace(const uint32 &colorspace);
			uint32				ColorSpace(void) const;

			void				SetID(int32 ID){ fID = ID; }

	// TODO: get/set prototype methods for graphic card features
			bool				SupportsResolution(BPoint res, uint32 colorspace);
			bool				SetResolution(BPoint res, uint32 colorspace);
			BPoint				Resolution() const;

			int32				ScreenNumber(void) const;
			DisplayDriver*		DDriver() const { return fDDriver; }

private:

	// TODO: members in witch we should store data.

			int32				fID;
			DisplayDriver		*fDDriver;
};

#endif
