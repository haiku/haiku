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
// Contour generator
//
//----------------------------------------------------------------------------

#include <math.h>
#include "agg_vcgen_contour.h"

namespace agg
{

    //------------------------------------------------------------------------
    vcgen_contour::vcgen_contour() :
        m_src_vertices(),
        m_width(1.0),
        m_abs_width(1.0),
        m_signed_width(1.0),
        m_miter_limit(4.0),
        m_status(initial),
        m_src_vertex(0),
        m_closed(0),
        m_orientation(0),
        m_auto_detect(true)
    {
    }


    //------------------------------------------------------------------------
    void vcgen_contour::remove_all()
    {
        m_src_vertices.remove_all();
        m_closed = 0;
        m_orientation = 0;
        m_abs_width = fabs(m_width);
        m_signed_width = m_width;
        m_status = initial;
    }


    //------------------------------------------------------------------------
    void vcgen_contour::miter_limit_theta(double t)
    { 
        m_miter_limit = 1.0 / sin(t * 0.5) ;
    }


    //------------------------------------------------------------------------
    void vcgen_contour::add_vertex(double x, double y, unsigned cmd)
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
                if(is_end_poly(cmd))
                {
                    m_closed = get_close_flag(cmd);
                    if(m_orientation == path_flags_none) 
                    {
                        m_orientation = get_orientation(cmd);
                    }
                }
            }
        }
    }


    //------------------------------------------------------------------------
    void vcgen_contour::rewind(unsigned)
    {
        if(m_status == initial)
        {
            m_src_vertices.close(true);
            m_signed_width = m_width;
            if(m_auto_detect)
            {
                if(!is_oriented(m_orientation))
                {
                    m_orientation = (calc_polygon_area(m_src_vertices) > 0.0) ? 
                                    path_flags_ccw : 
                                    path_flags_cw;
                }
            }
            if(is_oriented(m_orientation))
            {
                m_signed_width = is_ccw(m_orientation) ? m_width : -m_width;
            }
        }
        m_status = ready;
        m_src_vertex = 0;
    }


    //------------------------------------------------------------------------
    unsigned vcgen_contour::vertex(double* x, double* y)
    {
        bool done = false;
        while(!done)
        {
            switch(m_status)
            {
            case initial:
                rewind(0);

            case ready:
                if(m_src_vertices.size() <  3)
                {
                    return path_cmd_stop;
                }
                m_src_vertex = 0;
                m_status = outline;

            case outline:
                if(m_src_vertex >= m_src_vertices.size())
                {
                    m_status = end_poly;
                    return path_cmd_end_poly | m_orientation | m_closed;
                }
                if(calc_miter(m_src_vertices.prev(m_src_vertex), 
                              m_src_vertices.curr(m_src_vertex), 
                              m_src_vertices.next(m_src_vertex)))
                {
                    m_status = add_point;
                }
                ++m_src_vertex;
                *x = m_x1;
                *y = m_y1;
                return ((m_src_vertex == 1) ? path_cmd_move_to : path_cmd_line_to);

            case add_point:
                *x = m_x2;
                *y = m_y2;
                m_status = outline;
                return path_cmd_line_to;

            case end_poly:
                done = true;
                break;
            }
        }
        return path_cmd_stop;
    }



    //------------------------------------------------------------------------
    bool vcgen_contour::calc_miter(const vertex_dist& v0, 
                                   const vertex_dist& v1, 
                                   const vertex_dist& v2)
    {
        double dx1, dy1, dx2, dy2;

        dx1 = m_signed_width * (v1.y - v0.y) / v0.dist;
        dy1 = m_signed_width * (v1.x - v0.x) / v0.dist;

        dx2 = m_signed_width * (v2.y - v1.y) / v1.dist;
        dy2 = m_signed_width * (v2.x - v1.x) / v1.dist;

        double xi;
        double yi;

        if(!calc_intersection(v0.x + dx1, v0.y - dy1,
                              v1.x + dx1, v1.y - dy1,
                              v1.x + dx2, v1.y - dy2,
                              v2.x + dx2, v2.y - dy2,
                              &xi, &yi))
        {
            m_x1 = v1.x + dx1;
            m_y1 = v1.y - dy1;
            return false;
        }
        else
        {
            double d1 = calc_distance(v1.x, v1.y, xi, yi);
            double lim = m_abs_width * m_miter_limit;
            if(d1 > lim)
            {
                d1  = lim / d1;
                m_x1 = v1.x + dx1;
                m_y1 = v1.y - dy1;
                m_x2 = v1.x + dx2;
                m_y2 = v1.y - dy2;

                m_x1 += (xi - m_x1) * d1;
                m_y1 += (yi - m_y1) * d1;
                m_x2 += (xi - m_x2) * d1;
                m_y2 += (yi - m_y2) * d1;
                return true;
            }
            else
            {
                m_x1 = xi;
                m_y1 = yi;
            }
        }
        return false;
    }


}
