#ifndef _DHAPP_H_
#define _DHAPP_H_

#include <Application.h>

class DisplayDriver;

class DHApp : public BApplication
{
public:
	DHApp(void);
	~DHApp(void);
	DisplayDriver *driver;
};

#endif