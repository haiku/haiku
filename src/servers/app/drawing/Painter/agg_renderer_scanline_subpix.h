/*
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
 *
 *
 */

#include <Region.h>

#include "agg_basics.h"
#include "agg_array.h"
#include "agg_renderer_base.h"
#include "agg_renderer_scanline.h"


namespace agg
{

	//============================================render_scanline_subpix_solid
	template<class Scanline, class BaseRenderer, class ColorT>
	void render_scanline_subpix_solid(const Scanline& sl,
								  BaseRenderer& ren,
								  const ColorT& color)
	{
		int y = sl.y();
		unsigned num_spans = sl.num_spans();
		typename Scanline::const_iterator span = sl.begin();

		for(;;)
		{
			int x = span->x;
			if(span->len > 0)
			{
				ren.blend_solid_hspan_subpix(x, y, (unsigned)span->len,
									  color,
									  span->covers);
			}
			else
			{
				ren.blend_hline(x, y, (unsigned)(x - (span->len / 3) - 1),
								color,
								*(span->covers));
			}
			if(--num_spans == 0) break;
			++span;
		}
	}

	//==========================================renderer_scanline_subpix_solid
	template<class BaseRenderer> class renderer_scanline_subpix_solid
	{
	public:
		typedef BaseRenderer base_ren_type;
		typedef typename base_ren_type::color_type color_type;

		//--------------------------------------------------------------------
		renderer_scanline_subpix_solid() : m_ren(0) {}
		renderer_scanline_subpix_solid(base_ren_type& ren) : m_ren(&ren) {}
		void attach(base_ren_type& ren)
		{
			m_ren = &ren;
		}

		//--------------------------------------------------------------------
		void color(const color_type& c) { m_color = c; }
		const color_type& color() const { return m_color; }

		//--------------------------------------------------------------------
		void prepare() {}

		//--------------------------------------------------------------------
		template<class Scanline> void render(const Scanline& sl)
		{
			render_scanline_subpix_solid(sl, *m_ren, m_color);
		}

	private:
		base_ren_type* m_ren;
		color_type m_color;
	};
}
