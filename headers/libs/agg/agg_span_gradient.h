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

#ifndef AGG_SPAN_GRADIENT_INCLUDED
#define AGG_SPAN_GRADIENT_INCLUDED

#include <math.h>
#include <stdlib.h>
#include "agg_basics.h"
#include "agg_span_generator.h"
#include "agg_math.h"


namespace agg
{

    //==========================================================span_gradient
    template<class ColorT, 
             class Interpolator,
             class GradientF, 
             class ColorF,
             class Allocator = span_allocator<ColorT> >
    class span_gradient : public span_generator<ColorT, Allocator>
    {
    public:
        typedef Interpolator interpolator_type;
        typedef Allocator alloc_type;
        typedef ColorT color_type;
        typedef span_generator<color_type, alloc_type> base_type;

        enum
        {
            base_shift = 8,
            base_size  = 1 << base_shift,
            base_mask  = base_size - 1,

            gradient_shift = 4,
            gradient_size  = 1 << gradient_shift,
            gradient_mask  = gradient_size - 1,

            downscale_shift = interpolator_type::subpixel_shift - gradient_shift
        };


        //--------------------------------------------------------------------
        span_gradient(alloc_type& alloc) : base_type(alloc) {}

        //--------------------------------------------------------------------
        span_gradient(alloc_type& alloc,
                      interpolator_type& inter,
                      const GradientF& gradient_function,
                      ColorF color_function,
                      double d1, double d2) : 
            base_type(alloc),
            m_interpolator(&inter),
            m_gradient_function(&gradient_function),
            m_color_function(color_function),
            m_d1(int(d1 * gradient_size)),
            m_d2(int(d2 * gradient_size))
        {}

        //--------------------------------------------------------------------
        interpolator_type& interpolator() { return *m_interpolator; }
        const GradientF& gradient_function() const { return *m_gradient_function; }
        const ColorF color_function() const { return m_color_function; }
        double d1() const { return double(m_d1) / gradient_size; }
        double d2() const { return double(m_d2) / gradient_size; }

        //--------------------------------------------------------------------
        void interpolator(interpolator_type& i) { m_interpolator = &i; }
        void gradient_function(const GradientF& gf) { m_gradient_function = &gf; }
        void color_function(ColorF cf) { m_color_function = cf; }
        void d1(double v) { m_d1 = int(v * gradient_size); }
        void d2(double v) { m_d2 = int(v * gradient_size); }

        //--------------------------------------------------------------------
        color_type* generate(int x, int y, unsigned len)
        {   
            color_type* span = base_type::allocator().span();
            int dd = m_d2 - m_d1;
            if(dd < 1) dd = 1;
            m_interpolator->begin(x+0.5, y+0.5, len);
            do
            {
                m_interpolator->coordinates(&x, &y);
                int d = m_gradient_function->calculate(x >> downscale_shift, 
                                                       y >> downscale_shift, dd);
                d = ((d - m_d1) << base_shift) / dd;
                if(d < 0) d = 0;
                if(d > base_mask) d = base_mask;
                *span++ = m_color_function[d];
                ++(*m_interpolator);
            }
            while(--len);
            return base_type::allocator().span();
        }

    private:
        interpolator_type* m_interpolator;
        const GradientF*   m_gradient_function;
        ColorF             m_color_function;
        int                m_d1;
        int                m_d2;
    };




    //=====================================================gradient_linear_color
    template<class ColorT, unsigned BaseShift=8> 
    struct gradient_linear_color
    {
        typedef ColorT color_type;
        enum
        {
            base_shift = BaseShift,
            base_size  = 1 << base_shift,
            base_mask  = base_size - 1
        };

        gradient_linear_color() {}
        gradient_linear_color(const color_type& c1, const color_type& c2) :
            m_c1(c1), m_c2(c2) {}

        color_type operator [] (unsigned v) const 
        {
            return m_c1.gradient(m_c2, double(v) / double(base_mask));
        }

        void colors(const color_type& c1, const color_type& c2)
        {
            m_c1 = c1;
            m_c2 = c2;
        }

        color_type m_c1;
        color_type m_c2;
    };



    //---------------------------------------------------------gradient_circle
    class gradient_circle
    {
    public:
        static int calculate(int x, int y, int)
        {
            return int(fast_sqrt(x*x + y*y));
        }
    };


    //--------------------------------------------------------------gradient_x
    class gradient_x
    {
    public:
        static int calculate(int x, int, int) { return x; }
    };


    //--------------------------------------------------------------gradient_y
    class gradient_y
    {
    public:
        static int calculate(int, int y, int) { return y; }
    };


    //--------------------------------------------------------gradient_diamond
    class gradient_diamond
    {
    public:
        static int calculate(int x, int y, int) 
        { 
            int ax = abs(x);
            int ay = abs(y);
            return ax > ay ? ax : ay; 
        }
    };


    //-------------------------------------------------------------gradient_xy
    class gradient_xy
    {
    public:
        static int calculate(int x, int y, int d) 
        { 
            return abs(x) * abs(y) / d; 
        }
    };


    //--------------------------------------------------------gradient_sqrt_xy
    class gradient_sqrt_xy
    {
    public:
        static int calculate(int x, int y, int) 
        { 
            return fast_sqrt(abs(x) * abs(y)); 
        }
    };


    //----------------------------------------------------------gradient_conic
    class gradient_conic
    {
    public:
        static int calculate(int x, int y, int d) 
        { 
            return int(fabs(atan2(double(y), double(x))) * double(d) / pi);
        }
    };




}

#endif
