#ifndef APR_WORLD_H
#define APR_WORLD_H

#include <Application.h>
#include "APRWindow.h"

class APRApplication : public BApplication 
{
public:
	APRApplication();
	APRWindow *aprwin;
};

#endif
