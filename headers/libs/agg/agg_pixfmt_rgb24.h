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

#ifndef AGG_PIXFMT_RGB24_INCLUDED
#define AGG_PIXFMT_RGB24_INCLUDED

#include <string.h>
#include "agg_basics.h"
#include "agg_color_rgba8.h"
#include "agg_rendering_buffer.h"

namespace agg
{
    
    //=====================================================pixel_formats_rgb24
    template<class Order> class pixel_formats_rgb24
    {
    public:
        typedef rgba8 color_type;

        //--------------------------------------------------------------------
        pixel_formats_rgb24(rendering_buffer& rb)
            : m_rbuf(&rb)
        {
        }

        //--------------------------------------------------------------------
        unsigned width()  const { return m_rbuf->width();  }
        unsigned height() const { return m_rbuf->height(); }

        //--------------------------------------------------------------------
        color_type pixel(int x, int y)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
            return color_type(p[Order::R], p[Order::G], p[Order::B]);
        }

        //--------------------------------------------------------------------
        void copy_pixel(int x, int y, const color_type& c)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
            p[Order::R] = (int8u)c.r;
            p[Order::G] = (int8u)c.g;
            p[Order::B] = (int8u)c.b;
        }

        //--------------------------------------------------------------------
        void blend_pixel(int x, int y, const color_type& c, int8u cover)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
            int alpha = int(cover) * c.a;
            if(alpha == 255*255)
            {
                p[Order::R] = (int8u)c.r;
                p[Order::G] = (int8u)c.g;
                p[Order::B] = (int8u)c.b;
            }
            else
            {
                int r = p[Order::R];
                int g = p[Order::G];
                int b = p[Order::B];
                p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
                p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
                p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
            }
        }


        //--------------------------------------------------------------------
        void copy_hline(int x, int y, 
                        unsigned len, 
                        const color_type& c)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
            do
            {
                p[Order::R] = (int8u)c.r; 
                p[Order::G] = (int8u)c.g; 
                p[Order::B] = (int8u)c.b;
                p += 3;
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void copy_vline(int x, int y,
                        unsigned len, 
                        const color_type& c)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
            do
            {
                p[Order::R] = (int8u)c.r; 
                p[Order::G] = (int8u)c.g; 
                p[Order::B] = (int8u)c.b;
                p += m_rbuf->stride();
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void blend_hline(int x, int y,
                         unsigned len, 
                         const color_type& c,
                         int8u cover)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
            int alpha = int(cover) * c.a;
            if(alpha == 255*255)
            {
                do
                {
                    p[Order::R] = (int8u)c.r; 
                    p[Order::G] = (int8u)c.g; 
                    p[Order::B] = (int8u)c.b;
                    p += 3;
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
                    p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
                    p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
                    p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
                    p += 3;
                }
                while(--len);
            }
        }


        //--------------------------------------------------------------------
        void blend_vline(int x, int y,
                         unsigned len, 
                         const color_type& c,
                         int8u cover)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
            int alpha = int(cover) * c.a;
            if(alpha == 255*255)
            {
                do
                {
                    p[Order::R] = (int8u)c.r; 
                    p[Order::G] = (int8u)c.g; 
                    p[Order::B] = (int8u)c.b;
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
                    p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
                    p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
                    p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
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
            memmove(m_rbuf->row(ydst) + xdst * 3, 
                    (const void*)(from.row(ysrc) + xsrc * 3), len * 3);
        }


        //--------------------------------------------------------------------
        void blend_solid_hspan(int x, int y,
                               unsigned len, 
                               const color_type& c,
                               const int8u* covers)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
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
                    }
                    else
                    {
                        int r = (int8u)p[Order::R];
                        int g = (int8u)p[Order::G];
                        int b = (int8u)p[Order::B];
                        p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
                        p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
                        p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
                    }
                }
                p += 3;
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void blend_solid_vspan(int x, int y,
                               unsigned len, 
                               const color_type& c,
                               const int8u* covers)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
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
                    }
                    else
                    {
                        int r = p[Order::R];
                        int g = p[Order::G];
                        int b = p[Order::B];
                        p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
                        p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
                        p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
                    }
                }
                p += m_rbuf->stride();
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void blend_color_hspan(int x, int y,
                               unsigned len, 
                               const color_type* colors,
                               const int8u* covers,
                               const int8u cover)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
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
                    }
                    else
                    {
                        int r = p[Order::R];
                        int g = p[Order::G];
                        int b = p[Order::B];
                        p[Order::R] = (int8u)((((colors->r - r) * alpha) + (r << 16)) >> 16);
                        p[Order::G] = (int8u)((((colors->g - g) * alpha) + (g << 16)) >> 16);
                        p[Order::B] = (int8u)((((colors->b - b) * alpha) + (b << 16)) >> 16);
                    }
                }
                p += 3;
                ++colors;
            }
            while(--len);
        }



        //--------------------------------------------------------------------
        void blend_color_vspan(int x, int y,
                               unsigned len, 
                               const color_type* colors,
                               const int8u* covers,
                               const int8u cover)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
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
                    }
                    else
                    {
                        int r = p[Order::R];
                        int g = p[Order::G];
                        int b = p[Order::B];
                        p[Order::R] = (int8u)((((colors->r - r) * alpha) + (r << 16)) >> 16);
                        p[Order::G] = (int8u)((((colors->g - g) * alpha) + (g << 16)) >> 16);
                        p[Order::B] = (int8u)((((colors->b - b) * alpha) + (b << 16)) >> 16);
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


    typedef pixel_formats_rgb24<order_rgb24> pixfmt_rgb24;    //----pixfmt_rgb24
    typedef pixel_formats_rgb24<order_bgr24> pixfmt_bgr24;    //----pixfmt_bgr24

}

#endif

