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

#ifndef AGG_PIXFMT_RGB24_PRE_INCLUDED
#define AGG_PIXFMT_RGB24_PRE_INCLUDED

#include <string.h>
#include "agg_basics.h"
#include "agg_color_rgba8.h"
#include "agg_rendering_buffer.h"

namespace agg
{


    //==================================================pixel_formats_rgb24_pre
    template<class Order> class pixel_formats_rgb24_pre
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
        }

        //--------------------------------------------------------------------
        static inline void blend_pix(int8u* p, const color_type& c, 
                                     unsigned cover, unsigned alpha)
        {
            p[Order::R] = (int8u)((p[Order::R] * alpha + ((c.r * cover) << 8)) >> 16);
            p[Order::G] = (int8u)((p[Order::G] * alpha + ((c.g * cover) << 8)) >> 16);
            p[Order::B] = (int8u)((p[Order::B] * alpha + ((c.b * cover) << 8)) >> 16);
        }

        //--------------------------------------------------------------------
        static inline void copy_or_blend_pix(int8u* p, const color_type& c, unsigned cover)
        {
            unsigned alpha = 65535 - cover * c.a;

            if(alpha < 65535)
            {
                if(alpha <= 65535-255*255)
                {
                    copy_pix(p, c);
                }
                else
                {
                    blend_pix(p, c, cover, alpha);
                }
            }
        }

        //--------------------------------------------------------------------
        static inline void copy_or_blend_pix(int8u* p, const color_type& c)
        {
            unsigned alpha = 255 - c.a;
            if(alpha < 255)
            {
                if(alpha == 0)
                {
                    copy_pix(p, c);
                }
                else
                {
                    p[Order::R] = (int8u)((p[Order::R] * alpha + (c.r << 8)) >> 8);
                    p[Order::G] = (int8u)((p[Order::G] * alpha + (c.g << 8)) >> 8);
                    p[Order::B] = (int8u)((p[Order::B] * alpha + (c.b << 8)) >> 8);
                }
            }
        }



    public:
        //--------------------------------------------------------------------
        pixel_formats_rgb24_pre(rendering_buffer& rb)
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
            copy_or_blend_pix(m_rbuf->row(y) + x + x + x, c, cover);
        }

        //--------------------------------------------------------------------
        void copy_hline(int x, int y, unsigned len, const color_type& c)
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
        void copy_vline(int x, int y, unsigned len, const color_type& c)
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
        void blend_hline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
            unsigned alpha = unsigned(cover) * c.a;
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
                alpha = 65535 - alpha;
                do
                {
                    blend_pix(p, c, cover, alpha);
                    p += 3;
                }
                while(--len);
            }
        }

        //--------------------------------------------------------------------
        void blend_vline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {
            int8u* p = m_rbuf->row(y) + x + x + x;
            unsigned alpha = unsigned(cover) * c.a;
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
                alpha = 65535 - alpha;
                do
                {
                    blend_pix(p, c, cover, alpha);
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
        void blend_solid_hspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
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
        void blend_solid_vspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
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
        void blend_color_hspan(int x, int y, unsigned len, 
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
        void blend_color_vspan(int x, int y, unsigned len, 
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
    };

    typedef pixel_formats_rgb24_pre<order_rgb24> pixfmt_rgb24_pre; //----pixfmt_rgb24_pre
    typedef pixel_formats_rgb24_pre<order_bgr24> pixfmt_bgr24_pre; //----pixfmt_bgr24_pre
}


#endif
