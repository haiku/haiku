//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
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
// Stroke math
//
//----------------------------------------------------------------------------

#ifndef AGG_STROKE_MATH_INCLUDED
#define AGG_STROKE_MATH_INCLUDED

#include "agg_math.h"
#include "agg_vertex_sequence.h"

namespace agg
{
    //-------------------------------------------------------------line_cap_e
    enum line_cap_e
    {
        butt_cap,
        square_cap,
        round_cap
    };

    //------------------------------------------------------------line_join_e
    enum line_join_e
    {
        miter_join         = 0,
        miter_join_revert  = 1,
        round_join         = 2,
        bevel_join         = 3,
        miter_join_round   = 4
    };


    //-----------------------------------------------------------inner_join_e
    enum inner_join_e
    {
        inner_bevel,
        inner_miter,
        inner_jag,
        inner_round
    };

    //------------------------------------------------------------math_stroke
    template<class VertexConsumer> class math_stroke
    {
    public:
        typedef typename VertexConsumer::value_type coord_type;

        math_stroke();

        void line_cap(line_cap_e lc)     { m_line_cap = lc; }
        void line_join(line_join_e lj)   { m_line_join = lj; }
        void inner_join(inner_join_e ij) { m_inner_join = ij; }

        line_cap_e   line_cap()   const { return m_line_cap; }
        line_join_e  line_join()  const { return m_line_join; }
        inner_join_e inner_join() const { return m_inner_join; }

        void width(double w);
        void miter_limit(double ml) { m_miter_limit = ml; }
        void miter_limit_theta(double t);
        void inner_miter_limit(double ml) { m_inner_miter_limit = ml; }
        void approximation_scale(double as) { m_approx_scale = as; }

        double width() const { return m_width * 2.0; }
        double miter_limit() const { return m_miter_limit; }
        double inner_miter_limit() const { return m_inner_miter_limit; }
        double approximation_scale() const { return m_approx_scale; }

        void calc_cap(VertexConsumer& out_vertices,
                      const vertex_dist& v0,
                      const vertex_dist& v1,
                      double len);

        void calc_join(VertexConsumer& out_vertices,
                       const vertex_dist& v0,
                       const vertex_dist& v1,
                       const vertex_dist& v2,
                       double len1,
                       double len2);

    private:
        void calc_arc(VertexConsumer& out_vertices,
                      double x,   double y,
                      double dx1, double dy1,
                      double dx2, double dy2);

        void calc_miter(VertexConsumer& out_vertices,
                        const vertex_dist& v0,
                        const vertex_dist& v1,
                        const vertex_dist& v2,
                        double dx1, double dy1,
                        double dx2, double dy2,
                        line_join_e lj,
                        double ml);

        double       m_width;
        double       m_width_abs;
        int          m_width_sign;
        double       m_miter_limit;
        double       m_inner_miter_limit;
        double       m_approx_scale;
        line_cap_e   m_line_cap;
        line_join_e  m_line_join;
        inner_join_e m_inner_join;
    };

    //-----------------------------------------------------------------------
    template<class VC> math_stroke<VC>::math_stroke() :
        m_width(0.5),
        m_width_abs(0.5),
        m_width_sign(1),
        m_miter_limit(4.0),
        m_inner_miter_limit(1.01),
        m_approx_scale(1.0),
        m_line_cap(butt_cap),
        m_line_join(miter_join),
        m_inner_join(inner_miter)
    {
    }

    //-----------------------------------------------------------------------
    template<class VC> void math_stroke<VC>::width(double w)
    {
        m_width = w * 0.5;
        if(m_width < 0)
        {
            m_width_abs  = -m_width;
            m_width_sign = -1;
        }
        else
        {
            m_width_abs  = m_width;
            m_width_sign = 1;
        }
    }

    //-----------------------------------------------------------------------
    template<class VC> void math_stroke<VC>::miter_limit_theta(double t)
    {
        m_miter_limit = 1.0 / sin(t * 0.5) ;
    }

    //-----------------------------------------------------------------------
    template<class VC>
    void math_stroke<VC>::calc_arc(VC& out_vertices,
                                   double x,   double y,
                                   double dx1, double dy1,
                                   double dx2, double dy2)
    {
        double a1 = atan2(dy1 * m_width_sign, dx1 * m_width_sign);
        double a2 = atan2(dy2 * m_width_sign, dx2 * m_width_sign);
        double da = acos(m_width_abs / (m_width_abs + 0.125 / m_approx_scale)) * 2;
        int i, n;


        out_vertices.add(coord_type(x + dx1, y + dy1));
        if(m_width_sign > 0)
        {
            if(a1 > a2) a2 += 2 * pi;
            n = int((a2 - a1) / da);
            da = (a2 - a1) / (n + 1);
            a1 += da;
            for(i = 0; i < n; i++)
            {
                out_vertices.add(coord_type(x + cos(a1) * m_width,
                                            y + sin(a1) * m_width));
                a1 += da;
            }
        }
        else
        {
            if(a1 < a2) a2 -= 2 * pi;
            n = int((a1 - a2) / da);
            da = (a1 - a2) / (n + 1);
            a1 -= da;
            for(i = 0; i < n; i++)
            {
                out_vertices.add(coord_type(x + cos(a1) * m_width,
                                            y + sin(a1) * m_width));
                a1 -= da;
            }
        }
        out_vertices.add(coord_type(x + dx2, y + dy2));
    }

