/*

	StarWindow.cpp

	by Pierre Raynaud-Richard.

*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Application.h>

#include "StarWindow.h"
#include "Stars.h"

#include <string.h>
#include <stdlib.h>

#include <AppFileInfo.h>
#include <FindDirectory.h>
#include <Alert.h>
#include <File.h>
#include <Path.h>

#include <Debug.h>

// return the version_info of a file, described by its
// name and its generic folder (in find_directory syntax).
status_t get_file_version_info(	directory_which	dir,
								char			*filename,
								version_info	*info) {
	BPath 			path;
	BFile			file;
	status_t		res;
	BAppFileInfo	appinfo;

	// find the directory
	if ((res = find_directory(dir, &path)) != B_NO_ERROR)
		return res;

	// find the file
	path.Append(filename);
	file.SetTo(path.Path(), O_RDONLY);
	if ((res = file.InitCheck()) != B_NO_ERROR)
		return res;

	// get the version_info
	if ((res = appinfo.SetTo(&file)) != B_NO_ERROR)
		return res;
	return appinfo.GetVersionInfo(info, B_APP_VERSION_KIND);
}

enum {
	// pseudo-random generator parameters (not very good ones,
	// but good enough for what we do here).
	CRC_START		= 0x56dec231,
	CRC_KEY			= 0x1789feb3
};


StarWindow::StarWindow(BRect frame, const char *name)
	: BDirectWindow(frame, name, B_TITLED_WINDOW, 0)
{
	uint32			i;
	int32			x, y, dx, dy, cnt, square;

	// init the crc pseudo-random generator
	crc_alea = CRC_START;

	// allocate the star struct array
	star_count_max = 8192;
	star_count = 0;
	star_list = (star*)malloc(sizeof(star)*star_count_max);

	// initialise the default state of the star array
	for (i = 0; i < star_count_max; i++) {
		// peek a random vector. This is certainly not the nicest way
		// to do it (the probability and the angle are linked), but that's
		// simple and doesn't require any trigonometry.
		do {
			dx = (crc_alea&0xffff) - 0x8000;
			CrcStep();
			CrcStep();

			dy = (crc_alea&0xffff) - 0x8000;
			CrcStep();
			CrcStep();
		} while ((dx == 0) && (dy == 0));

		// enforce a minimal length by doubling the vector as many times
		// as needed.
		square = dx*dx+dy*dy;
		while (square < 0x08000000) {
			dx <<= 1;
			dy <<= 1;
			square <<= 2;
		}

		// save the starting speed vector.
		star_list[i].dx0 = dx;
		star_list[i].dy0 = dy;

		// simulate the animation to see how many moves are needed to
		// get out by at least 1024 in one direction. That will give us
		// an minimal value for how long we should wait before restarting
		// the animation. It wouldn't work if the window was getting
		// much larger than 2048 pixels in one dimension.
		cnt = 0;
		x = 0;
		y = 0;
		while ((x<0x4000000) && (x>-0x4000000) && (y<0x4000000) && (y>-0x4000000)) {
			x += dx;
			y += dy;
			dx += (dx>>4);
			dy += (dy>>4);
			cnt++;
		}

		// add a random compenent [0 to 15] to the minimal count before
		// restart.
		star_list[i].count0 = cnt + ((crc_alea&0xf0000)>>16);
		// make the star initialy invisible and fixed, then spread the
		// value of their restart countdown so that they won't start all
		// at the same time, but progressively
		star_list[i].last_draw = INVALID;
		star_list[i].x = 0x40000000;
		star_list[i].y = 0x40000000;
		star_list[i].dx = 0;
		star_list[i].dy = 0;
		star_list[i].count = (i&255);
	}

	// allocate the semaphore used to synchronise the star animation drawing access.
	drawing_lock = create_sem(0, "star locker");

	// spawn the star animation thread (we have better set the force quit flag to
	// false first).
	kill_my_thread = false;
	my_thread = spawn_thread(StarWindow::StarAnimation, "StarAnimation",
							 B_DISPLAY_PRIORITY, (void*)this);
	resume_thread(my_thread);

	// add a view in the background to insure that the content area will
	// be properly erased in black. This erase mechanism is not synchronised
	// with the star animaton, which means that from time to time, some
	// stars will be erreneously erased by the view redraw. But as every
	// single star is erased and redraw every frame, that graphic glitch
	// will last less than a frame, and that just in the area being redraw
	// because of a window resize, move... Which means the glitch won't
	// be noticeable. The other solution for erasing the background would
	// have been to do it on our own (which means some serious region
	// calculation and handling all color_space). Better to use the kit
	// to do it, as it gives us access to hardware acceleration...
	frame.OffsetTo(0.0, 0.0);
	//view = new BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW);

	// The only think we want from the view mechanism is to
	// erase the background in black. Because of the way the
	// star animation is done, this erasing operation doesn't
	// need to be synchronous with the animation. That the
	// reason why we use both the direct access and the view
	// mechanism to draw in the same area of the StarWindow.
	// Such thing is usualy not recommended as synchronisation
	// is generally an issue (drawing in random order usualy
	// gives remanent incorrect result).
	// set the view color to be black (nicer update).
	//view->SetViewColor(0, 0, 0);
	//AddChild(view);

	// Add a shortcut to switch in and out of fullscreen mode.
	AddShortcut('f', B_COMMAND_KEY, new BMessage('full'));

	// As we said before, the window shouldn't get wider than 2048 in any
	// direction, so those limits will do.
	SetSizeLimits(40.0, 2000.0, 40.0, 2000.0);

	// If the graphic card/graphic driver we use doesn't support directwindow
	// in window mode, then we need to switch to fullscreen immediately, or
	// the user won't see anything, as long as it doesn't used the undocumented
	// shortcut. That would be bad behavior...
	if (!BDirectWindow::SupportsWindowMode()) {
		bool		sSwapped;
		char		*buf;
		BAlert		*quit_alert;

		key_map *map;
		get_key_map(&map, &buf);

		if (map != NULL) {
			sSwapped = (map->left_control_key == 0x5d)
				&& (map->left_command_key == 0x5c);
		} else
			sSwapped = false;

		free(map);
		free(buf);
		quit_alert = new BAlert("QuitAlert", sSwapped ?
		                        "This demo runs only in full screen mode.\n"
		                        "While running, press 'Ctrl-Q' to quit.":
		                        "This demo runs only in full screen mode.\n"
		                        "While running, press 'Alt-Q' to quit.",
		                        "Quit", "Start demo", NULL,
		                        B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		if (quit_alert->Go() == 0)
			((StarsApp*)be_app)->abort_required = true;
		else
			SetFullScreen(true);
	}
}


StarWindow::~StarWindow()
{
	// force the drawing_thread to quit. This is the easiest way to deal
	// with potential closing problem. When it's not practical, we
	// recommand to use Hide() and Sync() to force the disconnection of
	// the direct access, and use some other flag to guarantee that your
	// drawing thread won't draw anymore. After that, you can pursue the
	// window destructor and kill your drawing thread...
	kill_my_thread = true;
	delete_sem(drawing_lock);

	status_t result;
	wait_for_thread(my_thread, &result);

	// Free window resources. As they're used by the drawing thread, we
	// need to terminate that thread before freeing them, or we could crash.
	free(star_list);
}


bool
StarWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
StarWindow::MessageReceived(BMessage *message)
{
	int8 key_code;

	switch (message->what) {
		// Switch between full-screen mode and windowed mode.
		case 'full':
			SetFullScreen(!IsFullScreen());
			break;
		case B_KEY_DOWN:
			if (!IsFullScreen())
				break;
			if (message->FindInt8("byte", &key_code) != B_OK)
				break;
			if (key_code == B_ESCAPE)
				PostMessage(B_QUIT_REQUESTED);
			break;
		default:
			BDirectWindow::MessageReceived(message);
			break;
	}
}


void
StarWindow::CrcStep()
{
	// basic crc pseudo-random generator
	crc_alea <<= 1;
	if (crc_alea < 0)
		crc_alea ^= CRC_KEY;
}


void
StarWindow::DirectConnected(direct_buffer_info *info)
{
	// you need to use that mask to read the buffer state.
	switch (info->buffer_state & B_DIRECT_MODE_MASK) {
		// start a direct screen connection.
		case B_DIRECT_START:
			SwitchContext(info);		// update the direct screen infos.
			release_sem(drawing_lock);	// unblock the animation thread.
			break;
		// stop a direct screen connection.
		case B_DIRECT_STOP:
			acquire_sem(drawing_lock);	// block the animation thread.
			break;
		// modify the state of a direct screen connection.
		case B_DIRECT_MODIFY:
			acquire_sem(drawing_lock);	// block the animation thread.
			SwitchContext(info);		// update the direct screen infos.
			release_sem(drawing_lock);	// unblock the animation thread.
			break;

		default:
			break;
	}
}


// This function update the internal graphic context of the StarWindow
// object to reflect the infos send through the DirectConnected API.
// It also update the state of stars (and erase some of them) to
// insure a clean transition during resize. As this function is called
// in DirectConnected, it's a bad idea to do any heavy drawing (long)
// operation. But as we only update the stars (the background will be
// updated a little later by the view system), it's not a big deal.
void
StarWindow::SwitchContext(direct_buffer_info *info)
{
	star			*s;
	int32			x, y, deltax, deltay;
	uint32			i, j, window_area, cx, cy;
	uint32			star_count_new;
	clipping_rect	*r;

	// calculate the new star count, depending the size of the window frame.
	// we do that because we want to keep the star count proportionnal to
	// the size of the window, to keep an similar overall star density feeling
	window_area = (info->window_bounds.right-info->window_bounds.left+1)*
				  (info->window_bounds.bottom-info->window_bounds.top+1);
	// max out beyond 1M pixels.
	if (window_area > (1<<20))
		window_area = (1<<20);
	star_count_new = (star_count_max*(window_area>>10))>>10;
	if (star_count_new > star_count_max)
		star_count_new = star_count_max;

	// set the position of the new center of the window (in case of move or resize)
	cx = (info->window_bounds.right+info->window_bounds.left+1)/2;
	cy = (info->window_bounds.bottom+info->window_bounds.top+1)/2;

	// update to the new clipping region. The local copy is kept relative
	// to the center of the animation (origin of the star coordinate).
	clipping_bound.left = info->clip_bounds.left - cx;
	clipping_bound.right = info->clip_bounds.right - cx;
	clipping_bound.top = info->clip_bounds.top - cy;
	clipping_bound.bottom = info->clip_bounds.bottom - cy;
	// the clipping count is bounded (see comment in header file).
	clipping_list_count = info->clip_list_count;
	if (clipping_list_count > MAX_CLIPPING_RECT_COUNT)
		clipping_list_count = MAX_CLIPPING_RECT_COUNT;
	for (i=0; i<clipping_list_count; i++) {
		clipping_list[i].left = info->clip_list[i].left - cx;
		clipping_list[i].right = info->clip_list[i].right - cx;
		clipping_list[i].top = info->clip_list[i].top - cy;
		clipping_list[i].bottom = info->clip_list[i].bottom - cy;
	}

	// update the new rowbyte
	// NOTE: "row_bytes" is completely misnamed, and was misused too
	row_bytes = info->bytes_per_row / (info->bits_per_pixel / 8);

	// update the screen bases (only one of the 3 will be really used).
	draw_ptr8 = (uint8*)info->bits + info->bytes_per_row
		* info->window_bounds.top + info->window_bounds.left
		* (info->bits_per_pixel / 8);
		// Note: parenthesis around "info->bits_per_pixel / 8"
		// are needed to avoid an overflow when info->window_bounds.left
		// becomes negative.

	draw_ptr16 = (uint16*)draw_ptr8;
	draw_ptr32 = (uint32*)draw_ptr8;

	// cancel the erasing of all stars if the buffer has been reset.
	// Because of a bug in the R3 direct window protocol, B_BUFFER_RESET is not set
	// whew showing a previously hidden window. The second test is a reasonnable
	// way to work around that bug...
	if ((info->buffer_state & B_BUFFER_RESET) ||
		(need_r3_buffer_reset_work_around &&
		 ((info->buffer_state & (B_DIRECT_MODE_MASK|B_BUFFER_MOVED)) == B_DIRECT_START))) {
		s = star_list;
		for (i=0; i<star_count_max; i++) {
			s->last_draw = INVALID;
			s++;
		}
	}
	// in the other case, update the stars that will stay visible.
	else {
		// calculate the delta vector due to window resize or move.
		deltax = cx_old - (cx - info->window_bounds.left);
		deltay = cy_old - (cy - info->window_bounds.top);
		// check all the stars previously used.
		s = star_list;
		for (i=0; i<star_count; i++) {
			// if the star wasn't visible before, then no more question.
			if (s->last_draw == INVALID)
				goto not_defined;
			// convert the old position into the new referential.
			x = (s->x>>16) + deltax;
			y = (s->y>>16) + deltay;
			// check if the old position is still visible in the new clipping
			if ((x < clipping_bound.left) || (x > clipping_bound.right) ||
				(y < clipping_bound.top) || (y > clipping_bound.bottom))
				goto invisible;
			if (clipping_list_count == 1)
				goto visible;
			r = clipping_list;
			for (j=0; j<clipping_list_count; j++) {
				if ((x >= r->left) && (x <= r->right) &&
					(y >= r->top) && (y <= r->bottom))
					goto visible;
				r++;
			}
			goto invisible;

			// if it's still visible...
		visible:
			if (i >= star_count_new) {
				// ...and the star won't be used anylonger, then we erase it.
				if (pixel_depth == 32)
					draw_ptr32[s->last_draw] = 0;
				else if (pixel_depth == 16)
					draw_ptr16[s->last_draw] = 0;
				else
					draw_ptr8[s->last_draw] = 0;
			}
			goto not_defined;

			// if the star just became invisible and it was because the
			// context was modified and not fully stop, then we need to erase
			// those stars who just became invisible (or they could leave
			// artefacts in the drawing area in some cases). This problem is
			// a side effect of the interaction between a complex resizing
			// case (using 2 B_DIRECT_MODIFY per step), and the dynamic
			// star count management we are doing. In most case, you never
			// have to erase things going out of the clipping region...
		invisible:
			if ((info->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_MODIFY) {
				if (pixel_depth == 32)
					draw_ptr32[s->last_draw] = 0;
				else if (pixel_depth == 16)
					draw_ptr16[s->last_draw] = 0;
				else
					draw_ptr8[s->last_draw] = 0;
			}
			// and set its last position as invalid.
			s->last_draw = INVALID;
		not_defined:
			s++;
		}

		// initialise all the new star (the ones which weren't used but
		// will be use after that context update) to set their last position
		// as invalid.
		s = star_list+star_count;
		for (i=star_count; i<star_count_new; i++) {
			s->last_draw = INVALID;
			s++;
		}
	}

	// update the window origin offset.
	window_offset = row_bytes*(cy-info->window_bounds.top) + (cx-info->window_bounds.left);

	// set the pixel_depth and the pixel data, from the color_space.
	switch (info->pixel_format) {
	case B_RGBA32 :
	case B_RGB32 :
		pixel_depth = 32;
		((uint8*)&pixel32)[0] = 0x20;
		((uint8*)&pixel32)[1] = 0xff;
		((uint8*)&pixel32)[2] = 0x20;
		((uint8*)&pixel32)[3] = 0xff;
		break;
	case B_RGB16 :
		pixel_depth = 16;
		((uint8*)&pixel16)[0] = 0xe0;
		((uint8*)&pixel16)[1] = 0x07;
		break;
	case B_RGB15 :
	case B_RGBA15 :
		pixel_depth = 16;
		((uint8*)&pixel16)[0] = 0xe0;
		((uint8*)&pixel16)[1] = 0x03;
		break;
	case B_CMAP8 :
		pixel_depth = 8;
		pixel8 = 52;
		break;
	case B_RGBA32_BIG :
	case B_RGB32_BIG :
		pixel_depth = 32;
		((uint8*)&pixel32)[3] = 0x20;
		((uint8*)&pixel32)[2] = 0xff;
		((uint8*)&pixel32)[1] = 0x20;
		((uint8*)&pixel32)[0] = 0xff;
		break;
	case B_RGB16_BIG :
		pixel_depth = 16;
		((uint8*)&pixel16)[1] = 0xe0;
		((uint8*)&pixel16)[0] = 0x07;
		break;
	case B_RGB15_BIG :
	case B_RGBA15_BIG :
		pixel_depth = 16;
		((uint8*)&pixel16)[1] = 0xe0;
		((uint8*)&pixel16)[0] = 0x03;
		break;
	default:	// unsupported color space?
		fprintf(stderr, "ERROR - unsupported color space!\n");
		exit(1);
		break;
	}

	// set the new star count.
	star_count = star_count_new;

	// save a copy of the variables used to calculate the move of the window center
	cx_old = cx - info->window_bounds.left;
	cy_old = cy - info->window_bounds.top;
}


// This is the thread doing the star animation itself. It would be easy to
// adapt to do any other sort of pixel animation.
status_t
StarWindow::StarAnimation(void *data)
{
	star			*s;
	int32			x, y;
	uint32			i, j;
	bigtime_t		time;
	StarWindow		*w;
	clipping_rect	*r;

	// receive a pointer to the StarWindow object.
	w = (StarWindow*)data;

	// loop, frame after frame, until asked to quit.
	while (!w->kill_my_thread) {
		// we want a frame to take at least 16 ms.
		time = system_time()+16000;

		// get the right to do direct screen access.
		while (acquire_sem(w->drawing_lock) == B_INTERRUPTED)
			;
		if (w->kill_my_thread)
			break;

		// go through the array of star, for all currently used star.
		s = w->star_list;
		for (i=0; i<w->star_count; i++) {
			if (s->count == 0) {
				// restart the star animation, from a random point close to
				// the center [-16, +15], both axis.
				x = s->x = ((w->crc_alea&0x1f00)>>8) - 16;
				y = s->y = ((w->crc_alea&0x1f0000)>>16) - 16;
				s->dx = s->dx0;
				s->dy = s->dy0;
				// add a small random component to the duration of the star
				s->count = s->count0 + (w->crc_alea&0x7);
				w->CrcStep();
			}
			else {
				// just move the star
				s->count--;
				x = s->x += s->dx;
				y = s->y += s->dy;
				s->dx += (s->dx>>4);
				s->dy += (s->dy>>4);
			}
			// erase the previous position, if necessary
			if (s->last_draw != INVALID) {
				if (w->pixel_depth == 32)
					w->draw_ptr32[s->last_draw] = 0;
				else if (w->pixel_depth == 16)
					w->draw_ptr16[s->last_draw] = 0;
				else
					w->draw_ptr8[s->last_draw] = 0;
			}
			// check if the new position is visible in the current clipping
			x >>= 16;
			y >>= 16;
			if ((x < w->clipping_bound.left) || (x > w->clipping_bound.right) ||
				(y < w->clipping_bound.top) || (y > w->clipping_bound.bottom))
				goto invisible;
			if (w->clipping_list_count == 1) {
		visible:
				// if it's visible, then draw it.
				s->last_draw = w->window_offset + w->row_bytes*y + x;
				if (w->pixel_depth == 32)
					w->draw_ptr32[s->last_draw] = w->pixel32;
				else if (w->pixel_depth == 16)
					w->draw_ptr16[s->last_draw] = w->pixel16;
				else
					w->draw_ptr8[s->last_draw] = w->pixel8;
				goto loop;
			}
			// handle complex clipping cases
			r = w->clipping_list;
			for (j=0; j<w->clipping_list_count; j++) {
				if ((x >= r->left) && (x <= r->right) &&
					(y >= r->top) && (y <= r->bottom))
					goto visible;
				r++;
			}
		invisible:
			// if not visible, register the fact that the star wasn't draw.
			s->last_draw = INVALID;
		loop:
			s++;
		}

		// release the direct screen access
		release_sem(w->drawing_lock);

		// snooze for whatever time is left from the initial allocation done
		// at the beginning of the loop.
		time -= system_time();
		if (time > 0)
			snooze(time);
	}
	return 0;
}

