/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 */
#ifndef _GLIFE_GLIFECONFIG_H
#define _GLIFE_GLIFECONFIG_H


#include <Slider.h>
#include <View.h>

#include "GLifeState.h"


// Message IDs
enum {
	e_midGridWidth		= 'grdw',
	e_midGridHeight		= 'grdh',
	e_midBorder			= 'bord'
};


// GLifeConfig Class Declaration
class GLifeConfig : public BView
{
private:
	GLifeState*	m_pglsState;
	
	uint32		m_uiWindowFlags;
	
	BSlider*	fGridWidth;
	BSlider*	fGridHeight;
	BSlider*	fBorder;
	
public:
				GLifeConfig(BRect, GLifeState*);
	
	void		AttachedToWindow(void);
	void		MessageReceived(BMessage*);
};


#endif /* _GLIFE_GLIFECONFIG_H */
