/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
 *
 * Class clipped_alpha_mask, a modified version of alpha_mask_u8 that can
 * offset the mask, and has a controllable value for the area outside it.
 */

#ifndef AGG_CLIPED_ALPHA_MASK_INCLUDED
#define AGG_CLIPED_ALPHA_MASK_INCLUDED


#include <agg_alpha_mask_u8.h>
#include <agg_rendering_buffer.h>


namespace agg
{
	class clipped_alpha_mask
	{
		public:
			typedef int8u cover_type;
			enum cover_scale_e
			{
				cover_shift = 8,
				cover_none  = 0,
				cover_full  = 255
			};

			clipped_alpha_mask()
				: m_xOffset(0), m_yOffset(0), m_rbuf(0), m_outside(0) {}
			clipped_alpha_mask(rendering_buffer& rbuf)
				: m_xOffset(0), m_yOffset(0), m_rbuf(&rbuf), m_outside(0) {}

			void attach(rendering_buffer& rbuf)
			{
				m_rbuf = &rbuf;
			}

			void attach(rendering_buffer& rbuf, int x, int y, int8u outside)
			{
				m_rbuf = &rbuf;
				m_xOffset = x;
				m_yOffset = y;
				m_outside = outside;
			}

			void combine_hspan(int x, int y, cover_type* dst, int num_pix) const
			{
				int count = num_pix;
				cover_type* covers = dst;

				bool has_inside = _set_outside(x, y, covers, count);
				if (!has_inside)
					return;

				const int8u* mask = m_rbuf->row_ptr(y) + x * Step + Offset;
				do
				{
					*covers = (cover_type)((cover_full + (*covers) * (*mask))
						>> cover_shift);
					++covers;
					mask += Step;
				}
				while(--count);
			}

			void get_hspan(int x, int y, cover_type* dst, int num_pix) const
			{
				int count = num_pix;
				cover_type* covers = dst;

				bool has_inside = _set_outside(x, y, covers, count);
				if (!has_inside)
					return;

				const int8u* mask = m_rbuf->row_ptr(y) + x * Step + Offset;
				memcpy(covers, mask, count);
			}

		private:
			bool _set_outside(int& x, int& y, cover_type*& covers,
				int& count) const
			{
				x -= m_xOffset;
				y -= m_yOffset;

				int xmax = m_rbuf->width() - 1;
				int ymax = m_rbuf->height() - 1;

				int num_pix = count;
				cover_type* dst = covers;

				if(y < 0 || y > ymax)
				{
					memset(dst, m_outside, num_pix * sizeof(cover_type));
					return false;
				}

				if(x < 0)
				{
					count += x;
					if(count <= 0)
					{
						memset(dst, m_outside, num_pix * sizeof(cover_type));
						return false;
					}
					memset(covers, m_outside, -x * sizeof(cover_type));
					covers -= x;
					x = 0;
				}

				if(x + count > xmax)
				{
					int rest = x + count - xmax - 1;
					count -= rest;
					if(count <= 0)
					{
						memset(dst, m_outside, num_pix * sizeof(cover_type));
						return false;
					}
					memset(covers + count, m_outside, rest * sizeof(cover_type));
				}

				return true;
			}


		private:
			int m_xOffset;
			int m_yOffset;

			rendering_buffer* m_rbuf;
			int8u m_outside;

			static const int Step = 1;
			static const int Offset = 0;
	};
}


#endif
