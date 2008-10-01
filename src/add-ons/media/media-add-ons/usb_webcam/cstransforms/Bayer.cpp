/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

/*
 * Bayer to RGB32 colorspace transformation
 */

#include "CamColorSpaceTransform.h"

class BayerTransform : public CamColorSpaceTransform
{
	public: 
						BayerTransform();
	virtual				~BayerTransform();
	
	virtual const char*	Name();
	virtual color_space	OutputSpace();
	
//	virtual status_t	SetVideoFrame(BRect rect);
//	virtual BRect		VideoFrame() const { return fVideoFrame; };
	
	private:
};

// -----------------------------------------------------------------------------
BayerTransform::BayerTransform()
{
}

// -----------------------------------------------------------------------------
BayerTransform::~BayerTransform()
{
}

// -----------------------------------------------------------------------------
const char *
BayerTransform::Name()
{
	return "bayer";
}

// -----------------------------------------------------------------------------
color_space
BayerTransform::OutputSpace()
{
	return B_RGB32;
}

// -----------------------------------------------------------------------------



B_WEBCAM_DECLARE_CSTRANSFORM(BayerTransform, bayer)

