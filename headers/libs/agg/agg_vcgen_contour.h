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

#ifndef AGG_VCGEN_CONTOUR_INCLUDED
#define AGG_VCGEN_CONTOUR_INCLUDED

#include "agg_basics.h"
#include "agg_vertex_sequence.h"

namespace agg
{

    //----------------------------------------------------------vcgen_contour
    //
    // See Implementation agg_vcgen_contour.cpp
    //
    class vcgen_contour
    {
        enum status_e
        {
            initial,
            ready,
            outline,
            add_point,
            end_poly
        };

    public:
        typedef vertex_sequence<vertex_dist, 6> vertex_storage;

        vcgen_contour();

        void width(double w) { m_width = w * 0.5; }
        void miter_limit(double ml) { m_miter_limit = ml; }
        void miter_limit_theta(double t);
        void auto_detect_orientation(bool v) { m_auto_detect = v; }

        double width() const { return m_width * 2.0; }
        double miter_limit() const { return m_miter_limit; }
        bool   auto_detect_orientation() const { return m_auto_detect; }

        // Generator interface
        void remove_all();
        void add_vertex(double x, double y, unsigned cmd);

        // Vertex Source Interface
        void     rewind(unsigned id);
        unsigned vertex(double* x, double* y);

    private:
        vcgen_contour(const vcgen_contour&);
        const vcgen_contour& operator = (const vcgen_contour&);

        bool calc_miter(const vertex_dist& v0, 
                        const vertex_dist& v1, 
                        const vertex_dist& v2);

        vertex_storage m_src_vertices;
        double         m_width;
        double         m_abs_width;
        double         m_signed_width;
        double         m_miter_limit;
        status_e       m_status;
        unsigned       m_src_vertex;
        unsigned       m_closed;
        unsigned       m_orientation;
        bool           m_auto_detect;
        double         m_x1;
        double         m_y1;
        double         m_x2;
        double         m_y2;
    };

}

#endif
