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
#ifndef AGG_LINE_AA_BASICS_INCLUDED
#define AGG_LINE_AA_BASICS_INCLUDED

#include <stdlib.h>
#include "agg_basics.h"

namespace agg
{

    // See Implementation agg_line_aa_basics.cpp 

    //-------------------------------------------------------------------------
    enum
    {
        line_subpixel_shift = 8,                        //----line_subpixel_shift
        line_subpixel_size  = 1 << line_subpixel_shift, //----line_subpixel_size 
        line_subpixel_mask  = line_subpixel_size - 1    //----line_subpixel_mask 
    };

    //-------------------------------------------------------------------------
    enum
    {
        line_mr_subpixel_shift = 4,                           //----line_mr_subpixel_shift
        line_mr_subpixel_size  = 1 << line_mr_subpixel_shift, //----line_mr_subpixel_size 
        line_mr_subpixel_mask  = line_mr_subpixel_size - 1    //----line_mr_subpixel_mask 
    };

    //------------------------------------------------------------------line_mr
    inline int line_mr(int x) 
    { 
        return x >> (line_subpixel_shift - line_mr_subpixel_shift); 
    }

    //-------------------------------------------------------------------line_hr
    inline int line_hr(int x) 
    { 
        return x << (line_subpixel_shift - line_mr_subpixel_shift); 
    }

    //---------------------------------------------------------------line_dbl_hr
    inline int line_dbl_hr(int x) 
    { 
        return x << line_subpixel_shift;
    }

    //---------------------------------------------------------------line_coord
    inline int line_coord(double x)
    {
        return int(x * line_subpixel_size);
    }

    //==========================================================line_parameters
    struct line_parameters
    {
        //---------------------------------------------------------------------
        line_parameters() {}
        line_parameters(int x1_, int y1_, int x2_, int y2_, int len_) :
            x1(x1_), y1(y1_), x2(x2_), y2(y2_), 
            dx(abs(x2_ - x1_)),
            dy(abs(y2_ - y1_)),
            sx((x2_ > x1_) ? 1 : -1),
            sy((y2_ > y1_) ? 1 : -1),
            vertical(dy >= dx),
            inc(vertical ? sy : sx),
            len(len_),
            octant((sy & 4) | (sx & 2) | int(vertical))
        {
        }

        //---------------------------------------------------------------------
        unsigned orthogonal_quadrant() const { return s_orthogonal_quadrant[octant]; }
        unsigned diagonal_quadrant()   const { return s_diagonal_quadrant[octant];   }

        //---------------------------------------------------------------------
        bool same_orthogonal_quadrant(const line_parameters& lp) const
        {
            return s_orthogonal_quadrant[octant] == s_orthogonal_quadrant[lp.octant];
        }

        //---------------------------------------------------------------------
        bool same_diagonal_quadrant(const line_parameters& lp) const
        {
            return s_diagonal_quadrant[octant] == s_diagonal_quadrant[lp.octant];
        }
        
        //---------------------------------------------------------------------
        int x1, y1, x2, y2, dx, dy, sx, sy;
        bool vertical;
        int inc;
        int len;
        int octant;

        //---------------------------------------------------------------------
        static int8u s_orthogonal_quadrant[8];
        static int8u s_diagonal_quadrant[8];
    };



    // See Implementation agg_line_aa_basics.cpp 

    //----------------------------------------------------------------bisectrix
    void bisectrix(const line_parameters& l1, 
                   const line_parameters& l2, 
                   int* x, int* y);

}

#endif
