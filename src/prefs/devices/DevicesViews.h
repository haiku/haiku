/*

Devices Views Header by Sikosis

(C)2003 OBOS

*/

#ifndef __DEVICESVIEWS_H__
#define __DEVICESVIEWS_H__

#include "Devices.h"
#include "DevicesWindows.h"

class DevicesView : public BView
{
	public:
    	DevicesView(BRect frame);
    	virtual	void	Draw(BRect updateRect);	
};


class ResourceUsageView : public BView
{
	public:
    	ResourceUsageView(BRect frame);
};

#endif
