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
// Image transformations with filtering. Span generator base class
//
//----------------------------------------------------------------------------
#ifndef AGG_SPAN_IMAGE_FILTER_INCLUDED
#define AGG_SPAN_IMAGE_FILTER_INCLUDED

#include "agg_basics.h"
#include "agg_image_filters.h"
#include "agg_rendering_buffer.h"
#include "agg_span_generator.h"


namespace agg
{

    //--------------------------------------------------span_image_filter
    template<class ColorT, class Interpolator, class Allocator> 
    class span_image_filter : public span_generator<ColorT, Allocator>
    {
    public:
        typedef ColorT color_type;
        typedef Allocator alloc_type;
        typedef Interpolator interpolator_type;

        //----------------------------------------------------------------
        span_image_filter(alloc_type& alloc) : 
            span_generator<color_type, alloc_type>(alloc) 
        {}

        //----------------------------------------------------------------
        span_image_filter(alloc_type& alloc,
                          const rendering_buffer& src, 
                          const color_type& back_color,
                          interpolator_type& interpolator,
                          const image_filter_base* filter) : 
            span_generator<color_type, alloc_type>(alloc),
            m_src(&src),
            m_back_color(back_color),
            m_interpolator(&interpolator),
            m_filter(filter),
            m_dx_dbl(0.5),
            m_dy_dbl(0.5),
            m_dx_int(image_subpixel_size / 2),
            m_dy_int(image_subpixel_size / 2)
        {}

        //----------------------------------------------------------------
        const  rendering_buffer& source_image() const { return *m_src; }
        const  color_type& background_color()   const { return m_back_color; }
        const  image_filter_base& filter()      const { return *m_filter; }
        int    filter_dx_int()                  const { return m_dx_int; }
        int    filter_dy_int()                  const { return m_dy_int; }
        double filter_dx_dbl()                  const { return m_dx_dbl; }
        double filter_dy_dbl()                  const { return m_dy_dbl; }

        //----------------------------------------------------------------
        void source_image(const rendering_buffer& v) { m_src = &v; }
        void background_color(const color_type& v)   { m_back_color = v; }
        void interpolator(interpolator_type& v)      { m_interpolator = &v; }
        void filter(const image_filter_base& v)      { m_filter = &v; }
        void filter_offset(double dx, double dy)
        {
            m_dx_dbl = dx;
            m_dy_dbl = dy;
            m_dx_int = int(dx * image_subpixel_size);
            m_dy_int = int(dy * image_subpixel_size);
        }
        void filter_offset(double d) { filter_offset(d, d); }

        //----------------------------------------------------------------
        interpolator_type& interpolator() { return *m_interpolator; }

        //----------------------------------------------------------------
    private:
        const rendering_buffer*  m_src;
        color_type               m_back_color;
        interpolator_type*       m_interpolator;
        const image_filter_base* m_filter;
        double   m_dx_dbl;
        double   m_dy_dbl;
        unsigned m_dx_int;
        unsigned m_dy_int;
    };



    //-------------------------------------------------remainder_unsigned
    class remainder_unsigned
    {
    public:
        remainder_unsigned(unsigned size) : 
            m_size(size), 
            m_add(size * (0x3FFFFFFF / size)) 
        {}

        unsigned operator () (int v) const 
        { 
            return (unsigned(v) + m_add) % m_size; 
        }
    private:
        unsigned m_size;
        unsigned m_add;
    };


    //----------------------------------------------------remainder_pow2
    class remainder_pow2
    {
    public:
        remainder_pow2(unsigned size) 
        {
            m_mask = 1;
            while(m_mask < size) m_mask = (m_mask << 1) | 1;
            m_mask >>= 1;
        }
        unsigned operator () (int v) const { return unsigned(v) & m_mask; }
    private:
        unsigned m_mask;
    };


    //-----------------------------------------------remainder_auto_pow2
    class remainder_auto_pow2
    {
    public:
        remainder_auto_pow2(unsigned size) :
            m_size(size),
            m_add(size * (0x3FFFFFFF / size)),
            m_mask((m_size & (m_size-1)) ? 0 : m_size-1)
        {}

        unsigned operator () (int v) const 
        { 
            if(m_mask) return unsigned(v) & m_mask;
            return (unsigned(v) + m_add) % m_size;
        }

    private:
        unsigned m_size;
        unsigned m_add;
        unsigned m_mask;
    };


}

#endif
