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

#ifndef AGG_PIXFMT_RGBA32_PLAIN_INCLUDED
#define AGG_PIXFMT_RGBA32_PLAIN_INCLUDED

#include <string.h>
#include "agg_basics.h"
#include "agg_color_rgba8.h"
#include "agg_rendering_buffer.h"

namespace agg
{


    //===============================================pixel_formats_rgba32_plain
    template<class Order> class pixel_formats_rgba32_plain
    {
    public:
        typedef rgba8 color_type;


    private:
        //--------------------------------------------------------------------
        static inline void copy_pix(int8u* p, const color_type& c)
        {
            p[Order::R] = (int8u)c.r;
            p[Order::G] = (int8u)c.g;
            p[Order::B] = (int8u)c.b;
            p[Order::A] = (int8u)c.a;
        }

        //--------------------------------------------------------------------
        static inline void blend_pix(int8u* p, const color_type& c, unsigned alpha)
        {
            int a = p[Order::A];
            int r = p[Order::R] * a;
            int g = p[Order::G] * a;
            int b = p[Order::B] * a;
            a = ((alpha << 8) + (a << 16)) - alpha * a;
            p[Order::A] = (int8u)(a >> 16);
            p[Order::R] = (int8u)((((((c.r << 8) - r) * alpha) + (r << 16)) / a));
            p[Order::G] = (int8u)((((((c.g << 8) - g) * alpha) + (g << 16)) / a));
            p[Order::B] = (int8u)((((((c.b << 8) - b) * alpha) + (b << 16)) / a));
        }

        //--------------------------------------------------------------------
        static inline void copy_or_blend_pix(int8u* p, const color_type& c, unsigned cover)
        {
            unsigned alpha = cover * c.a;
// For testing 
//color_type c2 = c; 
//c2.a = alpha >> 8;
//copy_or_blend_pix(p, c2);

            if(alpha)
            {
                if(alpha == 255*255)
                {
                    copy_pix(p, c);
                }
                else
                {
                    blend_pix(p, c, alpha);
                }
            }
        }

        //--------------------------------------------------------------------
        static inline void copy_or_blend_pix(int8u* p, const color_type& c)
        {
            unsigned alpha = c.a;
            if(alpha)
            {
                if(alpha == 255)
                {
                    copy_pix(p, c);
                }
                else
                {
                    int a = p[Order::A];
                    int r = p[Order::R] * a;
                    int g = p[Order::G] * a;
                    int b = p[Order::B] * a;
                    a = ((alpha + a) << 8) - alpha * a;
                    p[Order::A] = (int8u)(a >> 8);
                    p[Order::R] = (int8u)((((((c.r << 8) - r) * alpha) + (r << 8)) / a));
                    p[Order::G] = (int8u)((((((c.g << 8) - g) * alpha) + (g << 8)) / a));
                    p[Order::B] = (int8u)((((((c.b << 8) - b) * alpha) + (b << 8)) / a));
                }
            }
        }



    public:
        //--------------------------------------------------------------------
        pixel_formats_rgba32_plain(rendering_buffer& rb)
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
            copy_or_blend_pix(m_rbuf->row(y) + (x << 2), c, cover);
        }

        //--------------------------------------------------------------------
        void copy_hline(int x, int y, unsigned len, const color_type& c)
        {
            int32u v;
            copy_pix((int8u*)&v, c);
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
            copy_pix((int8u*)&v, c);
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
            unsigned alpha = unsigned(cover) * c.a;
            if(alpha)
            {
                if(alpha == 255*255)
                {
                    int32u v;
                    copy_pix((int8u*)&v, c);
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
                        blend_pix(p, c, alpha);
                        p += 4;
                    }
                    while(--len);
                }
            }
        }

        //--------------------------------------------------------------------
        void blend_vline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {
            int8u* p = m_rbuf->row(y) + (x << 2);
            unsigned alpha = unsigned(cover) * c.a;
            if(alpha)
            {
                unsigned alpha = unsigned(cover) * c.a;
                if(alpha == 255*255)
                {
                    int32u v;
                    copy_pix((int8u*)&v, c);
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
                        blend_pix(p, c, alpha);
                        p += m_rbuf->stride();
                    }
                    while(--len);
                }
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
                copy_or_blend_pix(p, c, *covers++);
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
                copy_or_blend_pix(p, c, *covers++);
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
                copy_or_blend_pix(p, *colors++, covers ? *covers++ : cover);
                p += 4;
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
                copy_or_blend_pix(p, *colors++, covers ? *covers++ : cover);
                p += m_rbuf->stride();
            }
            while(--len);
        }

    private:
        rendering_buffer* m_rbuf;
    };

    typedef pixel_formats_rgba32_plain<order_rgba32> pixfmt_rgba32_plain; //----pixfmt_rgba32_plain
    typedef pixel_formats_rgba32_plain<order_argb32> pixfmt_argb32_plain; //----pixfmt_argb32_plain
    typedef pixel_formats_rgba32_plain<order_abgr32> pixfmt_abgr32_plain; //----pixfmt_abgr32_plain
    typedef pixel_formats_rgba32_plain<order_bgra32> pixfmt_bgra32_plain; //----pixfmt_bgra32_plain
}


#endif
