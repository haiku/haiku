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
    	virtual	void Draw(BRect updateRect);	
};


class ResourceUsageView : public BView
{
	public:
    	ResourceUsageView(BRect frame);
};


class ModemView : public BView
{
	public:
		ModemView(BRect frame);
};	


class IRQView : public BView
{
	public:
		IRQView(BRect frame);
};		


class DMAView : public BView
{
	public:
		DMAView(BRect frame);
};		
		
		
class IORangeView : public BView
{
	public:
		IORangeView(BRect frame);
};		


class MemoryRangeView : public BView
{
	public:
		MemoryRangeView(BRect frame);
};				



#endif

