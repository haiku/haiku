#ifndef _SERVERSCREEN_H_
#define _SERVERSCREEN_H_

#include <Point.h>
#include "DisplayDriver.h"

class Screen{
public:
	Screen(DisplayDriver *dDriver, BPoint res, uint32 colorspace, const int32 &ID);
	Screen(void);
	~Screen(void);

	void SetID(int32 ID){ fID = ID; }

	// TODO: get/set prototype methods for graphic card features
	bool				SupportsResolution(const BPoint &res, const uint32 &colorspace);
	bool				SetResolution(const BPoint &res, const uint32 &colorspace);
	BPoint				Resolution() const;

	int32				ScreenNumber(void) const;
	DisplayDriver*		GetDriver(void) const { return fDriver; }

private:

	// TODO: add members in which we should store data.
	int32 fID;
	DisplayDriver *fDriver;
};

#endif
