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
// classes span_pattern_filter_rgba32*
//
//----------------------------------------------------------------------------
#ifndef AGG_SPAN_PATTERN_FILTER_RGBA32_INCLUDED
#define AGG_SPAN_PATTERN_FILTER_RGBA32_INCLUDED

#include "agg_basics.h"
#include "agg_color_rgba8.h"
#include "agg_span_image_filter.h"


namespace agg
{

    //===========================================span_pattern_filter_rgba32_nn
    template<class Order, 
             class Interpolator,
             class RemainderX,
             class RemainderY,
             class Allocator = span_allocator<rgba8> > 
    class span_pattern_filter_rgba32_nn : 
    public span_image_filter<rgba8, Interpolator, Allocator>
    {
    public:
        typedef Interpolator interpolator_type;
        typedef Allocator alloc_type;
        typedef span_image_filter<rgba8, Interpolator, alloc_type> base_type;
        typedef rgba8 color_type;

        //--------------------------------------------------------------------
        span_pattern_filter_rgba32_nn(alloc_type& alloc) : 
            base_type(alloc),
            m_remainder_x(1),
            m_remainder_y(1)
        {}

        //--------------------------------------------------------------------
        span_pattern_filter_rgba32_nn(alloc_type& alloc,
                                      const rendering_buffer& src, 
                                      interpolator_type& inter) :
            base_type(alloc, src, color_type(0,0,0,0), inter, 0),
            m_remainder_x(src.width()),
            m_remainder_y(src.height())
        {}

        //--------------------------------------------------------------------
        void source_image(const rendering_buffer& src) 
        { 
            base_type::source_image(src);
            m_remainder_x = RemainderX(src.width());
            m_remainder_y = RemainderX(src.height());
        }

        //--------------------------------------------------------------------
        color_type* generate(int x, int y, unsigned len)
        {
            base_type::interpolator().begin(x + base_type::filter_dx_dbl(), 
                                            y + base_type::filter_dy_dbl(), len);
            const unsigned char *fg_ptr;
            color_type* span = base_type::allocator().span();
            do
            {
                base_type::interpolator().coordinates(&x, &y);

                x = m_remainder_x(x >> image_subpixel_shift);
                y = m_remainder_y(y >> image_subpixel_shift);

                fg_ptr = base_type::source_image().row(y) + (x << 2);
                span->r = fg_ptr[Order::R];
                span->g = fg_ptr[Order::G];
                span->b = fg_ptr[Order::B];
                span->a = fg_ptr[Order::A];
                ++span;
                ++base_type::interpolator();

            } while(--len);

            return base_type::allocator().span();
        }

    private:
        RemainderX m_remainder_x;
        RemainderY m_remainder_y;
    };










