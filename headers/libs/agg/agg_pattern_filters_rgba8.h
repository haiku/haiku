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
#ifndef AGG_PATTERN_FILTERS_RGBA8_INCLUDED
#define AGG_PATTERN_FILTERS_RGBA8_INCLUDED

#include "agg_basics.h"
#include "agg_line_aa_basics.h"
#include "agg_color_rgba8.h"


namespace agg
{

    //=======================================================pattern_filter_nn
    template<class ColorT> struct pattern_filter_nn
    {
        typedef ColorT color_type;
        static unsigned dilation() { return 0; }

        static void pixel_low_res(color_type const* const* buf, 
                                  color_type* p, int x, int y)
        {
            *p = buf[y][x];
        }

        static void pixel_high_res(color_type const* const* buf, 
                                   color_type* p, int x, int y)
        {
            *p = buf[y >> line_subpixel_shift]
                    [x >> line_subpixel_shift];
        }
    };

    typedef pattern_filter_nn<rgba8> pattern_filter_nn_rgba8;



    //===========================================pattern_filter_bilinear_rgba8
    struct pattern_filter_bilinear_rgba8
    {
        typedef rgba8 color_type;
        static unsigned dilation() { return 1; }

        static void pixel_low_res(color_type const* const* buf, 
                           color_type* p, int x, int y)
        {
            *p = buf[y][x];
        }

        static void pixel_high_res(color_type const* const* buf, 
                                   color_type* p, int x, int y)
        {
            int r, g, b, a;
            r = g = b = a = line_subpixel_size * line_subpixel_size / 2;

            int weight;
            int x_lr = x >> line_subpixel_shift;
            int y_lr = y >> line_subpixel_shift;

            x &= line_subpixel_mask;
            y &= line_subpixel_mask;
            const color_type* ptr = buf[y_lr] + x_lr;

            weight = (line_subpixel_size - x) * 
                     (line_subpixel_size - y);
            r += weight * ptr->r;
            g += weight * ptr->g;
            b += weight * ptr->b;
            a += weight * ptr->a;

            ++ptr;

            weight = x * (line_subpixel_size - y);
            r += weight * ptr->r;
            g += weight * ptr->g;
            b += weight * ptr->b;
            a += weight * ptr->a;

            ptr = buf[y_lr + 1] + x_lr;

            weight = (line_subpixel_size - x) * y;
            r += weight * ptr->r;
            g += weight * ptr->g;
            b += weight * ptr->b;
            a += weight * ptr->a;

            ++ptr;

            weight = x * y;
            r += weight * ptr->r;
            g += weight * ptr->g;
            b += weight * ptr->b;
            a += weight * ptr->a;

            p->r = (int8u)(r >> line_subpixel_shift * 2);
            p->g = (int8u)(g >> line_subpixel_shift * 2);
            p->b = (int8u)(b >> line_subpixel_shift * 2);
            p->a = (int8u)(a >> line_subpixel_shift * 2);

        }
    };

}

#endif
