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

#ifndef AGG_PIXFMT_GRAY8_INCLUDED
#define AGG_PIXFMT_GRAY8_INCLUDED

#include <string.h>
#include "agg_basics.h"
#include "agg_gray8.h"
#include "agg_rendering_buffer.h"

namespace agg
{

    //======================================================pixfmt_gray8_base
    template<unsigned Step=1, unsigned Offset=0>
    class pixfmt_gray8_base
    {
    public:
        typedef gray8 color_type;

        //--------------------------------------------------------------------
        pixfmt_gray8_base(rendering_buffer& rb)
            : m_rbuf(&rb)
        {
        }

        //--------------------------------------------------------------------
        unsigned width()  const { return m_rbuf->width();  }
        unsigned height() const { return m_rbuf->height(); }

        //--------------------------------------------------------------------
        color_type pixel(int x, int y)
        {
            return color_type(m_rbuf->row(y)[x * Step + Offset]);
        }

        //--------------------------------------------------------------------
        void copy_pixel(int x, int y, const color_type& c)
        {
            m_rbuf->row(y)[x * Step + Offset] = (int8u)c.v;
        }

        //--------------------------------------------------------------------
        void blend_pixel(int x, int y, const color_type& c, int8u cover)
        {
            int8u* p = m_rbuf->row(y) + x * Step + Offset;
            int v = *p;
            int alpha = int(cover) * c.a;
            *p = (int8u)((((c.v - v) * alpha) + (v << 16)) >> 16);
        }


