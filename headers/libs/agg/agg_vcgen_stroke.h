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

#ifndef AGG_VCGEN_STROKE_INCLUDED
#define AGG_VCGEN_STROKE_INCLUDED

#include "agg_basics.h"
#include "agg_vertex_sequence.h"
#include "agg_vertex_iterator.h"

namespace agg
{

    // Minimal angle to calculate round joins 
    const double vcgen_stroke_theta = 1e-10; //----vcgen_stroke_theta

    //============================================================vcgen_stroke
    //
    // See Implementation agg_vcgen_stroke.cpp
    // Stroke generator
    //
    //------------------------------------------------------------------------
    class vcgen_stroke
    {
        enum status_e
        {
            initial,
            ready,
            cap1,
            cap2,
            outline1,
            close_first,
            outline2,
            out_vertices,
            end_poly1,
            end_poly2,
            stop
        };

    public:
        enum line_cap_e
        {
            butt_cap,
            square_cap,
            round_cap
        };

        enum line_join_e
        {
            miter_join,
            miter_join_revert,
            round_join,
            bevel_join
        };

        struct coord_type
        {
            double x, y;

            coord_type() {}
            coord_type(double x_, double y_) : x(x_), y(y_) {}
        };


        typedef vertex_sequence<vertex_dist, 6> vertex_storage;
        typedef pod_deque<coord_type, 6>        coord_storage;

        vcgen_stroke();

        void line_cap(line_cap_e lc) { m_line_cap = lc; }
        void line_join(line_join_e lj) { m_line_join = lj; }

        line_cap_e line_cap() const { return m_line_cap; }
        line_join_e line_join() const { return m_line_join; }

        void width(double w) { m_width = w * 0.5; }
        void miter_limit(double ml) { m_miter_limit = ml; }
        void miter_limit_theta(double t);
        void approximation_scale(double as) { m_approx_scale = as; }

        double width() const { return m_width * 2.0; }
        double miter_limit() const { return m_miter_limit; }
        double approximation_scale() const { return m_approx_scale; }

        void shorten(double s) { m_shorten = s; }
        double shorten() const { return m_shorten; }

        // Vertex Generator Interface
        void remove_all();
        void add_vertex(double x, double y, unsigned cmd);

        // Vertex Source Interface
        void     rewind(unsigned id);
        unsigned vertex(double* x, double* y);

        typedef vcgen_stroke source_type;
        typedef vertex_iterator<source_type> iterator;
        iterator begin(unsigned id) { return iterator(*this, id); }
        iterator end() { return iterator(path_cmd_stop); }

    private:
        vcgen_stroke(const vcgen_stroke&);
        const vcgen_stroke& operator = (const vcgen_stroke&);

        void calc_join(const vertex_dist& v0, 
                       const vertex_dist& v1, 
                       const vertex_dist& v2,
                       double len1, double len2);

        void calc_miter(const vertex_dist& v0, 
                        const vertex_dist& v1, 
                        const vertex_dist& v2,
                        double dx1, double dy1, 
                        double dx2, double dy2,
                        bool revert_flag);

        void calc_arc(double x,   double y, 
                      double dx1, double dy1, 
                      double dx2, double dy2);

        void calc_cap(const vertex_dist& v0, 
                      const vertex_dist& v1, 
                      double len);

        vertex_storage m_src_vertices;
        coord_storage  m_out_vertices;
        double         m_width;
        double         m_miter_limit;
        double         m_approx_scale;
        double         m_shorten;
        line_cap_e     m_line_cap;
        line_join_e    m_line_join;
        unsigned       m_closed;
        status_e       m_status;
        status_e       m_prev_status;
        unsigned       m_src_vertex;
        unsigned       m_out_vertex;
    };


}

#endif
