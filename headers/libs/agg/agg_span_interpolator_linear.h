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

#ifndef AGG_SPAN_INTERPOLATOR_LINEAR_INCLUDED
#define AGG_SPAN_INTERPOLATOR_LINEAR_INCLUDED

#include "agg_basics.h"
#include "agg_dda_line.h"
#include "agg_trans_affine.h"

namespace agg
{

    //================================================span_interpolator_linear
    template<class Transformer = trans_affine, unsigned SubpixelShift = 8> 
    class span_interpolator_linear
    {
    public:
        typedef Transformer trans_type;

        enum
        {
            subpixel_shift = SubpixelShift,
            subpixel_size  = 1 << subpixel_shift
        };

        //--------------------------------------------------------------------
        span_interpolator_linear() {}
        span_interpolator_linear(const trans_type& trans) : m_trans(&trans) {}
        span_interpolator_linear(const trans_type& trans,
                                 double x, double y, unsigned len) :
            m_trans(&trans)
        {
            begin(x, y, len);
        }

        //----------------------------------------------------------------
        const trans_type& transformer() const { return *m_trans; }
        void transformer(const trans_type& trans) { m_trans = &trans; }

        //----------------------------------------------------------------
        void begin(double x, double y, unsigned len)
        {
            double tx;
            double ty;

            tx = x;
            ty = y;
            m_trans->transform(&tx, &ty);
            int x1 = int(tx * subpixel_size);
            int y1 = int(ty * subpixel_size);

            tx = x + len;
            ty = y;
            m_trans->transform(&tx, &ty);
            int x2 = int(tx * subpixel_size);
            int y2 = int(ty * subpixel_size);

            m_li_x = dda2_line_interpolator(x1, x2, len);
            m_li_y = dda2_line_interpolator(y1, y2, len);
        }

        //----------------------------------------------------------------
        void operator++()
        {
            ++m_li_x;
            ++m_li_y;
        }

        //----------------------------------------------------------------
        void coordinates(int* x, int* y) const
        {
            *x = m_li_x.y();
            *y = m_li_y.y();
        }

    private:
        const trans_type* m_trans;
        dda2_line_interpolator m_li_x;
        dda2_line_interpolator m_li_y;
    };

}



#endif


