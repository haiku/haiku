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

#ifndef AGG_PIXFMT_RGB24_GAMMA_INCLUDED
#define AGG_PIXFMT_RGB24_GAMMA_INCLUDED

#include <string.h>
#include "agg_basics.h"
#include "agg_color_rgba8.h"
#include "agg_rendering_buffer.h"
#include "agg_gamma_lut.h"


namespace agg
{

    //================================================pixel_formats_rgb24_gamma
    template<class Order, class Gamma> class pixel_formats_rgb24_gamma
    {
    public:
        typedef rgba8 color_type;
        typedef Gamma gamma_type;

    private:
        //--------------------------------------------------------------------
        inline void blend_pix(int8u* p, const color_type& c, int alpha)
        {
            int r = m_gamma->dir(p[Order::R]);
            int g = m_gamma->dir(p[Order::G]);
            int b = m_gamma->dir(p[Order::B]);
            p[Order::R] = m_gamma->inv((((m_gamma->dir(c.r) - r) * alpha) + (r << 16)) >> 16);
            p[Order::G] = m_gamma->inv((((m_gamma->dir(c.g) - g) * alpha) + (g << 16)) >> 16);
            p[Order::B] = m_gamma->inv((((m_gamma->dir(c.b) - b) * alpha) + (b << 16)) >> 16);
        }

        //--------------------------------------------------------------------
        inline void copy_or_blend_pix(int8u* p, const color_type& c, int cover)
        {
            int alpha = int(c.a) * cover;
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
                    blend_pix(p, c, alpha);
                }
            }
        }

        //--------------------------------------------------------------------
        inline void copy_or_blend_pix(int8u* p, const color_type& c)
        {
            if(c.a)
            {
                if(c.a == 255)
                {
                    p[Order::R] = (int8u)c.r;
                    p[Order::G] = (int8u)c.g;
                    p[Order::B] = (int8u)c.b;
                }
                else
                {
                    int r = m_gamma->dir(p[Order::R]);
                    int g = m_gamma->dir(p[Order::G]);
                    int b = m_gamma->dir(p[Order::B]);
                    p[Order::R] = m_gamma->inv((((m_gamma->dir(c.r) - r) * c.a) + (r << 8)) >> 8);
                    p[Order::G] = m_gamma->inv((((m_gamma->dir(c.g) - g) * c.a) + (g << 8)) >> 8);
                    p[Order::B] = m_gamma->inv((((m_gamma->dir(c.b) - b) * c.a) + (b << 8)) >> 8);
                }
            }
        }



    public:
        //--------------------------------------------------------------------
        pixel_formats_rgb24_gamma(rendering_buffer& rb, const gamma_type& g) :
            m_rbuf(&rb),
            m_gamma(&g)
        {}

        //--------------------------------------------------------------------
        void gamma(const gamma_type& g) { m_gamma = &g; }
        const gamma_type& gamma() const { return *m_gamma; }

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
            copy_or_blend_pix(m_rbuf->row(y) + x + x + x, c, cover);
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
                    blend_pix(p, c, alpha);
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
                    blend_pix(p, c, alpha);
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
                copy_or_blend_pix(p, c, *covers++);
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
                copy_or_blend_pix(p, c, *covers++);
                p += m_rbuf->stride();
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void blend_color_hspan(int x, int y,
                               unsigned len, 
                               const color_type* colors,
                               const int8u* covers,
                               int8u cover)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
            do 
            {
                copy_or_blend_pix(p, *colors++, covers ? *covers++ : cover);
                p += 3;
            }
            while(--len);
        }



        //--------------------------------------------------------------------
        void blend_color_vspan(int x, int y,
                               unsigned len, 
                               const color_type* colors,
                               const int8u* covers,
                               int8u cover)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
            do 
            {
                copy_or_blend_pix(p, *colors++, covers ? *covers++ : cover);
                p += m_rbuf->stride();
            }
            while(--len);
        }

    private:
        rendering_buffer* m_rbuf;
        const gamma_type* m_gamma;
    };


    //-----------------------------------------------------pixfmt_rgb24_gamma
    template<class Gamma> class pixfmt_rgb24_gamma : 
    public pixel_formats_rgb24_gamma<order_rgb24, Gamma>
    {
    public:
        pixfmt_rgb24_gamma(rendering_buffer& rb, const Gamma& g) :
          pixel_formats_rgb24_gamma<order_rgb24, Gamma>(rb, g) {}
    };
        
    //-----------------------------------------------------pixfmt_bgr24_gamma
    template<class Gamma> class pixfmt_bgr24_gamma : 
    public pixel_formats_rgb24_gamma<order_bgr24, Gamma>
    {
    public:
        pixfmt_bgr24_gamma(rendering_buffer& rb, const Gamma& g) :
          pixel_formats_rgb24_gamma<order_bgr24, Gamma>(rb, g) {}
    };



}

#endif
