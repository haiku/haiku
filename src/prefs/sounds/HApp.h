#ifndef __HAPP_H__
#define __HAPP_H__

#include <Application.h>

class HApp :public BApplication{
public:
					HApp();
	virtual			~HApp();
protected:
	virtual void	AboutRequested();
};
#endif