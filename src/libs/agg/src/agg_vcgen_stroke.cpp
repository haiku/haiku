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
// Stroke generator
//
//----------------------------------------------------------------------------
#include <math.h>
#include "agg_vcgen_stroke.h"
#include "agg_shorten_path.h"

namespace agg
{

    //------------------------------------------------------------------------
    vcgen_stroke::vcgen_stroke() :
        m_src_vertices(),
        m_out_vertices(),
        m_width(0.5),
        m_miter_limit(4.0),
        m_approx_scale(1.0),
        m_shorten(0.0),
        m_line_cap(butt_cap),
        m_line_join(miter_join),
        m_closed(0),
        m_status(initial),
        m_src_vertex(0),
        m_out_vertex(0)
    {
    }


    //------------------------------------------------------------------------
    void vcgen_stroke::miter_limit_theta(double t)
    { 
        m_miter_limit = 1.0 / sin(t * 0.5) ;
    }


    //------------------------------------------------------------------------
    void vcgen_stroke::remove_all()
    {
        m_src_vertices.remove_all();
        m_closed = 0;
        m_status = initial;
    }


    //------------------------------------------------------------------------
    void vcgen_stroke::add_vertex(double x, double y, unsigned cmd)
    {
        m_status = initial;
        if(is_move_to(cmd))
        {
            m_src_vertices.modify_last(vertex_dist(x, y));
        }
        else
        {
            if(is_vertex(cmd))
            {
                m_src_vertices.add(vertex_dist(x, y));
            }
            else
            {
                m_closed = get_close_flag(cmd);
            }
        }
    }


    //------------------------------------------------------------------------
    void vcgen_stroke::rewind(unsigned)
    {
        if(m_status == initial)
        {
            m_src_vertices.close(m_closed != 0);
            shorten_path(m_src_vertices, m_shorten, m_closed);
        }
        m_status = ready;
        m_src_vertex = 0;
        m_out_vertex = 0;
    }


    //------------------------------------------------------------------------
    unsigned vcgen_stroke::vertex(double* x, double* y)
    {
        unsigned cmd = path_cmd_line_to;
        while(!is_stop(cmd))
        {
            switch(m_status)
            {
            case initial:
                rewind(0);

            case ready:
                if(m_src_vertices.size() <  2 + unsigned(m_closed != 0))
                {
                    cmd = path_cmd_stop;
                    break;
                }
                m_status = m_closed ? outline1 : cap1;
                cmd = path_cmd_move_to;
                m_src_vertex = 0;
                m_out_vertex = 0;
                break;

            case cap1:
                calc_cap(m_src_vertices[0], 
                         m_src_vertices[1], 
                         m_src_vertices[0].dist);
                m_src_vertex = 1;
                m_prev_status = outline1;
                m_status = out_vertices;
                m_out_vertex = 0;
                break;

            case cap2:
                calc_cap(m_src_vertices[m_src_vertices.size() - 1], 
                         m_src_vertices[m_src_vertices.size() - 2], 
                         m_src_vertices[m_src_vertices.size() - 2].dist);
                m_prev_status = outline2;
                m_status = out_vertices;
                m_out_vertex = 0;
                break;

            case outline1:
                if(m_closed)
                {
                    if(m_src_vertex >= m_src_vertices.size())
                    {
                        m_prev_status = close_first;
                        m_status = end_poly1;
                        break;
                    }
                }
                else
                {
                    if(m_src_vertex >= m_src_vertices.size() - 1)
                    {
                        m_status = cap2;
                        break;
                    }
                }

                calc_join(m_src_vertices.prev(m_src_vertex), 
                          m_src_vertices.curr(m_src_vertex), 
                          m_src_vertices.next(m_src_vertex), 
                          m_src_vertices.prev(m_src_vertex).dist,
                          m_src_vertices.curr(m_src_vertex).dist);
                ++m_src_vertex;
                m_prev_status = m_status;
                m_status = out_vertices;
                m_out_vertex = 0;
                break;

            case close_first:
                m_status = outline2;
                cmd = path_cmd_move_to;

            case outline2:
                if(m_src_vertex <= unsigned(m_closed == 0))
                {
                    m_status = end_poly2;
                    m_prev_status = stop;
                    break;
                }

                --m_src_vertex;
                calc_join(m_src_vertices.next(m_src_vertex), 
                          m_src_vertices.curr(m_src_vertex), 
                          m_src_vertices.prev(m_src_vertex), 
                          m_src_vertices.curr(m_src_vertex).dist, 
                          m_src_vertices.prev(m_src_vertex).dist);

                m_prev_status = m_status;
                m_status = out_vertices;
                m_out_vertex = 0;
                break;

            case out_vertices:
                if(m_out_vertex >= m_out_vertices.size())
                {
                    m_status = m_prev_status;
                }
                else
                {
                    const coord_type& c = m_out_vertices[m_out_vertex++];
                    *x = c.x;
                    *y = c.y;
                    return cmd;
                }
                break;

            case end_poly1:
                m_status = m_prev_status;
                return path_cmd_end_poly | path_flags_close | path_flags_ccw;

            case end_poly2:
                m_status = m_prev_status;
                return path_cmd_end_poly | path_flags_close | path_flags_cw;

            case stop:
                cmd = path_cmd_stop;
                break;
            }
        }
        return cmd;
    }



