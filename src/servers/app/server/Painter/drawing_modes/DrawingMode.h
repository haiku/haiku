// DrawingMode.h

#ifndef DRAWING_MODE_H
#define DRAWING_MODE_H

#include <stdio.h>
#include <string.h>
#include <agg_basics.h>
#include <agg_color_rgba8.h>
#include <agg_rendering_buffer.h>

class PatternHandler;

namespace agg
{
	//====================================================DrawingMode
	class DrawingMode
	{
	public:
		typedef rgba8 color_type;

		//--------------------------------------------------------------------
		DrawingMode()
			: m_rbuf(NULL)
		{
		}

		//--------------------------------------------------------------------
		void set_rendering_buffer(rendering_buffer* rbuf)
		{
			m_rbuf = rbuf;
		}
		//--------------------------------------------------------------------
		void set_pattern_handler(const PatternHandler* handler)
		{
			fPatternHandler = handler;
		}

		//--------------------------------------------------------------------
		virtual	void blend_pixel(int x, int y, const color_type& c, int8u cover)
		{
printf("DrawingMode::blend_pixel()\n");
		}

		//--------------------------------------------------------------------
		virtual	void blend_hline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
printf("DrawingMode::blend_hline()\n");
		}

		//--------------------------------------------------------------------
		virtual	void blend_vline(int x, int y, unsigned len, 
								 const color_type& c, int8u cover)
		{
printf("DrawingMode::blend_vline()\n");
		}

		//--------------------------------------------------------------------
		virtual	void blend_solid_hspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
//################################
printf("DrawingMode::blend_solid_hspan()\n");
		}



		//--------------------------------------------------------------------
		virtual	void blend_solid_vspan(int x, int y, unsigned len, 
									   const color_type& c, const int8u* covers)
		{
//################################
printf("DrawingMode::blend_solid_vspan()\n");
		}


		//--------------------------------------------------------------------
		virtual	void blend_color_hspan(int x, int y, unsigned len, 
									   const color_type* colors, 
									   const int8u* covers,
									   int8u cover)
		{
printf("DrawingMode::blend_color_hspan()\n");
		}


		//--------------------------------------------------------------------
		virtual	void blend_color_vspan(int x, int y, unsigned len, 
									   const color_type* colors, 
									   const int8u* covers,
									   int8u cover)
		{
printf("DrawingMode::blend_color_vspan()\n");
		}

	protected:
		rendering_buffer*		m_rbuf;
		const PatternHandler*	fPatternHandler;
		

	};

}

#endif // DRAWING_MODE_H

