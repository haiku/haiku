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

#ifndef AGG_PIXFMT_CRB_RGBA32_INCLUDED
#define AGG_PIXFMT_CRB_RGBA32_INCLUDED

#include <string.h>
#include "agg_basics.h"
#include "agg_color_rgba8.h"

namespace agg
{

    //================================================pixel_formats_crb_rgba32
    template<class RenBuf, class Order> class pixel_formats_crb_rgba32
    {
    public:
        typedef rgba8  color_type;
        typedef RenBuf buffer_type;
        typedef Order  order_type;

        typedef typename RenBuf::row_data row_data;

        //--------------------------------------------------------------------
        pixel_formats_crb_rgba32(RenBuf& rb) : m_rbuf(&rb) {}

        //--------------------------------------------------------------------
        unsigned width()  const { return m_rbuf->width();  }
        unsigned height() const { return m_rbuf->height(); }

        //--------------------------------------------------------------------
        color_type pixel(int x, int y) const
        {
            const int8u* p = m_rbuf->span_ptr(x, y, 1);
            return p ? color_type(p[Order::R], p[Order::G], p[Order::B], p[Order::A]) :
                       color_type::no_color();
        }

        //--------------------------------------------------------------------
        row_data span(int x, int y) const
        {
            return m_rbuf->span(x, y);
        }

        //--------------------------------------------------------------------
        void copy_pixel(int x, int y, const color_type& c)
        {
            int8u* p = m_rbuf->span_ptr(x, y, 1);
            p[Order::R] = (int8u)c.r;
            p[Order::G] = (int8u)c.g;
            p[Order::B] = (int8u)c.b;
            p[Order::A] = (int8u)c.a;
        }

        //--------------------------------------------------------------------
        void blend_pixel(int x, int y, const color_type& c, int8u cover)
        {
            int8u* p = m_rbuf->span_ptr(x, y, 1);
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
            int32u* p32  = (int32u*)(m_rbuf->span_ptr(x, y, len));
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
            do
            {
                *(int32u*)(m_rbuf->span_ptr(x, y, 1)) = v;
                ++y;
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
                int32u* p32  = (int32u*)(m_rbuf->span_ptr(x, y, len));
                do
                {
                    *p32++ = v;
                }
                while(--len);
            }
            else
            {
                int8u* p = m_rbuf->span_ptr(x, y, len);
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
                    *(int32u*)(m_rbuf->span_ptr(x, y, 1)) = v;
                    ++y;
                }
                while(--len);
            }
            else
            {
                do
                {
                    int8u* p = m_rbuf->span_ptr(x, y, 1);
                    int r = p[Order::R];
                    int g = p[Order::G];
                    int b = p[Order::B];
                    int a = p[Order::A];
                    p[Order::R] = (int8u)((((c.r - r) * alpha) + (r << 16)) >> 16);
                    p[Order::G] = (int8u)((((c.g - g) * alpha) + (g << 16)) >> 16);
                    p[Order::B] = (int8u)((((c.b - b) * alpha) + (b << 16)) >> 16);
                    p[Order::A] = (int8u)(((alpha + (a << 8)) - ((alpha * a) >> 8)) >> 8);
                    ++y;
                }
                while(--len);
            }
        }



        //--------------------------------------------------------------------
        template<class RenBuf2> void copy_from(const RenBuf2& from, 
                                               int xdst, int ydst,
                                               int xsrc, int ysrc,
                                               unsigned len)
        {
            const int8u* p = from.row(ysrc);
            if(p)
            {
                p += xsrc << 2;
                memmove(m_rbuf->span_ptr(xdst, ydst, len), p, len * 4);
            }
        }



        //--------------------------------------------------------------------
        template<class SrcPixelFormatRenderer> 
        void blend_from(const SrcPixelFormatRenderer& from, 
                        const int8u* psrc,
                        int xdst, int ydst,
                        int xsrc, int ysrc,
                        unsigned len)
        {
            typedef typename SrcPixelFormatRenderer::order_type src_order;
            int8u* pdst = m_rbuf->span_ptr(xdst, ydst, len);
            int incp = 4;
            if(xdst > xsrc)
            {
                psrc += (len-1) << 2;
                pdst += (len-1) << 2;
                incp = -4;
            }
            do 
            {
                int alpha = psrc[src_order::A];

                if(alpha)
                {
                    if(alpha == 255)
                    {
                        pdst[Order::R] = psrc[src_order::R];
                        pdst[Order::G] = psrc[src_order::G];
                        pdst[Order::B] = psrc[src_order::B];
                        pdst[Order::A] = psrc[src_order::A];
                    }
                    else
                    {
                        int r = pdst[Order::R];
                        int g = pdst[Order::G];
                        int b = pdst[Order::B];
                        int a = pdst[Order::A];
                        pdst[Order::R] = (int8u)((((psrc[src_order::R] - r) * alpha) + (r << 8)) >> 8);
                        pdst[Order::G] = (int8u)((((psrc[src_order::G] - g) * alpha) + (g << 8)) >> 8);
                        pdst[Order::B] = (int8u)((((psrc[src_order::B] - b) * alpha) + (b << 8)) >> 8);
                        pdst[Order::A] = (int8u)((alpha + a) - ((alpha * a) >> 8));
                    }
                }
                psrc += incp;
                pdst += incp;
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void blend_solid_hspan(int x, int y, unsigned len, 
                               const color_type& c, const int8u* covers)
        {
            int8u* p = m_rbuf->span_ptr(x, y, len);
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
            do 
            {
                int8u* p = m_rbuf->span_ptr(x, y, 1);
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
                ++y;
            }
            while(--len);
        }


        //--------------------------------------------------------------------
        void blend_color_hspan(int x, int y, unsigned len, 
                               const color_type* colors, 
                               const int8u* covers,
                               int8u cover)
        {
            int8u* p = m_rbuf->span_ptr(x, y, len);
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
            do 
            {
                int8u* p = m_rbuf->span_ptr(x, y, 1);
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
                ++y;
                ++colors;
            }
            while(--len);
        }

    private:
        RenBuf* m_rbuf;
    };


}

#endif

