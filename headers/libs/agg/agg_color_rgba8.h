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
// color type rgba8
//
//----------------------------------------------------------------------------

#ifndef AGG_COLOR_RGBA8_INCLUDED
#define AGG_COLOR_RGBA8_INCLUDED

#include "agg_basics.h"
#include "agg_color_rgba.h"

namespace agg
{

    // Supported byte orders for RGB and RGBA pixel formats
    //=======================================================================
    struct order_rgb24  { enum { R=0, G=1, B=2, rgb24_tag }; };       //----order_rgb24
    struct order_bgr24  { enum { B=0, G=1, R=2, rgb24_tag }; };       //----order_bgr24
    struct order_rgba32 { enum { R=0, G=1, B=2, A=3, rgba32_tag }; }; //----order_rgba32
    struct order_argb32 { enum { A=0, R=1, G=2, B=3, rgba32_tag }; }; //----order_argb32
    struct order_abgr32 { enum { A=0, B=1, G=2, R=3, rgba32_tag }; }; //----order_abgr32
    struct order_bgra32 { enum { B=0, G=1, R=2, A=3, rgba32_tag }; }; //----order_bgra32

    //==================================================================rgba8
    struct rgba8 
    {
        enum order  { rgb, bgr  };
        enum premul { pre };

        int8u r;
        int8u g;
        int8u b;
        int8u a;

        //--------------------------------------------------------------------
        rgba8() {}

        //--------------------------------------------------------------------
        rgba8(unsigned r_, unsigned g_, unsigned b_, unsigned a_=255) :
            r(int8u(r_)), g(int8u(g_)), b(int8u(b_)), a(int8u(a_)) {}

        //--------------------------------------------------------------------
        rgba8(premul, unsigned r_, unsigned g_, unsigned b_, unsigned a_=255) :
            r(int8u(r_)), g(int8u(g_)), b(int8u(b_)), a(int8u(a_)) 
        {
            premultiply();
        }

        //--------------------------------------------------------------------
        rgba8(const rgba& c) :
            r(int8u(c.r * 255.0 + 0.5)), 
            g(int8u(c.g * 255.0 + 0.5)), 
            b(int8u(c.b * 255.0 + 0.5)), 
            a(int8u(c.a * 255.0 + 0.5)) {}

        //--------------------------------------------------------------------
        rgba8(premul, const rgba8& c) :
            r(int8u(c.r)), g(int8u(c.g)), b(int8u(c.b)), a(int8u(c.a)) 
        {
            premultiply();
        }

        //--------------------------------------------------------------------
        rgba8(const rgba8& c, unsigned a_) :
            r(int8u(c.r)), g(int8u(c.g)), b(int8u(c.b)), a(int8u(a_)) {}

        //--------------------------------------------------------------------
        rgba8(premul, const rgba8& c, unsigned a_) :
            r(int8u(c.r)), g(int8u(c.g)), b(int8u(c.b)), a(int8u(a_)) 
        {
            premultiply();
        }

        //--------------------------------------------------------------------
        rgba8(unsigned packed, order o) : 
            r(int8u((o == rgb) ? ((packed >> 16) & 0xFF) : (packed & 0xFF))),
            g(int8u((packed >> 8)  & 0xFF)),
            b(int8u((o == rgb) ? (packed & 0xFF) : ((packed >> 16) & 0xFF))),
            a(255) {}

        //--------------------------------------------------------------------
        rgba8(premul, unsigned packed, order o) : 
            r(int8u((o == rgb) ? ((packed >> 16) & 0xFF) : (packed & 0xFF))),
            g(int8u((packed >> 8)  & 0xFF)),
            b(int8u((o == rgb) ? (packed & 0xFF) : ((packed >> 16) & 0xFF))),
            a(255) 
        {
            premultiply();
        }

        //--------------------------------------------------------------------
        void clear()
        {
            r = g = b = a = 0;
        }
        
        //--------------------------------------------------------------------
        const rgba8& transparent()
        {
            a = 0;
            return *this;
        }

