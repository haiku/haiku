/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 */
#ifndef _GLIFE_GLIFESAVER_H
#define _GLIFE_GLIFESAVER_H


#include <OS.h>
#include <ScreenSaver.h>
#include <View.h>

#include "GLifeGrid.h"
#include "GLifeState.h"
#include "GLifeView.h"


// Constants
const int32 c_iTickSize = 50000;


// GLifeSaver Class Declaration
class GLifeSaver : public BScreenSaver
{
public:
	// Constructor
				GLifeSaver(BMessage*, image_id);

	// State/Preferences Methods			
	status_t	SaveState(BMessage*) const;
	void		RestoreState(BMessage*);
	void		StartConfig(BView*);

	// Start/Stop Methods
	status_t	StartSaver(BView*, bool);
	void		StopSaver(void);

	// Graphics Methods
	void		DirectConnected(direct_buffer_info*);
	void		DirectDraw(int32);

private:
	GLifeState	fGLifeState;
	GLifeView*	fGLifeViewport;
};


#endif /* _GLIFE_GLIFESAVER_H */
