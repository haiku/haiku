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
// class renderer_base
//
//----------------------------------------------------------------------------

#ifndef AGG_RENDERER_BASE_INCLUDED
#define AGG_RENDERER_BASE_INCLUDED

#include "agg_basics.h"
#include "agg_rendering_buffer.h"

namespace agg
{
    //-----------------------------------------------------------renderer_base
    template<class PixelFormat> class renderer_base
    {
    public:
        typedef PixelFormat pixfmt_type;
        typedef typename pixfmt_type::color_type color_type;

        //--------------------------------------------------------------------
        renderer_base(pixfmt_type& ren) :
            m_ren(&ren),
            m_clip_box(0, 0, ren.width() - 1, ren.height() - 1)
        {
        }

        //--------------------------------------------------------------------
        const pixfmt_type& ren() const { return *m_ren;  }
        pixfmt_type& ren() { return *m_ren;  }
          
        //--------------------------------------------------------------------
        unsigned width()  const { return m_ren->width();  }
        unsigned height() const { return m_ren->height(); }

        //--------------------------------------------------------------------
        bool clip_box(int x1, int y1, int x2, int y2)
        {
            rect cb(x1, y1, x2, y2);
            cb.normalize();
            if(cb.clip(rect(0, 0, width() - 1, height() - 1)))
            {
                m_clip_box = cb;
                return true;
            }
            m_clip_box.x1 = 1;
            m_clip_box.y1 = 1;
            m_clip_box.x2 = 0;
            m_clip_box.y2 = 0;
            return false;
        }

        //--------------------------------------------------------------------
        void reset_clipping(bool visibility)
        {
            if(visibility)
            {
                m_clip_box.x1 = 0;
                m_clip_box.y1 = 0;
                m_clip_box.x2 = width() - 1;
                m_clip_box.y2 = height() - 1;
            }
            else
            {
                m_clip_box.x1 = 1;
                m_clip_box.y1 = 1;
                m_clip_box.x2 = 0;
                m_clip_box.y2 = 0;
            }
        }

        //--------------------------------------------------------------------
        void clip_box_naked(int x1, int y1, int x2, int y2)
        {
            m_clip_box.x1 = x1;
            m_clip_box.y1 = y1;
            m_clip_box.x2 = x2;
            m_clip_box.y2 = y2;
        }

        //--------------------------------------------------------------------
        bool inbox(int x, int y) const
        {
            return x >= m_clip_box.x1 && y >= m_clip_box.y1 &&
                   x <= m_clip_box.x2 && y <= m_clip_box.y2;
        }
        
        //--------------------------------------------------------------------
        void first_clip_box() {}
        bool next_clip_box() { return false; }

        //--------------------------------------------------------------------
        const rect& clip_box() const { return m_clip_box;    }
        int         xmin()     const { return m_clip_box.x1; }
        int         ymin()     const { return m_clip_box.y1; }
        int         xmax()     const { return m_clip_box.x2; }
        int         ymax()     const { return m_clip_box.y2; }

        //--------------------------------------------------------------------
        const rect& bounding_clip_box() const { return m_clip_box;    }
        int         bounding_xmin()     const { return m_clip_box.x1; }
        int         bounding_ymin()     const { return m_clip_box.y1; }
        int         bounding_xmax()     const { return m_clip_box.x2; }
        int         bounding_ymax()     const { return m_clip_box.y2; }

        //--------------------------------------------------------------------
        void clear(const color_type& c)
        {
            unsigned y;
            if(width())
            {
                for(y = 0; y < height(); y++)
                {
                    m_ren->copy_hline(0, y, width(), c);
                }
            }
        }
          
        //--------------------------------------------------------------------
        void copy_pixel(int x, int y, const color_type& c)
        {
            if(inbox(x, y))
            {
                m_ren->copy_pixel(x, y, c);
            }
        }

        //--------------------------------------------------------------------
        void blend_pixel(int x, int y, const color_type& c, cover_type cover)
        {
            if(inbox(x, y))
            {
                m_ren->blend_pixel(x, y, c, cover);
            }
        }

        //--------------------------------------------------------------------
        color_type pixel(int x, int y) const
        {
            return inbox(x, y) ? 
                   m_ren->pixel(x, y) :
                   color_type::no_color();
        }

        //--------------------------------------------------------------------
        void copy_hline(int x1, int y, int x2, const color_type& c)
        {
            if(x1 > x2) { int t = x2; x2 = x1; x1 = t; }
            if(y  > ymax()) return;
            if(y  < ymin()) return;
            if(x1 > xmax()) return;
            if(x2 < xmin()) return;

            if(x1 < xmin()) x1 = xmin();
            if(x2 > xmax()) x2 = xmax();

            m_ren->copy_hline(x1, y, x2 - x1 + 1, c);
        }

        //--------------------------------------------------------------------
        void copy_vline(int x, int y1, int y2, const color_type& c)
        {
            if(y1 > y2) { int t = y2; y2 = y1; y1 = t; }
            if(x  > xmax()) return;
            if(x  < xmin()) return;
            if(y1 > ymax()) return;
            if(y2 < ymin()) return;

            if(y1 < ymin()) y1 = ymin();
            if(y2 > ymax()) y2 = ymax();

            m_ren->copy_vline(x, y1, y2 - y1 + 1, c);
        }

        //--------------------------------------------------------------------
        void blend_hline(int x1, int y, int x2, 
                         const color_type& c, cover_type cover)
        {
            if(x1 > x2) { int t = x2; x2 = x1; x1 = t; }
            if(y  > ymax()) return;
            if(y  < ymin()) return;
            if(x1 > xmax()) return;
            if(x2 < xmin()) return;

            if(x1 < xmin()) x1 = xmin();
            if(x2 > xmax()) x2 = xmax();

            m_ren->blend_hline(x1, y, x2 - x1 + 1, c, cover);
        }

        //--------------------------------------------------------------------
        void blend_vline(int x, int y1, int y2, 
                         const color_type& c, cover_type cover)
        {
            if(y1 > y2) { int t = y2; y2 = y1; y1 = t; }
            if(x  > xmax()) return;
            if(x  < xmin()) return;
            if(y1 > ymax()) return;
            if(y2 < ymin()) return;

            if(y1 < ymin()) y1 = ymin();
            if(y2 > ymax()) y2 = ymax();

            m_ren->blend_vline(x, y1, y2 - y1 + 1, c, cover);
        }


        //--------------------------------------------------------------------
        void copy_bar(int x1, int y1, int x2, int y2, const color_type& c)
        {
            rect rc(x1, y1, x2, y2);
            rc.normalize();
            if(rc.clip(clip_box()))
            {
                int y;
                for(y = rc.y1; y <= rc.y2; y++)
                {
                    m_ren->copy_hline(rc.x1, y, unsigned(rc.x2 - rc.x1 + 1), c);
                }
            }
        }

        //--------------------------------------------------------------------
        void blend_bar(int x1, int y1, int x2, int y2, 
                       const color_type& c, cover_type cover)
        {
            rect rc(x1, y1, x2, y2);
            rc.normalize();
            if(rc.clip(clip_box()))
            {
                int y;
                for(y = rc.y1; y <= rc.y2; y++)
                {
                    m_ren->blend_hline(rc.x1,
                                       y,
                                       unsigned(rc.x2 - rc.x1 + 1), 
                                       c, 
                                       cover);
                }
            }
        }


        //--------------------------------------------------------------------
        void blend_solid_hspan(int x, int y, int len, 
                               const color_type& c, 
                               const cover_type* covers)
        {
            if(y > ymax()) return;
            if(y < ymin()) return;

            if(x < xmin())
            {
                len -= xmin() - x;
                if(len <= 0) return;
                covers += xmin() - x;
                x = xmin();
            }
            if(x + len > xmax())
            {
                len = xmax() - x + 1;
                if(len <= 0) return;
            }
            m_ren->blend_solid_hspan(x, y, len, c, covers);
        }

        //--------------------------------------------------------------------
        void blend_solid_vspan(int x, int y, int len, 
                               const color_type& c, 
                               const cover_type* covers)
        {
            if(x > xmax()) return;
            if(x < xmin()) return;

            if(y < ymin())
            {
                len -= ymin() - y;
                if(len <= 0) return;
                covers += ymin() - y;
                y = ymin();
            }
            if(y + len > ymax())
            {
                len = ymax() - y + 1;
                if(len <= 0) return;
            }
            m_ren->blend_solid_vspan(x, y, len, c, covers);
        }

        //--------------------------------------------------------------------
        void blend_color_hspan(int x, int y, int len, 
                               const color_type* colors, 
                               const cover_type* covers,
                               cover_type cover = cover_full)
        {
            if(y > ymax()) return;
            if(y < ymin()) return;

            if(x < xmin())
            {
                int d = xmin() - x;
                len -= d;
                if(len <= 0) return;
                if(covers) covers += d;
                colors += d;
                x = xmin();
            }
            if(x + len > xmax())
            {
                len = xmax() - x + 1;
                if(len <= 0) return;
            }
            m_ren->blend_color_hspan(x, y, len, colors, covers, cover);
        }

        //--------------------------------------------------------------------
        void blend_color_vspan(int x, int y, int len, 
                               const color_type* colors, 
                               const cover_type* covers,
                               cover_type cover = cover_full)
        {
            if(x > xmax()) return;
            if(x < xmin()) return;

            if(y < ymin())
            {
                int d = ymin() - y;
                len -= d;
                if(len <= 0) return;
                if(covers) covers += d;
                colors += d;
                y = ymin();
            }
            if(y + len > ymax())
            {
                len = ymax() - y + 1;
                if(len <= 0) return;
            }
            m_ren->blend_color_vspan(x, y, len, colors, covers, cover);
        }

        //--------------------------------------------------------------------
        void blend_color_hspan_no_clip(int x, int y, int len, 
                                       const color_type* colors, 
                                       const cover_type* covers,
                                       cover_type cover = cover_full)
        {
            m_ren->blend_color_hspan(x, y, len, colors, covers, cover);
        }

        //--------------------------------------------------------------------
        void blend_color_vspan_no_clip(int x, int y, int len, 
                                       const color_type* colors, 
                                       const cover_type* covers,
                                       cover_type cover = cover_full)
        {
            m_ren->blend_color_vspan(x, y, len, colors, covers, cover);
        }

        //--------------------------------------------------------------------
        void copy_from(const rendering_buffer& from, 
                       const rect* rc=0, 
                       int x_to=0, 
                       int y_to=0)
        {
            rect tmp_rect(0, 0, from.width(), from.height());
            if(rc == 0)
            {
                rc = &tmp_rect;
            }

            rect rc2(*rc);
            rc2.normalize();
            if(rc2.clip(rect(0, 0, from.width() - 1, from.height() - 1)))
            {
                rect rc3(x_to + rc2.x1 - rc->x1, 
                         y_to + rc2.y1 - rc->y1, 
                         x_to + rc2.x2 - rc->x1, 
                         y_to + rc2.y2 - rc->y1);
                rc3.normalize();

                if(rc3.clip(clip_box()))
                {
                    while(rc3.y1 <= rc3.y2)
                    {
                        m_ren->copy_from(from, 
                                         rc3.x1, rc3.y1,
                                         rc2.x1, rc2.y1,
                                         rc3.x2 - rc3.x1 + 1);
                        ++rc2.y1;
                        ++rc3.y1;
                    }
                }
            }
        }
       

    private:
        pixfmt_type* m_ren;
        rect         m_clip_box;
    };


}

#endif
