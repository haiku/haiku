/*
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
 *
 *
 */
 
#ifndef AGG_RASTERIZER_SCANLINE_AA_SUBPIX_INCLUDED
#define AGG_RASTERIZER_SCANLINE_AA_SUBPIX_INCLUDED

#include "agg_rasterizer_cells_aa.h"
#include "agg_rasterizer_sl_clip.h"
#include "agg_gamma_functions.h"


namespace agg
{
	template<class Clip=rasterizer_sl_clip_int> class rasterizer_scanline_aa_subpix
	{
		enum status
		{
			status_initial,
			status_move_to,
			status_line_to,
			status_closed
		};

	public:
		typedef Clip					  clip_type;
		typedef typename Clip::conv_type  conv_type;
		typedef typename Clip::coord_type coord_type;

		enum aa_scale_e
		{
			aa_shift  = 8,
			aa_scale  = 1 << aa_shift,
			aa_mask	  = aa_scale - 1,
			aa_scale2 = aa_scale * 2,
			aa_mask2  = aa_scale2 - 1
		};

		//--------------------------------------------------------------------
		rasterizer_scanline_aa_subpix() : 
			m_outline(),
			m_clipper(),
			m_filling_rule(fill_non_zero),
			m_auto_close(true),
			m_start_x(0),
			m_start_y(0),
			m_status(status_initial)
		{
			int i;
			for(i = 0; i < aa_scale; i++) m_gamma[i] = i;
		}

		//--------------------------------------------------------------------
		template<class GammaF> 
		rasterizer_scanline_aa_subpix(const GammaF& gamma_function) : 
			m_outline(),
			m_clipper(m_outline),
			m_filling_rule(fill_non_zero),
			m_auto_close(true),
			m_start_x(0),
			m_start_y(0),
			m_status(status_initial)
		{
			gamma(gamma_function);
		}

		//--------------------------------------------------------------------
		void reset(); 
		void reset_clipping();
		void clip_box(double x1, double y1, double x2, double y2);
		void filling_rule(filling_rule_e filling_rule);
		void auto_close(bool flag) { m_auto_close = flag; }

		//--------------------------------------------------------------------
		template<class GammaF> void gamma(const GammaF& gamma_function)
		{ 
			int i;
			for(i = 0; i < aa_scale; i++)
			{
				m_gamma[i] = uround(gamma_function(double(i) / aa_mask) * aa_mask);
			}
		}

		//--------------------------------------------------------------------
		unsigned apply_gamma(unsigned cover) const 
		{ 
			return m_gamma[cover]; 
		}

		//--------------------------------------------------------------------
		void move_to(int x, int y);
		void line_to(int x, int y);
		void move_to_d(double x, double y);
		void line_to_d(double x, double y);
		void close_polygon();
		void add_vertex(double x, double y, unsigned cmd);

		void edge(int x1, int y1, int x2, int y2);
		void edge_d(double x1, double y1, double x2, double y2);

		//-------------------------------------------------------------------
		template<class VertexSource>
		void add_path(VertexSource& vs, unsigned path_id=0)
		{
			double x;
			double y;

			unsigned cmd;
			vs.rewind(path_id);
			if(m_outline.sorted()) reset();
			while(!is_stop(cmd = vs.vertex(&x, &y)))
			{
				if (is_vertex(cmd)) {
					x *= 3;
				}
				add_vertex(x, y, cmd);
			}
		}
		
		//--------------------------------------------------------------------
		int min_x() const { return m_outline.min_x() / 3; }
		int min_y() const { return m_outline.min_y(); }
		int max_x() const { return m_outline.max_x() / 3; }
		int max_y() const { return m_outline.max_y(); }

		//--------------------------------------------------------------------
		void sort();
		bool rewind_scanlines();
		bool navigate_scanline(int y);

		//--------------------------------------------------------------------
		AGG_INLINE unsigned calculate_alpha(int area) const
		{
			int cover = area >> (poly_subpixel_shift*2 + 1 - aa_shift);

			if(cover < 0) cover = -cover;
			if(m_filling_rule == fill_even_odd)
			{
				cover &= aa_mask2;
				if(cover > aa_scale)
				{
					cover = aa_scale2 - cover;
				}
			}
			if(cover > aa_mask) cover = aa_mask;
			return m_gamma[cover];
		}

		//--------------------------------------------------------------------
		template<class Scanline> bool sweep_scanline(Scanline& sl)
		{
			for(;;)
			{
				if(m_scan_y > m_outline.max_y()) return false;
				sl.reset_spans();
				unsigned num_cells = m_outline.scanline_num_cells(m_scan_y);
				const cell_aa* const* cells = m_outline.scanline_cells(m_scan_y);
				int cover = 0;
				int cover2 = 0;
				int cover3 = 0;

				while(num_cells)
				{
					const cell_aa* cur_cell = *cells;
					int x	 = cur_cell->x;
					int area1 = cur_cell->area;
					int area2;
					int area3;
					unsigned alpha1;
					unsigned alpha2;
					unsigned alpha3;

					int last_cover = cover3;
					cover = cover3;
					cover += cur_cell->cover;

					while(--num_cells)
					{
						cur_cell = *++cells;
						if(cur_cell->x != x) break;
						area1  += cur_cell->area;
						cover += cur_cell->cover;
					}
					
					if (x % 3 == 0)
					{
						if (cur_cell->x == x + 1)
						{
							area2 = cur_cell->area;
							cover2 = cover + cur_cell->cover;
							
							while (--num_cells)
							{
								cur_cell = *++cells;
								if (cur_cell->x != x+1) break;
								area2 += cur_cell->area;
								cover2 += cur_cell->cover;
							}
						}
						else
						{
							area2 = 0;
							cover2 = cover;
						}
						
						if (cur_cell->x == x + 2)
						{
							area3 = cur_cell->area;
							cover3 = cover2 + cur_cell->cover;
							
							while (--num_cells)
							{
								cur_cell = *++cells;
								if (cur_cell->x != x+2) break;
								area3 += cur_cell->area;
								cover3 += cur_cell->cover;
							}
						}
						else
						{
							area3 = 0;
							cover3 = cover2;
						}
					}
					else if (x % 3 == 1)
					{
						area2 = area1;
						area1 = 0;
						cover2 = cover;
						cover = last_cover;
						if (cur_cell->x == x+1)
						{
							area3 = cur_cell->area;
							cover3 = cover2 + cur_cell->cover;
							
							while (--num_cells)
							{
								cur_cell = *++cells;
								if (cur_cell->x != x+1) break;
								area3 += cur_cell->area;
								cover3 += cur_cell->cover;
							}
						}
						else
						{
							area3 = 0;
							cover3 = cover2;
						}
					}
					else // if (x % 3 == 2)
					{
						area3 = area1;
						area2 = 0;
						area1 = 0;
						cover3 = cover;
						cover = last_cover;
						cover2 = last_cover;
					}
					
					alpha1 = area1 ? calculate_alpha((cover 
						<< (poly_subpixel_shift + 1)) - area1) : 0;
					alpha2 = area2 ? calculate_alpha((cover2 
						<< (poly_subpixel_shift + 1)) - area2) : 0;
					alpha3 = area3 ? calculate_alpha((cover3 
						<< (poly_subpixel_shift + 1)) - area3) : 0;
					if(alpha1 || alpha2 || alpha3)
					{
						x += 3 - (x % 3);
						if (area1 && !area2 && area3)
						{
							alpha2 = calculate_alpha(cover
								<< (poly_subpixel_shift + 1));
						}
						else if (num_cells && cur_cell->x >= x)
						{
							if (area1 && !area2)
							{
								alpha2 = calculate_alpha(cover
									<< (poly_subpixel_shift + 1));
								alpha3 = alpha2;
							}
							if (area2 && !area3)
							{
								alpha3 = calculate_alpha(cover2
									<< (poly_subpixel_shift + 1));
							}
						}
						if (!area1)
						{
							if (area2)
							{
								alpha1 = calculate_alpha(cover
									<< (poly_subpixel_shift + 1));
							}
							else if (area3)
							{
								alpha2 = calculate_alpha(cover
									<< (poly_subpixel_shift + 1));
								alpha1 = alpha2;
							}
						}
						sl.add_cell(x / 3 - 1, alpha1, alpha2, alpha3);
					}
					
					if (num_cells && cur_cell->x - x >= 3)
					{
						alpha1 = calculate_alpha(cover3
							<< (poly_subpixel_shift + 1));
						sl.add_span(x / 3, cur_cell->x / 3 - x / 3, alpha1);
					}
				}
		
				if(sl.num_spans()) break;
				++m_scan_y;
			}

			sl.finalize(m_scan_y);
			++m_scan_y;
			return true;
		}

		//--------------------------------------------------------------------
		bool hit_test(int tx, int ty);


	private:
		//--------------------------------------------------------------------
		// Disable copying
		rasterizer_scanline_aa_subpix(const rasterizer_scanline_aa_subpix<Clip>&);
		const rasterizer_scanline_aa_subpix<Clip>& 
		operator = (const rasterizer_scanline_aa_subpix<Clip>&);

	private:
		rasterizer_cells_aa<cell_aa> m_outline;
		clip_type	   m_clipper;
		int			   m_gamma[aa_scale];
		filling_rule_e m_filling_rule;
		bool		   m_auto_close;
		coord_type	   m_start_x;
		coord_type	   m_start_y;
		unsigned	   m_status;
		int			   m_scan_y;
	};












	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::reset() 
	{ 
		m_outline.reset(); 
		m_status = status_initial;
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::filling_rule(filling_rule_e filling_rule) 
	{ 
		m_filling_rule = filling_rule; 
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::clip_box(double x1, double y1, 
												double x2, double y2)
	{
		reset();
		m_clipper.clip_box(3 * conv_type::downscale(x1), conv_type::upscale(y1), 
						   conv_type::upscale(3 * x2), conv_type::upscale(y2));
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::reset_clipping()
	{
		reset();
		m_clipper.reset_clipping();
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::close_polygon()
	{
		if(m_status == status_line_to)
		{
			m_clipper.line_to(m_outline, m_start_x, m_start_y);
			m_status = status_closed;
		}
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::move_to(int x, int y)
	{
		if(m_outline.sorted()) reset();
		if(m_auto_close) close_polygon();
		m_clipper.move_to(m_start_x = conv_type::downscale(x), 
						  m_start_y = conv_type::downscale(y));
		m_status = status_move_to;
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::line_to(int x, int y)
	{
		m_clipper.line_to(m_outline, 
						  conv_type::downscale(x), 
						  conv_type::downscale(y));
		m_status = status_line_to;
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::move_to_d(double x, double y) 
	{ 
		if(m_outline.sorted()) reset();
		if(m_auto_close) close_polygon();
		m_clipper.move_to(m_start_x = conv_type::upscale(x), 
						  m_start_y = conv_type::upscale(y)); 
		m_status = status_move_to;
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::line_to_d(double x, double y) 
	{ 
		m_clipper.line_to(m_outline, 
						  conv_type::upscale(x), 
						  conv_type::upscale(y)); 
		m_status = status_line_to;
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::add_vertex(double x, double y, unsigned cmd)
	{
		if(is_move_to(cmd)) 
		{
			move_to_d(x, y);
		}
		else 
		if(is_vertex(cmd))
		{
			line_to_d(x, y);
		}
		else
		if(is_close(cmd))
		{
			close_polygon();
		}
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::edge(int x1, int y1, int x2, int y2)
	{
		if(m_outline.sorted()) reset();
		m_clipper.move_to(conv_type::downscale(x1), conv_type::downscale(y1));
		m_clipper.line_to(m_outline, 
						  conv_type::downscale(x2), 
						  conv_type::downscale(y2));
		m_status = status_move_to;
	}
	
	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::edge_d(double x1, double y1, 
											  double x2, double y2)
	{
		if(m_outline.sorted()) reset();
		m_clipper.move_to(conv_type::upscale(x1), conv_type::upscale(y1)); 
		m_clipper.line_to(m_outline, 
						  conv_type::upscale(x2), 
						  conv_type::upscale(y2)); 
		m_status = status_move_to;
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	void rasterizer_scanline_aa_subpix<Clip>::sort()
	{
		m_outline.sort_cells();
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	AGG_INLINE bool rasterizer_scanline_aa_subpix<Clip>::rewind_scanlines()
	{
		if(m_auto_close) close_polygon();
		m_outline.sort_cells();
		if(m_outline.total_cells() == 0) 
		{
			return false;
		}
		m_scan_y = m_outline.min_y();
		return true;
	}


	//------------------------------------------------------------------------
	template<class Clip> 
	AGG_INLINE bool rasterizer_scanline_aa_subpix<Clip>::navigate_scanline(int y)
	{
		if(m_auto_close) close_polygon();
		m_outline.sort_cells();
		if(m_outline.total_cells() == 0 || 
		   y < m_outline.min_y() || 
		   y > m_outline.max_y()) 
		{
			return false;
		}
		m_scan_y = y;
		return true;
	}

	//------------------------------------------------------------------------
	template<class Clip> 
	bool rasterizer_scanline_aa_subpix<Clip>::hit_test(int tx, int ty)
	{
		if(!navigate_scanline(ty)) return false;
		scanline_hit_test sl(tx);
		sweep_scanline(sl);
		return sl.hit();
	}



}



#endif