    //=====================================span_pattern_filter_rgba32_bilinear
    template<class Order, 
             class Interpolator,
             class RemainderX,
             class RemainderY,
             class Allocator = span_allocator<rgba8> > 
    class span_pattern_filter_rgba32_bilinear : 
    public span_image_filter<rgba8, Interpolator, Allocator>
    {
    public:
        typedef Interpolator interpolator_type;
        typedef Allocator alloc_type;
        typedef span_image_filter<rgba8, Interpolator, alloc_type> base_type;
        typedef rgba8 color_type;

        //--------------------------------------------------------------------
        span_pattern_filter_rgba32_bilinear(alloc_type& alloc) : 
            base_type(alloc),
            m_remainder_x(1),
            m_remainder_y(1)
        {}

        //--------------------------------------------------------------------
        span_pattern_filter_rgba32_bilinear(alloc_type& alloc,
                                            const rendering_buffer& src, 
                                            interpolator_type& inter) :
            base_type(alloc, src, color_type(0,0,0,0), inter, 0),
            m_remainder_x(src.width()),
            m_remainder_y(src.height())
        {}

        //--------------------------------------------------------------------
        void source_image(const rendering_buffer& src) 
        { 
            base_type::source_image(src);
            m_remainder_x = RemainderX(src.width());
            m_remainder_y = RemainderX(src.height());
        }

        //--------------------------------------------------------------------
        color_type* generate(int x, int y, unsigned len)
        {
            base_type::interpolator().begin(x + base_type::filter_dx_dbl(), 
                                            y + base_type::filter_dy_dbl(), len);
            int fg[4];
            const int8u *fg_ptr;
            color_type* span = base_type::allocator().span();

            do
            {
                int x_hr;
                int y_hr;

                base_type::interpolator().coordinates(&x_hr, &y_hr);

                x_hr -= base_type::filter_dx_int();
                y_hr -= base_type::filter_dy_int();

                int x_lr = x_hr >> image_subpixel_shift;
                int y_lr = y_hr >> image_subpixel_shift;

                unsigned x1 = m_remainder_x(x_lr);
                unsigned x2 = x1 + 1;
                if(x2 >= base_type::source_image().width()) x2 = 0;
                x1 <<= 2;
                x2 <<= 2;

                unsigned y1 = m_remainder_y(y_lr);
                unsigned y2 = y1 + 1;
                if(y2 >= base_type::source_image().height()) y2 = 0;
                const int8u* ptr1 = base_type::source_image().row(y1);
                const int8u* ptr2 = base_type::source_image().row(y2);

                fg[0] = 
                fg[1] = 
                fg[2] = 
                fg[3] = image_subpixel_size * image_subpixel_size / 2;

                x_hr &= image_subpixel_mask;
                y_hr &= image_subpixel_mask;

                int weight;
                fg_ptr = ptr1 + x1;
                weight = (image_subpixel_size - x_hr) * 
                         (image_subpixel_size - y_hr);
                fg[0] += weight * fg_ptr[0];
                fg[1] += weight * fg_ptr[1];
                fg[2] += weight * fg_ptr[2];
                fg[3] += weight * fg_ptr[3];

                fg_ptr = ptr1 + x2;
                weight = x_hr * (image_subpixel_size - y_hr);
                fg[0] += weight * fg_ptr[0];
                fg[1] += weight * fg_ptr[1];
                fg[2] += weight * fg_ptr[2];
                fg[3] += weight * fg_ptr[3];

                fg_ptr = ptr2 + x1;
                weight = (image_subpixel_size - x_hr) * y_hr;
                fg[0] += weight * fg_ptr[0];
                fg[1] += weight * fg_ptr[1];
                fg[2] += weight * fg_ptr[2];
                fg[3] += weight * fg_ptr[3];

                fg_ptr = ptr2 + x2;
                weight = x_hr * y_hr;
                fg[0] += weight * fg_ptr[0];
                fg[1] += weight * fg_ptr[1];
                fg[2] += weight * fg_ptr[2];
                fg[3] += weight * fg_ptr[3];

                span->r = (int8u)(fg[Order::R] >> image_subpixel_shift * 2);
                span->g = (int8u)(fg[Order::G] >> image_subpixel_shift * 2);
                span->b = (int8u)(fg[Order::B] >> image_subpixel_shift * 2);
                span->a = (int8u)(fg[Order::A] >> image_subpixel_shift * 2);
                ++span;
                ++base_type::interpolator();

            } while(--len);

            return base_type::allocator().span();
        }
    private:
        RemainderX m_remainder_x;
        RemainderY m_remainder_y;
    };





