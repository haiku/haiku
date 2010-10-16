/*
	JSDSlider.cpp
	Dr.H.Reh
	27.11.2004
	
	Based on source code from Be Inc. RIP
	Copyright 1995 Be Incorporated, All Rights Reserved.
*/

#include "JSDSlider.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


JSDSlider::JSDSlider(const char* name, const char* label,
	BMessage* msg, int32 min, int32 max)
	: BSlider(name, label, msg, min, max, B_HORIZONTAL)
{
}


JSDSlider::~JSDSlider()
{ 
} 


const char*
JSDSlider::UpdateText() const
{
	// When the slider's Draw method is called, this method will also be called.
	// If its return value is non-NULL, then it will be drawn with the rest of
	// the slider
	static char string[64];
	string[0] = 0;

	if (!strcmp("gamma", Name())) {
		float gamma;
		gamma = exp((Value() * log(2.0) * 0.01) );
		sprintf(string, " %.2f", gamma);
	} else if (!strcmp("inkDensity", Name()) ) {
		float density = Value();
		density = (density / 127.0) * 100.0;
		sprintf(string," %.0f%%", density);
	}

	fResult.SetTo(string);
	return fResult.String();
}
