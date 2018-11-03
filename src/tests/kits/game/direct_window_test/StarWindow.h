/*

	StarWindow.h

	by Pierre Raynaud-Richard.

*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef STAR_WINDOW_H
#define STAR_WINDOW_H

#include <DirectWindow.h>
#include <OS.h>


class StarWindow : public BDirectWindow {

public:
		// standard constructor and destrcutor
				StarWindow(BRect frame, const char *name);
virtual			~StarWindow();

		// standard window member
virtual	bool	QuitRequested();
virtual	void	MessageReceived(BMessage *message);

		// this is the hook controling direct screen connection
virtual void	DirectConnected(direct_buffer_info *info);



private:
		// this function is used to inforce a direct access context
		// modification.
		void	SwitchContext(direct_buffer_info *info);

		// function used to increment the pseudo_random generator
inline	void	CrcStep();

		// the drawing thread function.
static	status_t	StarAnimation(void *data);

		// struct used to control each star.
		typedef struct {
			// current position (fixed point 16:16).
			int32		x;
			int32		y;
			// current speed vector (16:16).
			int32		dx;
			int32		dy;
			// starting speed vector, define only at init time (16:16).
			int32		dx0;
			int32		dy0;
			// how many moves left before restarting from the center.
			uint16		count;
			// minimal moves count between two restarts from the center.
			uint16		count0;
			// offset from the base of the buffer, in pixel, of the
			// last drawn position of the star, or INVALID if not
			// drawn.
			uint32		last_draw;
		} star;


		enum {
		// used for star->last_draw
			INVALID					= 0xffffffff,
		// used for the local copy of the clipping region
			MAX_CLIPPING_RECT_COUNT = 64
		};


		// flag used to force the drawing thread to quit.
		bool				kill_my_thread;
		// the drawing thread doing the star animation.
		thread_id			my_thread;
		// used to synchronise the star animation drawing.
		sem_id				drawing_lock;


		// array used for star animation.
		star				*star_list;
		// count of star currently animated.
		uint32				star_count;
		// maximal count of star in the array.
		uint32				star_count_max;


		// the pixel drawing can be done in 3 depth : 8, 16 or 32.
		int32				pixel_depth;

		// base pointer of the screen, one per pixel_depth
		uint8				*draw_ptr8;
		uint16				*draw_ptr16;
		uint32				*draw_ptr32;

		// pixel data, one per pixel_depth (for the same pixel_depth,
		// the value depends of the color and the specific color encoding).
		uint8				pixel8;
		uint16				pixel16;
		uint32				pixel32;

		// offset, in bytes, between two lines of the frame buffer.
		uint32				row_bytes;

		// offset, in bytes, between the base of the screen and the base
		// of the content area of the window (top left corner).
		uint32				window_offset;

		// clipping region, defined as a list of rectangle, including the
		// smaller possible bounding box. This application will draw only
		// in the 64 first rectangles of the region. Region more complex
		// than that won't be completly used. This is a valid approximation
		// with the current clipping architecture. If a region more complex
		// than that is passed to your application, that just means that
		// the user is doing something really, really weird.
		clipping_rect		clipping_bound;
		clipping_rect		clipping_list[MAX_CLIPPING_RECT_COUNT];
		uint32				clipping_list_count;

		// used for dynamic resizing.
		int32				cx_old, cy_old;

		// pseudo-random generator base
		int32				crc_alea;

		// enable/disable a work around needed for R3.0
		bool				need_r3_buffer_reset_work_around;
};

#endif
