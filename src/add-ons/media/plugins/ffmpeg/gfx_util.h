/******************************************************************************
/
/	File:			gfx_util.h
/
/	Description:	utility functions for ffmpeg codec wrappers for BeOs R5.
/
/	Disclaimer:		This decoder is based on undocumented APIs.
/					Therefore it is likely, if not certain, to break in future
/					versions of BeOS. This piece of software is in no way
/					connected to or even supported by Be, Inc.
/
/	Copyright (C) 2001 François Revol. All Rights Reserved.
/
******************************************************************************/

#ifndef __FFUTILS_H__
#define __FFUTILS_H__

// BeOS and libavcodec bitmap formats
#include <GraphicsDefs.h>
extern "C" {
	#include "libavcodec/avcodec.h"
}

// this function will be used by the wrapper to write into
// the Media Kit provided buffer from the self-allocated ffmpeg codec buffer
// it also will do some colorspace and planar/chunky conversions.
// these functions should be optimized with overlay in mind.
//typedef void (*gfx_convert_func) (AVPicture *in, AVPicture *out, long line_count, long size);
// will become:
typedef void (*gfx_convert_func) (AVFrame *in, AVFrame *out, int width, int height);

// this function will try to find the best colorspaces for both the ff-codec and
// the Media Kit sides.
gfx_convert_func resolve_colorspace(color_space cs, PixelFormat pixelFormat, int width, int height);

const char *pixfmt_to_string(int format);

color_space pixfmt_to_colorspace(int format);
PixelFormat colorspace_to_pixfmt(color_space format);

void dump_ffframe(AVFrame *frame, const char *name);

#endif