        //--------------------------------------------------------------------
        void copy_hline(int x, int y, unsigned len, const color_type& c)
        {
            int8u* p = m_rbuf->row(y) + x * Step + Offset;
            do
            {
                *p = (int8u)c.v; 
                p += Step;
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void copy_vline(int x, int y, unsigned len, const color_type& c)
        {
            int8u* p = m_rbuf->row(y) + x * Step + Offset;
            do
            {
                *p = (int8u)c.v; 
                p += m_rbuf->stride();
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void blend_hline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {
            int8u* p = m_rbuf->row(y) + x * Step + Offset;
            int alpha = int(cover) * c.a;
            if(alpha == 255*255)
            {
                do
                {
                    *p = (int8u)c.v; 
                    p += Step;
                }
                while(--len);
            }
            else
            {
                do
                {
                    int v = *p;
                    *p = (int8u)((((c.v - v) * alpha) + (v << 16)) >> 16);
                    p += Step;
                }
                while(--len);
            }
        }


        //--------------------------------------------------------------------
        void blend_vline(int x, int y, unsigned len, 
                         const color_type& c, int8u cover)
        {
            int8u* p = m_rbuf->row(y) + x * Step + Offset;
            int alpha = int(cover) * c.a;
            if(alpha == 255*255)
            {
                do
                {
                    *p = (int8u)c.v; 
                    p += m_rbuf->stride();
                }
                while(--len);
            }
            else
            {
                do
                {
                    int v = *p;
                    *p = (int8u)((((c.v - v) * alpha) + (v << 16)) >> 16);
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
            memmove(m_rbuf->row(ydst) + xdst * Step + Offset, 
                    (const void*)(from.row(ysrc) + xsrc * Step + Offset),
                    len * Step);
        }


        //--------------------------------------------------------------------
        void blend_solid_hspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
        {
            int8u* p = m_rbuf->row(y) + x * Step + Offset;
            do 
            {
                int alpha = int(*covers++) * c.a;

                if(alpha)
                {
                    if(alpha == 255*255)
                    {
                        *p = (int8u)c.v; 
                    }
                    else
                    {
                        int v = *p;
                        *p = (int8u)((((c.v - v) * alpha) + (v << 16)) >> 16);
                    }
                }
                p += Step;
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void blend_solid_vspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
        {
            int8u* p = m_rbuf->row(y) + x * Step + Offset;
            do 
            {
                int alpha = int(*covers++) * c.a;

                if(alpha)
                {
                    if(alpha == 255*255)
                    {
                        *p = (int8u)c.v; 
                    }
                    else
                    {
                        int v = *p;
                        *p = (int8u)((((c.v - v) * alpha) + (v << 16)) >> 16);
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
            int8u* p = m_rbuf->row(y) + x * Step + Offset;
            do 
            {
                int alpha = colors->a * (covers ? int(*covers++) : int(cover));

                if(alpha)
                {
                    if(alpha == 255*255)
                    {
                        *p = (int8u)colors->v; 
                    }
                    else
                    {
                        int v = *p;
                        *p = (int8u)((((colors->v - v) * alpha) + (v << 16)) >> 16);
                    }
                }
                p += Step;
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
            int8u* p = m_rbuf->row(y) + x * Step + Offset;
            do 
            {
                int alpha = colors->a * (covers ? int(*covers++) : int(cover));

                if(alpha)
                {
                    if(alpha == 255*255)
                    {
                        *p = (int8u)colors->v; 
                    }
                    else
                    {
                        int v = *p;
                        *p = (int8u)((((colors->v - v) * alpha) + (v << 16)) >> 16);
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


    typedef pixfmt_gray8_base<1, 0> pixfmt_gray8;         //----pixfmt_gray8

    typedef pixfmt_gray8_base<3, 0> pixfmt_gray8_rgb24r;  //----pixfmt_gray8_rgb24r
    typedef pixfmt_gray8_base<3, 1> pixfmt_gray8_rgb24g;  //----pixfmt_gray8_rgb24g
    typedef pixfmt_gray8_base<3, 2> pixfmt_gray8_rgb24b;  //----pixfmt_gray8_rgb24b

    typedef pixfmt_gray8_base<3, 2> pixfmt_gray8_bgr24r;  //----pixfmt_gray8_bgr24r
    typedef pixfmt_gray8_base<3, 1> pixfmt_gray8_bgr24g;  //----pixfmt_gray8_bgr24g
    typedef pixfmt_gray8_base<3, 0> pixfmt_gray8_bgr24b;  //----pixfmt_gray8_bgr24b

    typedef pixfmt_gray8_base<4, 0> pixfmt_gray8_rgba32r; //----pixfmt_gray8_rgba32r
    typedef pixfmt_gray8_base<4, 1> pixfmt_gray8_rgba32g; //----pixfmt_gray8_rgba32g
    typedef pixfmt_gray8_base<4, 2> pixfmt_gray8_rgba32b; //----pixfmt_gray8_rgba32b
    typedef pixfmt_gray8_base<4, 3> pixfmt_gray8_rgba32a; //----pixfmt_gray8_rgba32a

    typedef pixfmt_gray8_base<4, 1> pixfmt_gray8_argb32r; //----pixfmt_gray8_argb32r
    typedef pixfmt_gray8_base<4, 2> pixfmt_gray8_argb32g; //----pixfmt_gray8_argb32g
    typedef pixfmt_gray8_base<4, 3> pixfmt_gray8_argb32b; //----pixfmt_gray8_argb32b
    typedef pixfmt_gray8_base<4, 0> pixfmt_gray8_argb32a; //----pixfmt_gray8_argb32a

    typedef pixfmt_gray8_base<4, 2> pixfmt_gray8_bgra32r; //----pixfmt_gray8_bgra32r
    typedef pixfmt_gray8_base<4, 1> pixfmt_gray8_bgra32g; //----pixfmt_gray8_bgra32g
    typedef pixfmt_gray8_base<4, 0> pixfmt_gray8_bgra32b; //----pixfmt_gray8_bgra32b
    typedef pixfmt_gray8_base<4, 3> pixfmt_gray8_bgra32a; //----pixfmt_gray8_bgra32a

    typedef pixfmt_gray8_base<4, 3> pixfmt_gray8_abgr32r; //----pixfmt_gray8_abgr32r
    typedef pixfmt_gray8_base<4, 2> pixfmt_gray8_abgr32g; //----pixfmt_gray8_abgr32g
    typedef pixfmt_gray8_base<4, 1> pixfmt_gray8_abgr32b; //----pixfmt_gray8_abgr32b
    typedef pixfmt_gray8_base<4, 0> pixfmt_gray8_abgr32a; //----pixfmt_gray8_abgr32a

}

#endif

