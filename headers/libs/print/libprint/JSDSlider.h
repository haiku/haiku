/*
	JSDSlider.h
	Dr.H.Reh
	27.11.2004
	
	Based on source code from Be Inc. RIP
	Copyright 1995 Be Incorporated, All Rights Reserved.
*/

#ifndef __JSD_SLIDER_H
#define __JSD_SLIDER_H

#include <Slider.h>
#include <String.h>


class JSDSlider : public BSlider 
{
public: 
						JSDSlider(const char* name, const char* label,
							BMessage* msg, int32 min, int32 max);

	virtual				~JSDSlider();
	virtual const char* UpdateText() const;

private:
	mutable	BString		fResult;
};

#endif 