    //-----------------------------------------------------------------------
    template<class VC>
    void math_stroke<VC>::calc_miter(VC& out_vertices,
                                     const vertex_dist& v0,
                                     const vertex_dist& v1,
                                     const vertex_dist& v2,
                                     double dx1, double dy1,
                                     double dx2, double dy2,
                                     line_join_e lj,
                                     double ml)
    {
        double xi = v1.x;
        double yi = v1.y;
        bool miter_limit_exceeded = true; // Assume the worst

        if(calc_intersection(v0.x + dx1, v0.y - dy1,
                             v1.x + dx1, v1.y - dy1,
                             v1.x + dx2, v1.y - dy2,
                             v2.x + dx2, v2.y - dy2,
                             &xi, &yi))
        {
            // Calculation of the intersection succeeded
            //---------------------
            double d1 = calc_distance(v1.x, v1.y, xi, yi);
            double lim = m_width_abs * ml;
            if(d1 <= lim)
            {
                // Inside the miter limit
                //---------------------
                out_vertices.add(coord_type(xi, yi));
                miter_limit_exceeded = false;
            }
        }
        else
        {
            // Calculation of the intersection failed, most probably
            // the three points lie one straight line.
            // First check if v0 and v2 lie on the opposite sides of vector:
            // (v1.x, v1.y) -> (v1.x+dx1, v1.y-dy1), that is, the perpendicular
            // to the line determined by vertices v0 and v1.
            // This condition determines whether the next line segments continues
            // the previous one or goes back.
            //----------------
            double x2 = v1.x + dx1;
            double y2 = v1.y - dy1;
            if(((x2 - v0.x)*dy1 - (v0.y - y2)*dx1 < 0.0) !=
               ((x2 - v2.x)*dy1 - (v2.y - y2)*dx1 < 0.0))
            {
                // This case means that the next segment continues
                // the previous one (straight line)
                //-----------------
                out_vertices.add(coord_type(v1.x + dx1, v1.y - dy1));
                miter_limit_exceeded = false;
            }
        }

        if(miter_limit_exceeded)
        {
            // Miter limit exceeded
            //------------------------
            switch(lj)
            {
            case miter_join_revert:
                // For the compatibility with SVG, PDF, etc,
                // we use a simple bevel join instead of
                // "smart" bevel
                //-------------------
                out_vertices.add(coord_type(v1.x + dx1, v1.y - dy1));
                out_vertices.add(coord_type(v1.x + dx2, v1.y - dy2));
                break;

            case miter_join_round:
                calc_arc(out_vertices, v1.x, v1.y, dx1, -dy1, dx2, -dy2);
                break;

            default:
                // If no miter-revert, calculate new dx1, dy1, dx2, dy2
                //----------------
                ml *= m_width_sign;
                out_vertices.add(coord_type(v1.x + dx1 + dy1 * ml,
                                            v1.y - dy1 + dx1 * ml));
                out_vertices.add(coord_type(v1.x + dx2 - dy2 * ml,
                                            v1.y - dy2 - dx2 * ml));
                break;
            }
        }
    }

    //--------------------------------------------------------stroke_calc_cap
    template<class VC>
    void math_stroke<VC>::calc_cap(VC& out_vertices,
                                   const vertex_dist& v0,
                                   const vertex_dist& v1,
                                   double len)
    {
        out_vertices.remove_all();

        double dx1 = (v1.y - v0.y) / len;
        double dy1 = (v1.x - v0.x) / len;
        double dx2 = 0;
        double dy2 = 0;

        dx1 *= m_width;
        dy1 *= m_width;

        if(m_line_cap != round_cap)
        {
            if(m_line_cap == square_cap)
            {
                dx2 = dy1 * m_width_sign;
                dy2 = dx1 * m_width_sign;
            }
            out_vertices.add(coord_type(v0.x - dx1 - dx2, v0.y + dy1 - dy2));
            out_vertices.add(coord_type(v0.x + dx1 - dx2, v0.y - dy1 - dy2));
        }
        else
        {
            double da = acos(m_width_abs / (m_width_abs + 0.125 / m_approx_scale)) * 2;
            double a1;
            int i;
            int n = int(pi / da);

            da = pi / (n + 1);
            out_vertices.add(coord_type(v0.x - dx1, v0.y + dy1));
            if(m_width_sign > 0)
            {
                a1 = atan2(dy1, -dx1);
                a1 += da;
                for(i = 0; i < n; i++)
                {
                    out_vertices.add(coord_type(v0.x + cos(a1) * m_width,
                                                v0.y + sin(a1) * m_width));
                    a1 += da;
                }
            }
            else
            {
                a1 = atan2(-dy1, dx1);
                a1 -= da;
                for(i = 0; i < n; i++)
                {
                    out_vertices.add(coord_type(v0.x + cos(a1) * m_width,
                                                v0.y + sin(a1) * m_width));
                    a1 -= da;
                }
            }
            out_vertices.add(coord_type(v0.x + dx1, v0.y - dy1));
        }
    }