    //------------------------------------------------------------------------
    void vcgen_stroke::calc_arc(double x,   double y, 
                                double dx1, double dy1, 
                                double dx2, double dy2)
    {
        double a1 = atan2(dy1, dx1);
        double a2 = atan2(dy2, dx2);
        double da = a1 - a2;

        if(fabs(da) < vcgen_stroke_theta)
        {
            m_out_vertices.add(coord_type(x + dx1, y + dy1));
            m_out_vertices.add(coord_type(x + dx2, y + dy2));
            return;
        }

        bool ccw = da > 0.0 && da < pi;

        da = fabs(1.0 / (m_width * m_approx_scale));
        if(!ccw)
        {
            if(a1 > a2) a2 += 2 * pi;
            while(a1 < a2)
            {
                m_out_vertices.add(coord_type(x + cos(a1) * m_width, y + sin(a1) * m_width));
                a1 += da;
            }
        }
        else
        {
            if(a1 < a2) a2 -= 2 * pi;
            while(a1 > a2)
            {
                m_out_vertices.add(coord_type(x + cos(a1) * m_width, y + sin(a1) * m_width));
                a1 -= da;
            }
        }
        m_out_vertices.add(coord_type(x + dx2, y + dy2));
    }


    //------------------------------------------------------------------------
    void vcgen_stroke::calc_cap(const vertex_dist& v0, 
                                const vertex_dist& v1, 
                                double len)
    {
        m_out_vertices.remove_all();

        double dx1 = m_width * (v1.y - v0.y) / len;
        double dy1 = m_width * (v1.x - v0.x) / len;
        double dx2 = 0;
        double dy2 = 0;


        if(m_line_cap == square_cap)
        {
            dx2 = dy1;
            dy2 = dx1;
        }

        if(m_line_cap == round_cap)
        {
            double a1 = atan2(dy1, -dx1);
            double a2 = a1 + pi;
            double da = fabs(1.0 / (m_width * m_approx_scale));
            while(a1 < a2)
            {
                m_out_vertices.add(coord_type(v0.x + cos(a1) * m_width, 
                                              v0.y + sin(a1) * m_width));
                a1 += da;
            }
            m_out_vertices.add(coord_type(v0.x + dx1, v0.y - dy1));
        }
        else
        {
            m_out_vertices.add(coord_type(v0.x - dx1 - dx2, v0.y + dy1 - dy2));
            m_out_vertices.add(coord_type(v0.x + dx1 - dx2, v0.y - dy1 - dy2));
        }
    }



    //------------------------------------------------------------------------
    void vcgen_stroke::calc_miter(const vertex_dist& v0, 
                                  const vertex_dist& v1, 
                                  const vertex_dist& v2,
                                  double dx1, double dy1, 
                                  double dx2, double dy2,
                                  bool revert_flag)
    {
        double xi = v1.x;
        double yi = v1.y;
        if(!calc_intersection(v0.x + dx1, v0.y - dy1,
                              v1.x + dx1, v1.y - dy1,
                              v1.x + dx2, v1.y - dy2,
                              v2.x + dx2, v2.y - dy2,
                              &xi, &yi))
        {
            // The calculation didn't succeed, most probaly
            // the the three points lie one straight line
            //----------------
            m_out_vertices.add(coord_type(v1.x + dx1, v1.y - dy1));
        }
        else
        {
            double d1 = calc_distance(v1.x, v1.y, xi, yi);
            double lim = m_width * m_miter_limit;
            if(d1 > lim)
            {
                // Miter limit exceeded
                //------------------------
                if(revert_flag)
                {
                    // For the compatibility with SVG, PDF, etc, 
                    // we use a simple bevel join instead of
                    // "smart" bevel
                    //-------------------
                    m_out_vertices.add(coord_type(v1.x + dx1, v1.y - dy1));
                    m_out_vertices.add(coord_type(v1.x + dx2, v1.y - dy2));
                }
                else
                {
                    // Smart bevel that cuts the miter at the limit point
                    //-------------------
                    d1  = lim / d1;
                    double x1 = v1.x + dx1;
                    double y1 = v1.y - dy1;
                    double x2 = v1.x + dx2;
                    double y2 = v1.y - dy2;

                    x1 += (xi - x1) * d1;
                    y1 += (yi - y1) * d1;
                    x2 += (xi - x2) * d1;
                    y2 += (yi - y2) * d1;
                    m_out_vertices.add(coord_type(x1, y1));
                    m_out_vertices.add(coord_type(x2, y2));
                }
            }
            else
            {
                // Inside the miter limit
                //---------------------
                m_out_vertices.add(coord_type(xi, yi));
            }
        }
    }


    //------------------------------------------------------------------------
    void vcgen_stroke::calc_join(const vertex_dist& v0, 
                                 const vertex_dist& v1, 
                                 const vertex_dist& v2,
                                 double len1, double len2)
    {
        double dx1, dy1, dx2, dy2;

        dx1 = m_width * (v1.y - v0.y) / len1;
        dy1 = m_width * (v1.x - v0.x) / len1;

        dx2 = m_width * (v2.y - v1.y) / len2;
        dy2 = m_width * (v2.x - v1.x) / len2;

        m_out_vertices.remove_all();
        if(m_line_join == miter_join)
        {
            calc_miter(v0, v1, v2, dx1, dy1, dx2, dy2, m_line_join == miter_join_revert);
        }
        else
        {
            if(calc_point_location(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y) > 0.0)
            {
                calc_miter(v0, v1, v2, dx1, dy1, dx2, dy2, false);
            }
            else
            {
                if(m_line_join == round_join)
                {
                    calc_arc(v1.x, v1.y, dx1, -dy1, dx2, -dy2);
                }
                else
                {
                    if(m_line_join == miter_join_revert)
                    {
                        calc_miter(v0, v1, v2, dx1, dy1, dx2, dy2, true);
                    }
                    else
                    {
                        m_out_vertices.add(coord_type(v1.x + dx1, v1.y - dy1));
                        m_out_vertices.add(coord_type(v1.x + dx2, v1.y - dy2));
                    }
                }
            }
        }
    }


}
