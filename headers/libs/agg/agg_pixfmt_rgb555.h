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

#ifndef AGG_PIXFMT_RGB555_INCLUDED
#define AGG_PIXFMT_RGB555_INCLUDED

#include <string.h>
#include "agg_basics.h"
#include "agg_color_rgba8.h"
#include "agg_rendering_buffer.h"

namespace agg
{

    //------------------------------------------------------------------rgb555
    inline int16u rgb555(unsigned r, unsigned g, unsigned b)
    {
        return (int16u)(((r & 0xF8) << 7) | ((g & 0xF8) << 2) | (b >> 3));
    }


    //===========================================================pixfmt_rgb555
    class pixfmt_rgb555
    {
    public:
        typedef rgba8 color_type;

        //--------------------------------------------------------------------
        pixfmt_rgb555(rendering_buffer& rb)
            : m_rbuf(&rb)
        {
        }

        //--------------------------------------------------------------------
        unsigned width()  const { return m_rbuf->width();  }
        unsigned height() const { return m_rbuf->height(); }

        //--------------------------------------------------------------------
        color_type pixel(int x, int y)
        {
            unsigned rgb = ((int16u*)(m_rbuf->row(y)))[x];
            return color_type((rgb >> 7) & 0xF8, 
                              (rgb >> 2) & 0xF8, 
                              (rgb << 3) & 0xF8);
        }

        //--------------------------------------------------------------------
        void copy_pixel(int x, int y, const color_type& c)
        {
            ((int16u*)(m_rbuf->row(y)))[x] = rgb555(c.r, c.g, c.b);
        }

        //--------------------------------------------------------------------
        void blend_pixel(int x, int y, const color_type& c, int8u cover)
        {
            int alpha = int(cover) * c.a;
            int16u* p = (int16u*)(m_rbuf->row(y)) + x;
            if(alpha == 255*255)
            {
                *p = rgb555(c.r, c.g, c.b);
            }
            else
            {
                int16u rgb = *p;
                int r = (rgb >> 7) & 0xF8;
                int g = (rgb >> 2) & 0xF8;
                int b = (rgb << 3) & 0xF8;
                *p = (int16u)
                   ((((((c.r - r) * alpha) + (r << 16)) >> 9) & 0x7C00) |
                    (((((c.g - g) * alpha) + (g << 16)) >> 14) & 0x3E0) |
                     ((((c.b - b) * alpha) + (b << 16)) >> 19));
            }
        }

