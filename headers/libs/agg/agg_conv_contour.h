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
// conv_stroke
//
//----------------------------------------------------------------------------
#ifndef AGG_CONV_CONTOUR_INCLUDED
#define AGG_CONV_CONTOUR_INCLUDED

#include "agg_basics.h"
#include "agg_vcgen_contour.h"
#include "agg_conv_adaptor_vcgen.h"

namespace agg
{

    //-----------------------------------------------------------conv_contour
    template<class VertexSource> 
    struct conv_contour : public conv_adaptor_vcgen<VertexSource, vcgen_contour>
    {
        typedef conv_adaptor_vcgen<VertexSource, vcgen_contour> base_type;

        conv_contour(VertexSource& vs) : 
            conv_adaptor_vcgen<VertexSource, vcgen_contour>(vs)
        {
        }

        void width(double w) { base_type::generator().width(w); }
        void miter_limit(double ml) { base_type::generator().miter_limit(ml); }
        void miter_limit_theta(double t) { base_type::generator().miter_limit_theta(t); }
        void auto_detect_orientation(bool v) { base_type::generator().auto_detect_orientation(v); }

        double width() const { return base_type::generator().width(); }
        double miter_limit() const { return base_type::generator().miter_limit(); }
        bool auto_detect_orientation() const { return base_type::generator().auto_detect_orientation(); }

    private:
        conv_contour(const conv_contour<VertexSource>&);
        const conv_contour<VertexSource>& 
            operator = (const conv_contour<VertexSource>&);
    };

}

#endif
