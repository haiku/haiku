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
// color type gray8
//
//----------------------------------------------------------------------------

#ifndef AGG_GRAY8_INCLUDED
#define AGG_GRAY8_INCLUDED

#include "agg_basics.h"
#include "agg_color_rgba.h"
#include "agg_color_rgba8.h"

namespace agg
{

    //===================================================================gray8
    struct gray8 
    {
        int8u v;
        int8u a;

        //--------------------------------------------------------------------
        gray8() {}

        //--------------------------------------------------------------------
        gray8(unsigned v_, unsigned a_=255) :
            v(int8u(v_)), a(int8u(a_)) {}

        //--------------------------------------------------------------------
        gray8(const rgba& c) :
            v(int8u((0.299*c.r + 0.587*c.g + 0.114*c.b) * 255.0 + 0.5)),
            a(int8u(c.a*255.0)) {}

        //--------------------------------------------------------------------
        gray8(const rgba8& c) :
            v((c.r*77 + c.g*150 + c.b*29) >> 8),
            a(c.a) {}

        //--------------------------------------------------------------------
        void clear()
        {
            v = a = 0;
        }

        //--------------------------------------------------------------------
        const gray8& transparent()
        {
            a = 0;
            return *this;
        }

        //--------------------------------------------------------------------
        void opacity(double a_)
        {
            if(a_ < 0.0) a_ = 0.0;
            if(a_ > 1.0) a_ = 1.0;
            a = int8u(a_ * 255.0);
        }

        //--------------------------------------------------------------------
        double opacity() const
        {
            return double(a) / 255.0;
        }

        //--------------------------------------------------------------------
        gray8 gradient(gray8 c, double k) const
        {
            gray8 ret;
            int ik = int(k * 256);
            ret.v = int8u(int(v) + (((int(c.v) - int(v)) * ik) >> 8));
            ret.a = int8u(int(a) + (((int(c.a) - int(a)) * ik) >> 8));
            return ret;
        }

        //--------------------------------------------------------------------
        gray8 pre() const
        {
            return gray8((v*a) >> 8, a);

        }

        //--------------------------------------------------------------------
        static gray8 no_color() { return gray8(0,0); }
    };


}




#endif
