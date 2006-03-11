/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon driver
		
	Multi-Monitor Settings interface
*/
#ifndef _MULTIMON_H
#define _MULTIMON_H


#include <SupportDefs.h>


class BScreen;

status_t GetSwapDisplays( BScreen *screen, bool *swap );
status_t SetSwapDisplays( BScreen *screen, bool swap );

status_t GetUseLaptopPanel( BScreen *screen, bool *use );
status_t SetUseLaptopPanel( BScreen *screen, bool use );

status_t GetNthSupportedTVStandard( BScreen *screen, int idx, uint32 *standard );
status_t GetTVStandard( BScreen *screen, uint32 *standard );
status_t SetTVStandard( BScreen *screen, uint32 standard );

status_t TestMultiMonSupport( BScreen *screen );

#endif	// _MULTIMON_H
