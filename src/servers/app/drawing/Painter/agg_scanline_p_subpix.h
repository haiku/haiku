/*
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
 *
 *
 * Class scanline_p8_subpix_avrg_filtering, a slightly modified version of
 * scanline_p8 customized to store 3 covers per pixel
 *
 */
 
#ifndef AGG_SCANLINE_P_SUBPIX_INCLUDED
#define AGG_SCANLINE_P_SUBPIX_INCLUDED

#include "agg_array.h"

namespace agg
{

	//======================================================scanline_p8_subpix
	// 
	// This is a general purpose scaline container which supports the interface 
	// used in the rasterizer::render(). See description of scanline_u8
	// for details.
	// 
	//------------------------------------------------------------------------
	class scanline_p8_subpix
	{
	public:
		typedef scanline_p8_subpix	self_type;
		typedef int8u				cover_type;
		typedef int16				coord_type;

		//--------------------------------------------------------------------
		struct span
		{
			coord_type		  x;
			coord_type		  len; // If negative, it's a solid span, covers is valid
			const cover_type* covers;
		};

		typedef span* iterator;
		typedef const span* const_iterator;

		scanline_p8_subpix() :
			m_last_x(0x7FFFFFF0),
			m_covers(),
			m_cover_ptr(0),
			m_spans(),
			m_cur_span(0)
		{
		}

		//--------------------------------------------------------------------
		void reset(int min_x, int max_x)
		{
			unsigned max_len = 3*(max_x - min_x + 3);
			if(max_len > m_spans.size())
			{
				m_spans.resize(max_len);
				m_covers.resize(max_len);
			}
			m_last_x	= 0x7FFFFFF0;
			m_cover_ptr = &m_covers[0];
			m_cur_span	= &m_spans[0];
			m_cur_span->len = 0;
		}

		//--------------------------------------------------------------------
		void add_cell(int x, unsigned cover1, unsigned cover2, unsigned cover3)
		{
			m_cover_ptr[0] = (cover_type)cover1;
			m_cover_ptr[1] = (cover_type)cover2;
			m_cover_ptr[2] = (cover_type)cover3;
			if(x == m_last_x+1 && m_cur_span->len > 0)
			{
				m_cur_span->len += 3;
			}
			else
			{
				m_cur_span++;
				m_cur_span->covers = m_cover_ptr;
				m_cur_span->x = (int16)x;
				m_cur_span->len = 3;
			}
			m_last_x = x;
			m_cover_ptr += 3;
		}

		//--------------------------------------------------------------------
		void add_cells(int x, unsigned len, const cover_type* covers)
		{
			memcpy(m_cover_ptr, covers, 3 * len * sizeof(cover_type));
			if(x == m_last_x+1 && m_cur_span->len > 0)
			{
				m_cur_span->len += 3 * (int16)len;
			}
			else
			{
				m_cur_span++;
				m_cur_span->covers = m_cover_ptr;
				m_cur_span->x = (int16)x;
				m_cur_span->len = 3 * (int16)len;
			}
			m_cover_ptr += 3 * len;
			m_last_x = x + len - 1;
		}

		//--------------------------------------------------------------------
		void add_span(int x, unsigned len, unsigned cover)
		{
			if(x == m_last_x+1 && 
			   m_cur_span->len < 0 && 
			   cover == *m_cur_span->covers)
			{
				m_cur_span->len -= 3 * (int16)len;
			}
			else
			{
				*m_cover_ptr = (cover_type)cover;
				m_cur_span++;
				m_cur_span->covers = m_cover_ptr;
				m_cover_ptr += 3;
				m_cur_span->x	   = (int16)x;
				m_cur_span->len	   = 3 * (int16)(-int(len));
			}
			m_last_x = x + len - 1;
		}

		//--------------------------------------------------------------------
		void finalize(int y) 
		{ 
			m_y = y; 
		}

		//--------------------------------------------------------------------
		void reset_spans()
		{
			m_last_x	= 0x7FFFFFF0;
			m_cover_ptr = &m_covers[0];
			m_cur_span	= &m_spans[0];
			m_cur_span->len = 0;
		}

		//--------------------------------------------------------------------
		int			   y()		   const { return m_y; }
		unsigned	   num_spans() const { return unsigned(m_cur_span - &m_spans[0]); }
		const_iterator begin()	   const { return &m_spans[1]; }

	private:
		scanline_p8_subpix(const self_type&);
		const self_type& operator = (const self_type&);

		int					  m_last_x;
		int					  m_y;
		pod_array<cover_type> m_covers;
		cover_type*			  m_cover_ptr;
		pod_array<span>		  m_spans;
		span*				  m_cur_span;
	};

}


#endif

