#ifndef __DEVICESINFO_H__
#define __DEVICESINFO_H__

#include <drivers/config_manager.h>
#include <ListItem.h>

class DevicesInfo
{
	public:
    	DevicesInfo(struct device_info *info, 
    		struct device_configuration *current, 
    		struct possible_device_configurations *possible);
	    ~DevicesInfo();
		struct device_info * GetInfo() { return fInfo;}
		char * GetName() const { return fName; }
		struct device_configuration * GetCurrent() { return fCurrent;}
	private:
		struct device_info *fInfo;
		struct device_configuration *fCurrent; 
		struct possible_device_configurations *fPossible;
		char* fName;
};

class DeviceItem : public BListItem
{
	public:
		DeviceItem(DevicesInfo *info, const char* name);
		~DeviceItem();
		DevicesInfo *GetInfo() { return fInfo; }
		virtual void DrawItem(BView *, BRect, bool = false);
	private:
		DevicesInfo *fInfo;
		char* fName;
};

#endif
