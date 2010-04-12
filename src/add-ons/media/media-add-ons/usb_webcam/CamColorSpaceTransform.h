/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CAM_COLOR_SPACE_TRANSFORM_H
#define _CAM_COLOR_SPACE_TRANSFORM_H

#include "CamDevice.h"
#include <Rect.h>

// This class represents the camera's (cmos or whatever) sensor chip
class CamColorSpaceTransform {
	public:
						CamColorSpaceTransform();
	virtual				~CamColorSpaceTransform();

	virtual status_t	InitCheck();

	virtual const char*	Name();
	virtual color_space	OutputSpace();

	virtual status_t	SetVideoFrame(BRect rect);
	virtual BRect		VideoFrame() const { return fVideoFrame; };

	static CamColorSpaceTransform *Create(const char *name);

	protected:
		status_t		fInitStatus;
		BRect			fVideoFrame;
	private:
};

// internal modules
#define B_WEBCAM_DECLARE_CSTRANSFORM(trclass,trname) \
extern "C" CamColorSpaceTransform *Instantiate##trclass(); \
CamColorSpaceTransform *Instantiate##trclass() \
{ return new trclass(); };


#endif /* _CAM_COLOR_SPACE_TRANSFORM_H */
