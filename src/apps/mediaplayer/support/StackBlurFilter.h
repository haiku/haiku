/*
 * Copyright 2008-2010 Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT license.
 */
//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
//
// The Stack Blur Algorithm was invented by Mario Klingemann, 
// mario@quasimondo.com and described here:
// http://incubator.quasimondo.com/processing/fast_blur_deluxe.php
// (search phrase "Stackblur: Fast But Goodlooking"). 
// The major improvement is that there's no more division table
// that was very expensive to create for large blur radii. Insted, 
// for 8-bit per channel and radius not exceeding 254 the division is 
// replaced by multiplication and shift. 
//
//----------------------------------------------------------------------------
#ifndef STACK_BLUR_FILTER
#define STACK_BLUR_FILTER

#include <SupportDefs.h>

class BBitmap;
class RenderBuffer;

class StackBlurFilter {
public:
								StackBlurFilter();
								~StackBlurFilter();


			void				Filter(BBitmap* bitmap, double radius);

private:
			void				_Filter32(uint8* buffer,
										  unsigned width, unsigned height,
										  int32 bpr,
										  unsigned rx, unsigned ry) const;
			void				_Filter8(uint8* buffer,
										 unsigned width, unsigned height,
										 int32 bpr,
										 unsigned rx, unsigned ry) const;
};

#endif // STACK_BLUR_FILTER
