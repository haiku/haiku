//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.2
// Copyright (C) 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
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
// Conversion from one colorspace/pixel format to another
//
//----------------------------------------------------------------------------

#ifndef AGG_COLOR_CONV_INCLUDED
#define AGG_COLOR_CONV_INCLUDED

#include <string.h>
#include "agg_basics.h"
#include "agg_rendering_buffer.h"




namespace agg
{

    //--------------------------------------------------------------color_conv
    template<class CopyRow> 
    void color_conv(rendering_buffer* dst, 
                    const rendering_buffer* src,
                    CopyRow copy_row_functor)
    {
        unsigned width = src->width();
        unsigned height = src->height();

        if(dst->width()  < width)  width  = dst->width();
        if(dst->height() < height) height = dst->height();

        if(width)
        {
            unsigned y;
            for(y = 0; y < height; y++)
            {
                copy_row_functor(dst->row(y), src->row(y), width);
            }
        }
    }


    //---------------------------------------------------------color_conv_row
    template<class CopyRow> 
    void color_conv_row(unsigned char* dst, 
                        const unsigned char* src,
                        unsigned width,
                        CopyRow copy_row_functor)
    {
        copy_row_functor(dst, src, width);
    }


    //---------------------------------------------------------color_conv_same
    template<int BPP> class color_conv_same
    {
    public:
        void operator () (unsigned char* dst, 
                          const unsigned char* src,
                          unsigned width) const
        {
            memmove(dst, src, width*BPP);
        }
    };


}



#endif
