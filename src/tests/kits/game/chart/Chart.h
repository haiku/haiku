/*
	
	Chart.h
	
	by Pierre Raynaud-Richard.

*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef CHART_H
#define CHART_H

#include <Application.h>

#include "ChartWindow.h"


/* not too much to be said... */
class ChartApp : public BApplication {
public:
					ChartApp();
private:
	ChartWindow		*fWindow;
};

#endif