        //--------------------------------------------------------------------
        const rgba8& transparent(premul)
        {
            clear();
            return *this;
        }

        //--------------------------------------------------------------------
        const rgba8& opacity(double a_)
        {
            if(a_ < 0.0) a_ = 0.0;
            if(a_ > 1.0) a_ = 1.0;
            a = int8u(a_ * 255.0 + 0.5);
            return *this;
        }

        //--------------------------------------------------------------------
        const rgba8& opacity(premul, double a_)
        {
            if(a_ < 0.0) a_ = 0.0;
            if(a_ > 1.0) a_ = 1.0;
            a = int8u(a_ * 255.0 + 0.5);
            premultiply(int8u(a_ * 255.0));
            return *this;
        }

        //--------------------------------------------------------------------
        double opacity() const
        {
            return double(a) / 255.0;
        }

        //--------------------------------------------------------------------
        const rgba8& premultiply()
        {
            if(a == 255) return *this;
            if(a == 0)
            {
                r = g = b = 0;
                return *this;
            }
            r = (r * a) >> 8;
            g = (g * a) >> 8;
            b = (b * a) >> 8;
            return *this;
        }

        //--------------------------------------------------------------------
        const rgba8& premultiply(unsigned a_)
        {
            if(a == 255 && a_ >= 255) return *this;
            if(a == 0   || a_ == 0)
            {
                r = g = b = a = 0;
                return *this;
            }
            unsigned r_ = (r * a_) / a;
            unsigned g_ = (g * a_) / a;
            unsigned b_ = (b * a_) / a;
            r = int8u((r_ > a_) ? a_ : r_);
            g = int8u((g_ > a_) ? a_ : g_);
            b = int8u((b_ > a_) ? a_ : b_);
            a = int8u(a_);
            return *this;
        }

        //--------------------------------------------------------------------
        const rgba8& demultiply()
        {
            if(a == 255) return *this;
            if(a == 0)
            {
                r = g = b = 0;
                return *this;
            }
            unsigned r_ = (r * 255) / a;
            unsigned g_ = (g * 255) / a;
            unsigned b_ = (b * 255) / a;
            r = (r_ > 255) ? 255 : r_;
            g = (g_ > 255) ? 255 : g_;
            b = (b_ > 255) ? 255 : b_;
            return *this;
        }

        //--------------------------------------------------------------------
        rgba8 gradient(rgba8 c, double k) const
        {
            rgba8 ret;
            int ik = int(k * 256);
            ret.r = int8u(int(r) + (((int(c.r) - int(r)) * ik) >> 8));
            ret.g = int8u(int(g) + (((int(c.g) - int(g)) * ik) >> 8));
            ret.b = int8u(int(b) + (((int(c.b) - int(b)) * ik) >> 8));
            ret.a = int8u(int(a) + (((int(c.a) - int(a)) * ik) >> 8));
            return ret;
        }

        //--------------------------------------------------------------------
        static rgba8 no_color() { return rgba8(0,0,0,0); }


        //--------------------------------------------------------------------
        static rgba8 from_wavelength(double wl, double gamma = 1.0)
        {
            return rgba8(rgba::from_wavelength(wl, gamma));
        }
	
        //--------------------------------------------------------------------
        rgba8(double wavelen, double gamma=1.0)
        {
            *this = from_wavelength(wavelen, gamma);
        }
    };


    //------------------------------------------------------------------------
    inline rgba8 rgba8_pre(unsigned r, unsigned g, unsigned b, unsigned a=1.0)
    {
        return rgba8(rgba8::pre, r, g, b, a);
    }

    //--------------------------------------------------------------------
    inline rgba8 rgba8_pre(const rgba& c)
    {
        return rgba8(rgba8::pre, c);
    }

    //--------------------------------------------------------------------
    inline rgba8 rgba8_pre(const rgba8& c, unsigned a)
    {
        return rgba8(rgba8::pre, c, a);
    }


}




#endif
