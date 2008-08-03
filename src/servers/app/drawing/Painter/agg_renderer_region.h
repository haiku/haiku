/*
 * Copyright 2005-2006, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
 *
 *
 * class renderer_region, slightly modified renderer_mclip which directly
 * uses a BRegion for clipping info.
 *
 */

#ifndef AGG_RENDERER_REGION_INCLUDED
#define AGG_RENDERER_REGION_INCLUDED

#include <Region.h>

#include "agg_basics.h"
#include "agg_array.h"
#include "agg_renderer_base.h"

namespace agg
{

	//----------------------------------------------------------renderer_region
	template<class PixelFormat> class renderer_region
	{
	public:
		typedef PixelFormat pixfmt_type;
		typedef typename pixfmt_type::color_type color_type;
		typedef renderer_base<pixfmt_type> base_ren_type;

		//--------------------------------------------------------------------
		renderer_region(pixfmt_type& ren) :
			m_ren(ren),
			m_region(NULL),
			m_curr_cb(0),
			m_bounds(m_ren.xmin(), m_ren.ymin(), m_ren.xmax(), m_ren.ymax())
		{
		}

		//--------------------------------------------------------------------
		const pixfmt_type& ren() const { return m_ren.ren();  }
		pixfmt_type& ren() { return m_ren.ren();  }

		//--------------------------------------------------------------------
		unsigned width()  const { return m_ren.width();	 }
		unsigned height() const { return m_ren.height(); }

		//--------------------------------------------------------------------
		const rect_i& clip_box() const { return m_ren.clip_box(); }
		int			  xmin()	 const { return m_ren.xmin(); }
		int			  ymin()	 const { return m_ren.ymin(); }
		int			  xmax()	 const { return m_ren.xmax(); }
		int			  ymax()	 const { return m_ren.ymax(); }

		//--------------------------------------------------------------------
		const rect_i& bounding_clip_box() const { return m_bounds;	  }
		int			  bounding_xmin()	  const { return m_bounds.x1; }
		int			  bounding_ymin()	  const { return m_bounds.y1; }
		int			  bounding_xmax()	  const { return m_bounds.x2; }
		int			  bounding_ymax()	  const { return m_bounds.y2; }

		//--------------------------------------------------------------------
		void first_clip_box()
		{
			m_curr_cb = 0;
			if(m_region && m_region->CountRects() > 0)
			{
				clipping_rect cb = m_region->RectAtInt(0);
				m_ren.clip_box_naked(cb.left, cb.top, cb.right, cb.bottom);
			}
			else
				m_ren.clip_box_naked(0, 0, -1, -1);
		}

		//--------------------------------------------------------------------
		bool next_clip_box()
		{
			if(m_region && (int)(++m_curr_cb) < m_region->CountRects())
			{
				clipping_rect cb = m_region->RectAtInt(m_curr_cb);
				m_ren.clip_box_naked(cb.left, cb.top, cb.right, cb.bottom);
				return true;
			}
			return false;
		}

		//--------------------------------------------------------------------
		void reset_clipping(bool visibility)
		{
			m_ren.reset_clipping(visibility);
			m_region = NULL;
			m_curr_cb = 0;
			m_bounds = m_ren.clip_box();
		}

		//--------------------------------------------------------------------
		void set_clipping_region(BRegion* region)
		{
			m_region = region;
			if (m_region) {
				clipping_rect r = m_region->FrameInt();
				if (r.left <= r.right && r.top <= r.bottom) {
					// clip rect_i to frame buffer bounds
					r.left = max_c(0, r.left);
					r.top = max_c(0, r.top);
					r.right = min_c((int)width() - 1, r.right);
					r.bottom = min_c((int)height() - 1, r.bottom);

					if(r.left < m_bounds.x1) m_bounds.x1 = r.left;
					if(r.top < m_bounds.y1) m_bounds.y1 = r.top;
					if(r.right > m_bounds.x2) m_bounds.x2 = r.right;
					if(r.bottom > m_bounds.y2) m_bounds.y2 = r.bottom;
				}
			}
		}

		//--------------------------------------------------------------------
		void clear(const color_type& c)
		{
			m_ren.clear(c);
		}

		//--------------------------------------------------------------------
		void copy_pixel(int x, int y, const color_type& c)
		{
			first_clip_box();
			do
			{
				if(m_ren.inbox(x, y))
				{
					m_ren.ren().copy_pixel(x, y, c);
					break;
				}
			}
			while(next_clip_box());
		}

		//--------------------------------------------------------------------
		void blend_pixel(int x, int y, const color_type& c, cover_type cover)
		{
			first_clip_box();
			do
			{
				if(m_ren.inbox(x, y))
				{
					m_ren.ren().blend_pixel(x, y, c, cover);
					break;
				}
			}
			while(next_clip_box());
		}

		//--------------------------------------------------------------------
		color_type pixel(int x, int y) const
		{
			first_clip_box();
			do
			{
				if(m_ren.inbox(x, y))
				{
					return m_ren.ren().pixel(x, y);
				}
			}
			while(next_clip_box());
			return color_type::no_color();
		}

		//--------------------------------------------------------------------
		void copy_hline(int x1, int y, int x2, const color_type& c)
		{
			first_clip_box();
			do
			{
				m_ren.copy_hline(x1, y, x2, c);
			}
			while(next_clip_box());
		}

		//--------------------------------------------------------------------
		void copy_vline(int x, int y1, int y2, const color_type& c)
		{
			first_clip_box();
			do
			{
				m_ren.copy_vline(x, y1, y2, c);
			}
			while(next_clip_box());
		}

		//--------------------------------------------------------------------
		void blend_hline(int x1, int y, int x2,
						 const color_type& c, cover_type cover)
		{
			first_clip_box();
			do
			{
				m_ren.blend_hline(x1, y, x2, c, cover);
			}
			while(next_clip_box());
		}
		
		//--------------------------------------------------------------------
		void blend_vline(int x, int y1, int y2,
						 const color_type& c, cover_type cover)
		{
			first_clip_box();
			do
			{
				m_ren.blend_vline(x, y1, y2, c, cover);
			}
			while(next_clip_box());
		}

		//--------------------------------------------------------------------
		void copy_bar(int x1, int y1, int x2, int y2, const color_type& c)
		{
			first_clip_box();
			do
			{
				m_ren.copy_bar(x1, y1, x2, y2, c);
			}
			while(next_clip_box());
		}

		//--------------------------------------------------------------------
		void blend_bar(int x1, int y1, int x2, int y2,
					   const color_type& c, cover_type cover)
		{
			first_clip_box();
			do
			{
				m_ren.blend_bar(x1, y1, x2, y2, c, cover);
			}
			while(next_clip_box());
		}


		//--------------------------------------------------------------------
		void blend_solid_hspan(int x, int y, int len,
							   const color_type& c, const cover_type* covers)
		{
			first_clip_box();
			do
			{
				m_ren.blend_solid_hspan(x, y, len, c, covers);
			}
			while(next_clip_box());
		}

		//--------------------------------------------------------------------
		void blend_solid_hspan_subpix(int x, int y, int len,
							   const color_type& c, const cover_type* covers)
		{
			first_clip_box();
			do
			{
				m_ren.blend_solid_hspan_subpix(x, y, len, c, covers);
			}
			while(next_clip_box());
		}

		//--------------------------------------------------------------------
		void blend_solid_vspan(int x, int y, int len,
							   const color_type& c, const cover_type* covers)
		{
			first_clip_box();
			do
			{
				m_ren.blend_solid_vspan(x, y, len, c, covers);
			}
			while(next_clip_box());
		}

		//--------------------------------------------------------------------
		void blend_color_hspan(int x, int y, int len,
							   const color_type* colors,
							   const cover_type* covers,
							   cover_type cover = cover_full)
		{
			first_clip_box();
			do
			{
				m_ren.blend_color_hspan(x, y, len, colors, covers, cover);
			}
			while(next_clip_box());
		}

		//--------------------------------------------------------------------
		void blend_color_vspan(int x, int y, int len,
							   const color_type* colors,
							   const cover_type* covers,
							   cover_type cover = cover_full)
		{
			first_clip_box();
			do
			{
				m_ren.blend_color_hspan(x, y, len, colors, covers, cover);
			}
			while(next_clip_box());
		}

		//--------------------------------------------------------------------
		void blend_color_hspan_no_clip(int x, int y, int len,
									   const color_type* colors,
									   const cover_type* covers,
									   cover_type cover = cover_full)
		{
			m_ren.blend_color_hspan_no_clip(x, y, len, colors, covers, cover);
		}

		//--------------------------------------------------------------------
		void blend_color_vspan_no_clip(int x, int y, int len,
									   const color_type* colors,
									   const cover_type* covers,
									   cover_type cover = cover_full)
		{
			m_ren.blend_color_vspan_no_clip(x, y, len, colors, covers, cover);
		}

		//--------------------------------------------------------------------
		void copy_from(const rendering_buffer& from,
					   const rect_i* rc=0,
					   int x_to=0,
					   int y_to=0)
		{
			first_clip_box();
			do
			{
				m_ren.copy_from(from, rc, x_to, y_to);
			}
			while(next_clip_box());
		}

	private:
		renderer_region(const renderer_region<PixelFormat>&);
		const renderer_region<PixelFormat>&
			operator = (const renderer_region<PixelFormat>&);

		base_ren_type	   m_ren;
		BRegion*		   m_region;
		unsigned		   m_curr_cb;
		rect_i			   m_bounds;
	};


}

#endif
