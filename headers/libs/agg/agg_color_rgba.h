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
// color type rgba
//
//----------------------------------------------------------------------------

#ifndef AGG_COLOR_RGBA_INCLUDED
#define AGG_COLOR_RGBA_INCLUDED

#include <math.h>
#include "agg_basics.h"

namespace agg
{
    //====================================================================rgba
    struct rgba
    {
        enum premul { pre };

        double r;
        double g;
        double b;
        double a;

        //--------------------------------------------------------------------
        rgba() {}

        //--------------------------------------------------------------------
        rgba(double r_, double g_, double b_, double a_=1.0) :
            r(r_), g(g_), b(b_), a(a_) {}

        //--------------------------------------------------------------------
        rgba(premul, double r_, double g_, double b_, double a_=1.0) :
            r(r_), g(g_), b(b_), a(a_) 
        {
            premultiply();
        }

        //--------------------------------------------------------------------
        rgba(premul, const rgba& c) : r(c.r), g(c.g), b(c.b), a(c.a) 
        {
            premultiply();
        }

        //--------------------------------------------------------------------
        rgba(const rgba& c, double a_) : r(c.r), g(c.g), b(c.b), a(a_) {}

        //--------------------------------------------------------------------
        rgba(premul, const rgba& c, double a_) : r(c.r), g(c.g), b(c.b), a(a_) 
        {
            premultiply();
        }

        //--------------------------------------------------------------------
        void clear()
        {
            r = g = b = a = 0;
        }

        //--------------------------------------------------------------------
        const rgba& transparent()
        {
            a = 0.0;
            return *this;
        }

        //--------------------------------------------------------------------
        const rgba& transparent(premul)
        {
            clear();
            return *this;
        }

        //--------------------------------------------------------------------
        const rgba& opacity(double a_)
        {
            if(a_ < 0.0) a_ = 0.0;
            if(a_ > 1.0) a_ = 1.0;
            a = a_;
            return *this;
        }

        //--------------------------------------------------------------------
        const rgba& opacity(premul, double a_)
        {
            if(a_ < 0.0) a_ = 0.0;
            if(a_ > 1.0) a_ = 1.0;
            premultiply(a_);
            return *this;
        }

        //--------------------------------------------------------------------
        double opacity() const
        {
            return a;
        }

        //--------------------------------------------------------------------
        const rgba& premultiply()
        {
            r *= a;
            g *= a;
            b *= a;
            return *this;
        }

        //--------------------------------------------------------------------
        const rgba& premultiply(double a_)
        {
            if(a == 0.0 || a_ == 0.0)
            {
                r = g = b = a = 0.0;
                return *this;
            }
            a_ /= a;
            r *= a_;
            g *= a_;
            b *= a_;
            a  = a_;
            return *this;
        }

        //--------------------------------------------------------------------
        const rgba& demultiply()
        {
            if(a == 0)
            {
                r = g = b = 0;
                return *this;
            }
            double a_ = 1.0 / a;
            r *= a_;
            g *= a_;
            b *= a_;
            return *this;
        }


        //--------------------------------------------------------------------
        rgba gradient(rgba c, double k) const
        {
            rgba ret;
            ret.r = r + (c.r - r) * k;
            ret.g = g + (c.g - g) * k;
            ret.b = b + (c.b - b) * k;
            ret.a = a + (c.a - a) * k;
            return ret;
        }

        //--------------------------------------------------------------------
        static rgba no_color() { return rgba(0,0,0,0); }

        //--------------------------------------------------------------------
        static rgba from_wavelength(double wl, double gamma = 1.0);
	
        //--------------------------------------------------------------------
        rgba(double wavelen, double gamma=1.0)
        {
            *this = from_wavelength(wavelen, gamma);
        }

    };


    //------------------------------------------------------------------------
    inline rgba rgba_pre(double r, double g, double b, double a=1.0)
    {
        return rgba(rgba::pre, r, g, b, a);
    }

    //--------------------------------------------------------------------
    inline rgba rgba_pre(const rgba& c)
    {
        return rgba(rgba::pre, c);
    }

    //--------------------------------------------------------------------
    inline rgba rgba_pre(const rgba& c, double a)
    {
        return rgba(rgba::pre, c, a);
    }


    //------------------------------------------------------------------------
    inline rgba rgba::from_wavelength(double wl, double gamma)
    {
        rgba t(0.0, 0.0, 0.0);

        if(wl >= 380.0 && wl <= 440.0)
        {
            t.r = -1.0 * (wl - 440.0) / (440.0 - 380.0);
            t.b = 1.0;
        }
        else 
        if(wl >= 440.0 && wl <= 490.0)
        {
            t.g = (wl - 440.0) / (490.0 - 440.0);
            t.b = 1.0;
        }
        else
        if(wl >= 490.0 && wl <= 510.0)
        {
            t.g = 1.0;
            t.b = -1.0 * (wl - 510.0) / (510.0 - 490.0);
        }
        else
        if(wl >= 510.0 && wl <= 580.0)
        {
            t.r = (wl - 510.0) / (580.0 - 510.0);
            t.g = 1.0;
        }
        else
        if(wl >= 580.0 && wl <= 645.0)
        {
            t.r = 1.0;
            t.g = -1.0 * (wl - 645.0) / (645.0 - 580.0);
        }
        else
        if(wl >= 645.0 && wl <= 780.0)
        {
            t.r = 1.0;
        }

        double s = 1.0;
        if(wl > 700.0)       s = 0.3 + 0.7 * (780.0 - wl) / (780.0 - 700.0);
        else if(wl <  420.0) s = 0.3 + 0.7 * (wl - 380.0) / (420.0 - 380.0);

        t.r = pow(t.r * s, gamma);
        t.g = pow(t.g * s, gamma);
        t.b = pow(t.b * s, gamma);
        return t;
    }



}


#endif
