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

#ifndef AGG_PIXFMT_RGBA32_INCLUDED
#define AGG_PIXFMT_RGBA32_INCLUDED

#include <string.h>
#include "agg_basics.h"
#include "agg_color_rgba8.h"
#include "agg_rendering_buffer.h"

namespace agg
{

    //====================================================pixel_formats_rgba32
    template<class Order> class pixel_formats_rgba32
    {
    public:
        typedef rgba8 color_type;

        //--------------------------------------------------------------------
        pixel_formats_rgba32(rendering_buffer& rb)
            : m_rbuf(&rb)
        {
        }

        //--------------------------------------------------------------------
        unsigned width()  const { return m_rbuf->width();  }
        unsigned height() const { return m_rbuf->height(); }

        //--------------------------------------------------------------------
        color_type pixel(int x, int y)
        {
            int8u* p = m_rbuf->row(y) + (x << 2);
            return color_type(p[Order::R], p[Order::G], p[Order::B], p[Order::A]);
        }

        //--------------------------------------------------------------------
        void copy_pixel(int x, int y, const color_type& c)
        {
            int8u* p = m_rbuf->row(y) + (x << 2);
            p[Order::R] = (int8u)c.r;
            p[Order::G] = (int8u)c.g;
            p[Order::B] = (int8u)c.b;
            p[Order::A] = (int8u)c.a;
        }

        //--------------------------------------------------------------------
        void blend_pixel(int x, int y, const color_type& c, int8u cover)
        {
            int8u* p = m_rbuf->row(y) + (x << 2);
            int alpha = int(cover) * int(c.a);
            if(alpha == 255*255)
            {
                p[Order::R] = (int8u)c.r;
                p[Order::G] = (int8u)c.g;
                p[Order::B] = (int8u)c.b;
                p[Order::A] = (int8u)c.a;
            }
            else
            {
                int r = p[Order::R];
                int g = p[Order::G];
                int b = p[Order::B];
                int a = p[Order::A];
                p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
                p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
                p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
                p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
            }
        }

        //--------------------------------------------------------------------
        void copy_hline(int x, int y, unsigned len, const color_type& c)
        {
            int32u v;
            int8u* p8 = (int8u*)&v;
            p8[Order::R] = (int8u)c.r;
            p8[Order::G] = (int8u)c.g;
            p8[Order::B] = (int8u)c.b;
            p8[Order::A] = (int8u)c.a;
            int32u* p32 = (int32u*)(m_rbuf->row(y)) + x;
            do
            {
                *p32++ = v;
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void copy_vline(int x, int y, unsigned len, const color_type& c)
        {
            int32u v;
            int8u* p8 = (int8u*)&v;
            p8[Order::R] = (int8u)c.r;
            p8[Order::G] = (int8u)c.g;
            p8[Order::B] = (int8u)c.b;
            p8[Order::A] = (int8u)c.a;
            int8u* p = m_rbuf->row(y) + (x << 2);
            do
            {
                *(int32u*)p = v; 
                p += m_rbuf->stride();
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void blend_hline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {
            int alpha = int(cover) * int(c.a);
            if(alpha == 255*255)
            {
                int32u v;
                int8u* p8 = (int8u*)&v;
                p8[Order::R] = (int8u)c.r;
                p8[Order::G] = (int8u)c.g;
                p8[Order::B] = (int8u)c.b;
                p8[Order::A] = (int8u)c.a;
                int32u* p32 = (int32u*)(m_rbuf->row(y)) + x;
                do
                {
                    *p32++ = v;
                }
                while(--len);
            }
            else
            {
                int8u* p = m_rbuf->row(y) + (x << 2);
                do
                {
                    int r = p[Order::R];
                    int g = p[Order::G];
                    int b = p[Order::B];
                    int a = p[Order::A];
                    p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
                    p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
                    p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
                    p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
                    p += 4;
                }
                while(--len);
            }
        }

        //--------------------------------------------------------------------
        void blend_vline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {
            int8u* p = m_rbuf->row(y) + (x << 2);
            int alpha = int(cover) * c.a;
            if(alpha == 255*255)
            {
                int32u v;
                int8u* p8 = (int8u*)&v;
                p8[Order::R] = (int8u)c.r;
                p8[Order::G] = (int8u)c.g;
                p8[Order::B] = (int8u)c.b;
                p8[Order::A] = (int8u)c.a;
                do
                {
                    *(int32u*)p = v; 
                    p += m_rbuf->stride();
                }
                while(--len);
            }
            else
            {
                do
                {
                    int r = p[Order::R];
                    int g = p[Order::G];
                    int b = p[Order::B];
                    int a = p[Order::A];
                    p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
                    p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
                    p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
                    p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
                    p += m_rbuf->stride();
                }
                while(--len);
            }
        }


        //--------------------------------------------------------------------
        void copy_from(const rendering_buffer& from, 
                       int xdst, int ydst,
                       int xsrc, int ysrc,
                       unsigned len)
        {
            memmove(m_rbuf->row(ydst) + xdst * 4, 
                    (const void*)(from.row(ysrc) + xsrc * 4), len * 4);
        }


        //--------------------------------------------------------------------
        void blend_solid_hspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
        {
            int8u* p = m_rbuf->row(y) + (x << 2);
            do 
            {
                int alpha = int(*covers++) * c.a;

                if(alpha)
                {
                    if(alpha == 255*255)
                    {
                        p[Order::R] = (int8u)c.r;
                        p[Order::G] = (int8u)c.g;
                        p[Order::B] = (int8u)c.b;
                        p[Order::A] = (int8u)c.a;
                    }
                    else
                    {
                        int r = p[Order::R];
                        int g = p[Order::G];
                        int b = p[Order::B];
                        int a = p[Order::A];
                        p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
                        p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
                        p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
                        p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
                    }
                }
                p += 4;
            }
            while(--len);
        }



        //--------------------------------------------------------------------
        void blend_solid_vspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
        {
            int8u* p = m_rbuf->row(y) + (x << 2);
            do 
            {
                int alpha = int(*covers++) * c.a;

                if(alpha)
                {
                    if(alpha == 255*255)
                    {
                        p[Order::R] = (int8u)c.r;
                        p[Order::G] = (int8u)c.g;
                        p[Order::B] = (int8u)c.b;
                        p[Order::A] = (int8u)c.a;
                    }
                    else
                    {
                        int r = p[Order::R];
                        int g = p[Order::G];
                        int b = p[Order::B];
                        int a = p[Order::A];
                        p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
                        p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
                        p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
                        p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
                    }
                }
                p += m_rbuf->stride();
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void blend_color_hspan(int x, int y, unsigned len, 
                               const color_type* colors, 
                               const int8u* covers,
                               int8u cover)
        {
            int8u* p = m_rbuf->row(y) + (x << 2);
            do 
            {
                int alpha = colors->a * (covers ? int(*covers++) : int(cover));

                if(alpha)
                {
                    if(alpha == 255*255)
                    {
                        p[Order::R] = (int8u)colors->r;
                        p[Order::G] = (int8u)colors->g;
                        p[Order::B] = (int8u)colors->b;
                        p[Order::A] = (int8u)colors->a;
                    }
                    else
                    {
                        int r = p[Order::R];
                        int g = p[Order::G];
                        int b = p[Order::B];
                        int a = p[Order::A];
                        p[Order::R] = (int8u)((((colors->r - r) * alpha) + (r << 16)) >> 16);
                        p[Order::G] = (int8u)((((colors->g - g) * alpha) + (g << 16)) >> 16);
                        p[Order::B] = (int8u)((((colors->b - b) * alpha) + (b << 16)) >> 16);
                        p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
                    }
                }
                p += 4;
                ++colors;
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void blend_color_vspan(int x, int y, unsigned len, 
                               const color_type* colors, 
                               const int8u* covers,
                               int8u cover)
        {
            int8u* p = m_rbuf->row(y) + (x << 2);
            do 
            {
                int alpha = colors->a * (covers ? int(*covers++) : int(cover));

                if(alpha)
                {
                    if(alpha == 255*255)
                    {
                        p[Order::R] = (int8u)colors->r;
                        p[Order::G] = (int8u)colors->g;
                        p[Order::B] = (int8u)colors->b;
                        p[Order::A] = (int8u)colors->a;
                    }
                    else
                    {
                        int r = p[Order::R];
                        int g = p[Order::G];
                        int b = p[Order::B];
                        int a = p[Order::A];
                        p[Order::R] = (int8u)((((colors->r - r) * alpha) + (r << 16)) >> 16);
                        p[Order::G] = (int8u)((((colors->g - g) * alpha) + (g << 16)) >> 16);
                        p[Order::B] = (int8u)((((colors->b - b) * alpha) + (b << 16)) >> 16);
                        p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
                    }
                }
                p += m_rbuf->stride();
                ++colors;
            }
            while(--len);
        }

    private:
        rendering_buffer* m_rbuf;
    };

    typedef pixel_formats_rgba32<order_rgba32> pixfmt_rgba32; //----pixfmt_rgba32
    typedef pixel_formats_rgba32<order_argb32> pixfmt_argb32; //----pixfmt_argb32
    typedef pixel_formats_rgba32<order_abgr32> pixfmt_abgr32; //----pixfmt_abgr32
    typedef pixel_formats_rgba32<order_bgra32> pixfmt_bgra32; //----pixfmt_bgra32
}

#endif