        //--------------------------------------------------------------------
        void copy_hline(int x, int y, unsigned len, const color_type& c)
        {
            int16u* p = (int16u*)(m_rbuf->row(y)) + x;
            int16u v = rgb555(c.r, c.g, c.b);
            do
            {
                *p++ = v;
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void copy_vline(int x, int y,  unsigned len, const color_type& c)
        {
            int8u* p = m_rbuf->row(y) + (x << 1);
            int16u v = rgb555(c.r, c.g, c.b);
            do
            {
                *(int16u*)p = v;
                p += m_rbuf->stride();
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void blend_hline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {
            int16u* p = (int16u*)(m_rbuf->row(y)) + x;
            int alpha = int(cover) * c.a;
            if(alpha == 255*255)
            {
                int16u v = rgb555(c.r, c.g, c.b);
                do
                {
                    *p++ = v;
                }
                while(--len);
            }
            else
            {
                do
                {
                    int16u rgb = *p;
                    int r = (rgb >> 7) & 0xF8;
                    int g = (rgb >> 2) & 0xF8;
                    int b = (rgb << 3) & 0xF8;
                    *p++ = (int16u)
                       ((((((c.r - r) * alpha) + (r << 16)) >> 9) & 0x7C00) |
                        (((((c.g - g) * alpha) + (g << 16)) >> 14) & 0x3E0) |
                         ((((c.b - b) * alpha) + (b << 16)) >> 19));
                }
                while(--len);
            }
        }

        //--------------------------------------------------------------------
        void blend_vline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {
            int8u* p = m_rbuf->row(y) + (x << 1);
            int alpha = int(cover) * c.a;
            if(alpha == 255*255)
            {
                int16u v = rgb555(c.r, c.g, c.b);
                do
                {
                    *(int16u*)p = v;
                    p += m_rbuf->stride();
                }
                while(--len);
            }
            else
            {
                do
                {
                    int16u rgb = *(int16u*)p;
                    int r = (rgb >> 7) & 0xF8;
                    int g = (rgb >> 2) & 0xF8;
                    int b = (rgb << 3) & 0xF8;
                    *(int16u*)p = (int16u)
                        ((((((c.r - r) * alpha) + (r << 16)) >> 9) & 0x7C00) |
                         (((((c.g - g) * alpha) + (g << 16)) >> 14) & 0x3E0) |
                          ((((c.b - b) * alpha) + (b << 16)) >> 19));
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
            memmove(m_rbuf->row(ydst) + xdst * 2, 
                    (const void*)(from.row(ysrc) + xsrc * 2), len * 2);
        }

        //--------------------------------------------------------------------
        void blend_solid_hspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
        {
            int16u* p = (int16u*)(m_rbuf->row(y)) + x;
            do
            {
                int alpha = int(*covers++) * c.a;
                if(alpha)
                {
                    if(alpha == 255*255)
                    {
                        *p = rgb555(c.r, c.g, c.b);
                    }
                    else
                    {
                        int16u rgb = *p;
                        int r = (rgb >> 7) & 0xF8;
                        int g = (rgb >> 2) & 0xF8;
                        int b = (rgb << 3) & 0xF8;
                        *p = (int16u)
                            ((((((c.r - r) * alpha) + (r << 16)) >> 9) & 0x7C00) |
                             (((((c.g - g) * alpha) + (g << 16)) >> 14) & 0x3E0) |
                              ((((c.b - b) * alpha) + (b << 16)) >> 19));
                    }
                }
                ++p;
            }
            while(--len);
        }

        //--------------------------------------------------------------------
        void blend_solid_vspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
        {
            int8u* p = m_rbuf->row(y) + (x << 1);
            do
            {
                int alpha = int(*covers++) * c.a;

                if(alpha)
                {
                    if(alpha == 255*255)
                    {
                        *(int16u*)p = rgb555(c.r, c.g, c.b);
                    }
                    else
                    {
                        int16u rgb = *(int16u*)p;
                        int r = (rgb >> 7) & 0xF8;
                        int g = (rgb >> 2) & 0xF8;
                        int b = (rgb << 3) & 0xF8;
                        *(int16u*)p = (int16u)
                            ((((((c.r - r) * alpha) + (r << 16)) >> 9) & 0x7C00) |
                             (((((c.g - g) * alpha) + (g << 16)) >> 14) & 0x3E0) |
                              ((((c.b - b) * alpha) + (b << 16)) >> 19));
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
            int16u* p = (int16u*)(m_rbuf->row(y)) + x;
            do
            {
                int alpha = colors->a * (covers ? int(*covers++) : int(cover));
                if(alpha)
                {
                    if(alpha == 255*255)
                    {
                        *(int16u*)p = rgb555(colors->r, colors->g, colors->b);
                    }
                    else
                    {
                        int16u rgb = *p;
                        int r = (rgb >> 7) & 0xF8;
                        int g = (rgb >> 2) & 0xF8;
                        int b = (rgb << 3) & 0xF8;
                        *(int16u*)p = (int16u)
                            ((((((colors->r - r) * alpha) + (r << 16)) >> 9) & 0x7C00) |
                             (((((colors->g - g) * alpha) + (g << 16)) >> 14) & 0x3E0) |
                              ((((colors->b - b) * alpha) + (b << 16)) >> 19));
                    }
                }
                ++p;
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
            int8u* p = m_rbuf->row(y) + (x << 1);
            do
            {
                int alpha = colors->a * (covers ? int(*covers++) : int(cover));
                if(alpha)
                {
                    if(alpha == 255*255)
                    {
                        *(int16u*)p = rgb555(colors->r, colors->g, colors->b);
                    }
                    else
                    {
                        int16u rgb = *(int16u*)p;
                        int r = (rgb >> 7) & 0xF8;
                        int g = (rgb >> 2) & 0xF8;
                        int b = (rgb << 3) & 0xF8;
                        *(int16u*)p = (int16u)
                            ((((((colors->r - r) * alpha) + (r << 16)) >> 9) & 0x7C00) |
                             (((((colors->g - g) * alpha) + (g << 16)) >> 14) & 0x3E0) |
                              ((((colors->b - b) * alpha) + (b << 16)) >> 19));
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

}

#endif

