/*
	DisplayDriver.cpp
		Modular class to allow the server to not care what the ultimate output is
		for graphics calls.
*/
#include "DisplayDriver.h"
#include "LayerData.h"

DisplayDriver::DisplayDriver(void)
{
	lock_sem=create_sem(1,"displaydriver_lock");
	buffer_depth=0;
	buffer_width=0;
	buffer_height=0;
	buffer_mode=-1;
}

DisplayDriver::~DisplayDriver(void)
{
	delete_sem(lock_sem);
}

bool DisplayDriver::Initialize(void)
{
	return false;
}

uint8 DisplayDriver::GetDepth(void)
{
	return buffer_depth;
}

uint16 DisplayDriver::GetHeight(void)
{
	return buffer_height;
}

uint16 DisplayDriver::GetWidth(void)
{
	return buffer_width;
}

int32 DisplayDriver::GetMode(void)
{
	return buffer_mode;
}

bool DisplayDriver::DumpToFile(const char *path)
{
	return false;
}

//---------------------------------------------------------
//					Private Methods
//---------------------------------------------------------

void DisplayDriver::Lock(void)
{
	acquire_sem(lock_sem);
}

void DisplayDriver::Unlock(void)
{
	release_sem(lock_sem);
}

void DisplayDriver::SetDepthInternal(uint8 d)
{
	buffer_depth=d;
}

void DisplayDriver::SetHeightInternal(uint16 h)
{
	buffer_height=h;
}

void DisplayDriver::SetWidthInternal(uint16 w)
{
	buffer_width=w;
}

void DisplayDriver::SetModeInternal(int32 m)
{
	buffer_mode=m;
}
