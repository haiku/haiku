/*
 * HalftoneView.h
 * Copyright 2004 Michael Pfeiffer. All Rights Reserved.
 */

#ifndef _HALFTONE_VIEW_H
#define _HALFTONE_VIEW_H

#include <View.h>
#include "Halftone.h"

class HalftonePreviewView : public BView
{
public:
			HalftonePreviewView(BRect frame, const char* name,
				uint32 resizeMask, uint32 flags);

	void	Preview(float gamma, float min, Halftone::DitherType ditherType,
				bool color);
};

class HalftoneView : public BView
{
public:
			HalftoneView(BRect frame, const char* name, uint32 resizeMask,
				uint32 flags);

	void	Preview(float gamma, float min, Halftone::DitherType ditherType,
				bool color);

private:
	HalftonePreviewView* fPreview;
};

#endif

