/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "CamColorSpaceTransform.h"
#include "CamDebug.h"

/* I should replace this by a generic colorspace TranslatorAddOn... */

#undef B_WEBCAM_DECLARE_CSTRANSFORM
#define B_WEBCAM_DECLARE_CSTRANSFORM(trclass,trname) \
extern "C" CamColorSpaceTransform *Instantiate##trclass();
#include "CamInternalColorSpaceTransforms.h"
#undef B_WEBCAM_DECLARE_CSTRANSFORM
typedef CamColorSpaceTransform *(*TransformInstFunc)();
struct { const char *name; TransformInstFunc instfunc; } kTransformTable[] = {
#define B_WEBCAM_DECLARE_CSTRANSFORM(trclass,trname) \
{ #trname, &Instantiate##trclass },
#include "CamInternalColorSpaceTransforms.h"
{ NULL, NULL },
};
#undef B_WEBCAM_DECLARE_CSTRANSFORM


CamColorSpaceTransform::CamColorSpaceTransform()
	: fInitStatus(B_NO_INIT),
	  fVideoFrame()
{

}


CamColorSpaceTransform::~CamColorSpaceTransform()
{

}


status_t
CamColorSpaceTransform::InitCheck()
{
	return fInitStatus;
}


const char *
CamColorSpaceTransform::Name()
{
	return "<unknown>";
}


color_space
CamColorSpaceTransform::OutputSpace()
{
	return B_RGB32;
}


status_t
CamColorSpaceTransform::SetVideoFrame(BRect rect)
{
	return ENOSYS;
}


CamColorSpaceTransform *
CamColorSpaceTransform::Create(const char *name)
{
	for (int i = 0; kTransformTable[i].name; i++) {
		if (!strcmp(kTransformTable[i].name, name))
			return kTransformTable[i].instfunc();
	}
	return NULL;
}
