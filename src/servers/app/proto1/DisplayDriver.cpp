#include "DisplayDriver.h"

DisplayDriver::DisplayDriver(void)
{
	is_initialized=false;
	cursor_visible=false;
}

DisplayDriver::~DisplayDriver(void)
{
}

void DisplayDriver::Initialize(void)
{	// Loading loop to find proper driver

}

bool DisplayDriver::IsInitialized(void)
{	// Let dev know whether things worked out ok
	return is_initialized;
}

void DisplayDriver::SafeMode(void)
{	// Set video mode to 640x480x256 mode
}

void DisplayDriver::Reset(void)
{	// Reloads and restarts driver. Defaults to a safe mode

}

void DisplayDriver::SetScreen(uint32 space)
{
}