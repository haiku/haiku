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
            m_filter(filter)
        {}

        //----------------------------------------------------------------
        const rendering_buffer& source_image()  const { return *m_src; }
        const color_type& background_color()    const { return m_back_color; }
        const image_filter_base& filter()       const { return *m_filter; }

        //----------------------------------------------------------------
        void source_image(const rendering_buffer& v) { m_src = &v; }
        void background_color(const color_type& v)   { m_back_color = v; }
        void interpolator(interpolator_type& v)      { m_interpolator = &v; }
        void filter(const image_filter_base& v)      { m_filter = &v; }

        //----------------------------------------------------------------
        interpolator_type& interpolator() { return *m_interpolator; }

        //----------------------------------------------------------------
    private:
        const rendering_buffer*  m_src;
        color_type               m_back_color;
        interpolator_type*       m_interpolator;
        const image_filter_base* m_filter;
    };


}

#endif