    //-----------------------------------------------------------------------
    template<class VC>
    void math_stroke<VC>::calc_join(VC& out_vertices,
                                    const vertex_dist& v0,
                                    const vertex_dist& v1,
                                    const vertex_dist& v2,
                                    double len1,
                                    double len2)
    {
        double dx1, dy1, dx2, dy2;
        double d;

        dx1 = m_width * (v1.y - v0.y) / len1;
        dy1 = m_width * (v1.x - v0.x) / len1;

        dx2 = m_width * (v2.y - v1.y) / len2;
        dy2 = m_width * (v2.x - v1.x) / len2;

        out_vertices.remove_all();

        double cp = cross_product(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);
        if(cp != 0 && (cp > 0) == (m_width > 0))
        {
            // Inner join
            //---------------
            double limit = ((len1 < len2) ? len1 : len2) / m_width_abs;
            if(limit < m_inner_miter_limit)
            {
                limit = m_inner_miter_limit;
            }

            switch(m_inner_join)
            {
            default: // inner_bevel
                out_vertices.add(coord_type(v1.x + dx1, v1.y - dy1));
                out_vertices.add(coord_type(v1.x + dx2, v1.y - dy2));
                break;

            case inner_miter:
                calc_miter(out_vertices,
                           v0, v1, v2, dx1, dy1, dx2, dy2,
                           miter_join_revert,
                           limit);
                break;

            case inner_jag:
            case inner_round:
                {
                    d = (dx1-dx2) * (dx1-dx2) + (dy1-dy2) * (dy1-dy2);
                    if(d < len1 * len1 && d < len2 * len2)
                    {
                        calc_miter(out_vertices,
                                   v0, v1, v2, dx1, dy1, dx2, dy2,
                                   miter_join_revert,
                                   limit);
                    }
                    else
                    {
                        if(m_inner_join == inner_jag)
                        {
                            out_vertices.add(coord_type(v1.x + dx1, v1.y - dy1));
                            out_vertices.add(coord_type(v1.x,       v1.y      ));
                            out_vertices.add(coord_type(v1.x + dx2, v1.y - dy2));
                        }
                        else
                        {
                            out_vertices.add(coord_type(v1.x + dx1, v1.y - dy1));
                            out_vertices.add(coord_type(v1.x,       v1.y      ));
                            calc_arc(out_vertices, v1.x, v1.y, dx2, -dy2, dx1, -dy1);
                            out_vertices.add(coord_type(v1.x,       v1.y      ));
                            out_vertices.add(coord_type(v1.x + dx2, v1.y - dy2));
                        }
                    }
                }
                break;
            }
        }
        else
        {
            // Outer join
            //---------------
            line_join_e lj = m_line_join;
            if(m_line_join == round_join || m_line_join == bevel_join)
            {
                // This is an optimization that reduces the number of points
                // in cases of almost collonear segments. If there's no
                // visible difference between bevel and miter joins we'd rather
                // use miter join because it adds only one point instead of two.
                //
                // Here we calculate the middle point between the bevel points
                // and then, the distance between v1 and this middle point.
                // At outer joins this distance always less than stroke width,
                // because it's actually the height of an isosceles triangle of
                // v1 and its two bevel points. If the difference between this
                // width and this value is small (no visible bevel) we can switch
                // to the miter join.
                //
                // The constant in the expression makes the result approximately
                // the same as in round joins and caps. One can safely comment
                // out this "if".
                //-------------------
                double dx = (dx1 + dx2) / 2;
                double dy = (dy1 + dy2) / 2;
                d = m_width_abs - sqrt(dx * dx + dy * dy);
                if(d < 0.0625 / m_approx_scale)
                {
                    lj = miter_join;
                }
            }

            switch(lj)
            {
            case miter_join:
            case miter_join_revert:
            case miter_join_round:
                calc_miter(out_vertices,
                           v0, v1, v2, dx1, dy1, dx2, dy2,
                           lj,
                           m_miter_limit);
                break;

            case round_join:
                calc_arc(out_vertices, v1.x, v1.y, dx1, -dy1, dx2, -dy2);
                break;

            default: // Bevel join
                out_vertices.add(coord_type(v1.x + dx1, v1.y - dy1));
                out_vertices.add(coord_type(v1.x + dx2, v1.y - dy2));
                break;
            }
        }
    }




}

#endif
