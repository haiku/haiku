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


#ifndef AGG_SPAN_PATTERN_INCLUDED
#define AGG_SPAN_PATTERN_INCLUDED

#include "agg_basics.h"
#include "agg_rendering_buffer.h"
#include "agg_span_generator.h"


namespace agg
{

    //--------------------------------------------------------span_pattern
    template<class ColorT, class AlphaT, class Allocator> 
    class span_pattern : public span_generator<ColorT, Allocator>
    {
    public:
        typedef ColorT color_type;
        typedef AlphaT alpha_type;
        typedef Allocator alloc_type;

        //----------------------------------------------------------------
        span_pattern(alloc_type& alloc) : 
            span_generator<color_type, alloc_type>(alloc) 
        {}

        //----------------------------------------------------------------
        span_pattern(alloc_type& alloc,
                     const rendering_buffer& src, 
                     unsigned offset_x, unsigned offset_y, 
                     alpha_type alpha) :
            span_generator<color_type, alloc_type>(alloc),
            m_src(&src),
            m_offset_x(offset_x),
            m_offset_y(offset_y),
            m_alpha(alpha)
        {}

        //----------------------------------------------------------------
        const rendering_buffer& source_image() const { return *m_src; }
        unsigned offset_x()                    const { return m_offset_x; }
        unsigned offset_y()                    const { return m_offset_y; }
        alpha_type alpha()                     const { return m_alpha; }

        //----------------------------------------------------------------
        void source_image(const rendering_buffer& v)  { m_src = &v; }
        void offset_x(unsigned v) { m_offset_x = v; }
        void offset_y(unsigned v) { m_offset_y = v; }
        void alpha(alpha_type v)  { m_alpha = v; }

        //----------------------------------------------------------------
    private:
        const rendering_buffer* m_src;
        unsigned m_offset_x;
        unsigned m_offset_y;
        alpha_type m_alpha;
    };


}

#endif

