/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon driver
		
	Multi-Monitor Settings interface
*/

#ifndef _MULTIMON_H
#define _MULTIMON_H

class BScreen;

status_t GetSwapDisplays( BScreen *screen, bool *swap );
status_t SetSwapDisplays( BScreen *screen, bool swap );
status_t TestMultiMonSupport( BScreen *screen );

#endif