    //==============================================span_pattern_filter_rgba32
    template<class Order, 
             class Interpolator,
             class RemainderX,
             class RemainderY,
             class Allocator = span_allocator<rgba8> > 
    class span_pattern_filter_rgba32 : 
    public span_image_filter<rgba8, Interpolator, Allocator>
    {
    public:
        typedef Interpolator interpolator_type;
        typedef Allocator alloc_type;
        typedef span_image_filter<rgba8, Interpolator, alloc_type> base_type;
        typedef rgba8 color_type;

        //--------------------------------------------------------------------
        span_pattern_filter_rgba32(alloc_type& alloc) : 
            base_type(alloc)
        {}

        //--------------------------------------------------------------------
        span_pattern_filter_rgba32(alloc_type& alloc,
                                   const rendering_buffer& src, 
                                   interpolator_type& inter,
                                   const image_filter_base& filter) :
            base_type(alloc, src, color_type(0,0,0,0), inter, &filter),
            m_remainder_x(src.width()),
            m_remainder_y(src.height())
        {}

        //--------------------------------------------------------------------
        void source_image(const rendering_buffer& src) 
        { 
            base_type::source_image(src);
            m_remainder_x = RemainderX(src.width());
            m_remainder_y = RemainderX(src.height());
        }

        //--------------------------------------------------------------------
        color_type* generate(int x, int y, unsigned len)
        {
            base_type::interpolator().begin(x + base_type::filter_dx_dbl(), 
                                            y + base_type::filter_dy_dbl(), len);
            int fg[4];

            const unsigned char *fg_ptr;

            unsigned   dimension    = base_type::filter().dimension();
            int        start        = base_type::filter().start();
            const int* weight_array = base_type::filter().weight_array_int();

            color_type* span = base_type::allocator().span();

            int x_count; 
            int weight_y;

            do
            {
                base_type::interpolator().coordinates(&x, &y);

                x -= base_type::filter_dx_int();
                y -= base_type::filter_dy_int();

                int x_hr = x; 
                int y_hr = y; 

                int x_fract = x_hr & image_subpixel_mask;
                unsigned y_count = dimension;

                int y_lr  = m_remainder_y((y >> image_subpixel_shift) + start);
                int x_int = m_remainder_x((x >> image_subpixel_shift) + start);
                int x_lr;

                y_hr = image_subpixel_mask - (y_hr & image_subpixel_mask);
                fg[0] = fg[1] = fg[2] = fg[3] = image_filter_size / 2;

                do
                {
                    x_count = dimension;
                    weight_y = weight_array[y_hr];
                    x_hr = image_subpixel_mask - x_fract;
                    x_lr = x_int;
                    fg_ptr = base_type::source_image().row(y_lr) + (x_lr << 2);

                    do
                    {
                        int weight = (weight_y * weight_array[x_hr] + 
                                     image_filter_size / 2) >> 
                                     image_filter_shift;
        
                        fg[0] += *fg_ptr++ * weight;
                        fg[1] += *fg_ptr++ * weight;
                        fg[2] += *fg_ptr++ * weight;
                        fg[3] += *fg_ptr++ * weight;

                        x_hr += image_subpixel_size;
                        ++x_lr;
                        if(x_lr >= int(base_type::source_image().width())) 
                        {
                            x_lr = 0;
                            fg_ptr = base_type::source_image().row(y_lr);
                        }

                    } while(--x_count);

                    y_hr += image_subpixel_size;
                    ++y_lr;
                    if(y_lr >= int(base_type::source_image().height())) y_lr = 0;

                } while(--y_count);

                fg[0] >>= image_filter_shift;
                fg[1] >>= image_filter_shift;
                fg[2] >>= image_filter_shift;
                fg[3] >>= image_filter_shift;

                if(fg[0] < 0) fg[0] = 0;
                if(fg[1] < 0) fg[1] = 0;
                if(fg[2] < 0) fg[2] = 0;
                if(fg[3] < 0) fg[3] = 0;

                if(fg[Order::A] > 255)          fg[Order::A] = 255;
                if(fg[Order::R] > fg[Order::A]) fg[Order::R] = fg[Order::A];
                if(fg[Order::G] > fg[Order::A]) fg[Order::G] = fg[Order::A];
                if(fg[Order::B] > fg[Order::A]) fg[Order::B] = fg[Order::A];

                span->r = fg[Order::R];
                span->g = fg[Order::G];
                span->b = fg[Order::B];
                span->a = fg[Order::A];
                ++span;
                ++base_type::interpolator();

            } while(--len);

            return base_type::allocator().span();
        }

    private:
        RemainderX m_remainder_x;
        RemainderY m_remainder_y;
    };


}


#endif



