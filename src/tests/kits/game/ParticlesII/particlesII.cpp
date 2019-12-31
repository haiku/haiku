#include <Application.h>
#include <WindowScreen.h>
#include <Screen.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SupportDefs.h> // min_c() and max_c()

#ifdef DEBUGGING
#define PRINT(x) printf x
#else
#define PRINT(x)
#endif

// macros
#define set_pixel(x,y,color) (frame_buffer[x + (line_length*y)] = color)
#define get_pixel(x,y) (frame_buffer[x + (line_length*y)])

class NApplication : public BApplication {
public:
	NApplication();
	bool is_quitting; // So that the WindowScreen knows what
                     //       to do when disconnected.
private:
	bool QuitRequested();
	void ReadyToRun();
};

class NWindowScreen : public BWindowScreen {
public:
	NWindowScreen(status_t*);
private:
	void ScreenConnected(bool);
	int32 MyCode();
	static int32 Entry(void*);
	// handy stuff
	void set_frame_rate(float fps) {frame_pause = (bigtime_t)((1000 * 1000)/fps);}
	// special for demos
	enum {
		// used for star->last_draw
			INVALID	= 0x7fffffff
	};
	typedef struct {
		float init_velocity;
		float gravity;
		double cos_z_theta;
		int32 y;
		int32 x;
		int32 timeval;
		uint32 last_draw;
		int32 lx,ly;
	} particle;
	uint32 particle_count;
	particle *particle_list;
	// simple raster functions
	void draw_line(int x1, int y1, int x2, int y2, int color);
	void draw_rect(int x, int y, int w, int h, int color);
	void fill_rect(int x, int y, int w, int h, int color);
	void draw_ellipse(int cx, int cy, int wide, int deep, int color);
	void fill_ellipse(int x, int y, int xradius, int yradius, int color);
	void ellipse_points(int x, int y, int x_offset, int y_offset, int color);
	void ellipse_fill_points(int x, int y, int x_offset, int y_offset, int color);
	thread_id tid;
	sem_id sem;
	area_id area;
	uint8* save_buffer;
	uint8* frame_buffer;
	ulong line_length;
	bigtime_t frame_pause; // time between frames
	int width,height;
	int COLORS;
	bool thread_is_locked;	// small hack to allow to quit the
							// 	app from ScreenConnected()
};

int
main()
{
	NApplication app;
	return 0;
}

NApplication::NApplication()
      :BApplication("application/x-vnd.Prok-DemoTemplate")
{
	Run(); // see you in ReadyToRun()
}

void NApplication::ReadyToRun()
{
	PRINT(("ReadyToRun()\n"));
	status_t ret = B_ERROR;
	is_quitting = false;
	NWindowScreen *ws = new NWindowScreen(&ret);
	PRINT(("WindowScreen ctor returned. ret = %s\n", strerror(ret)));
	// exit if constructing the WindowScreen failed.
	if((ws == NULL) || (ret < B_OK))
	{
		//printf("the window screen was NULL, or there was an error\n");
		PostMessage(B_QUIT_REQUESTED);
	}
	else
		PRINT(("everything's just peachy. done with ReadyToRun().\n"));
}

bool NApplication::QuitRequested()
{
	PRINT(("QuitRequested()\n"));
	status_t ret;
	is_quitting = true;
	wait_for_thread(find_thread("rendering thread"), &ret); // wait for the render thread to finish	
	return true;
}

NWindowScreen::NWindowScreen(status_t *ret)
	: BWindowScreen("Example", B_8_BIT_640x480, ret), width(639), height(479), COLORS(256)
{
	PRINT(("WindowScreen ctor.\n"));
	thread_is_locked = true;
	tid = 0;
	if(*ret == B_OK)
	{
		PRINT(("creating blocking sem and save_buffer area.\n"));
		// this semaphore controls the access to the WindowScreen
		sem = create_sem(0,"WindowScreen Access");
		// this area is used to save the whole framebuffer when
		//       switching workspaces. (better than malloc()).
		area = create_area("save", (void**)&save_buffer, B_ANY_ADDRESS, 640*480, B_NO_LOCK, B_READ_AREA|B_WRITE_AREA);
		// exit if an error occured.
		if((sem < B_OK) || (area < B_OK))
		{
			PRINT(("create_area() or create_sem() failed\n"));
			*ret = B_ERROR;
		}
		else
		{
			PRINT(("calling Show().\n"));
			Show(); // let's go. See you in ScreenConnected.
		}
	}
	else
	{
		PRINT(("BWindowScreen base class ctor returned failure\n"));
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	// set the frame rate
	set_frame_rate(30.);
}


void
NWindowScreen::ScreenConnected(bool connected)
{
	PRINT(("ScreenConnected()\n"));
	fflush(stdout);
	if(connected)
	{
		if(SetSpace(B_8_BIT_640x480) < B_OK)
		{
			//SetFrameBuffer(640, 480);
			PRINT(("SetSpace() failed\n"));
			// properly set the framebuffer. exit if an error occurs.
			be_app->PostMessage(B_QUIT_REQUESTED);
			return;
		}
		// get the framebuffer-related info, each time the
		// WindowScreen is connected (multiple monitor)
		frame_buffer = (uint8*)(CardInfo()->frame_buffer);
		line_length = FrameBufferInfo()->bytes_per_row;
		if(tid == 0)
		{
			// clean the framebuffer
			PRINT(("zeroing the framebuffer\n"));
			memset(frame_buffer,0,480*line_length);
			// spawn the rendering thread. exit if an error occurs.
			PRINT(("spawning the render thread.\n"));
			tid = spawn_thread(Entry,"rendering thread", B_URGENT_DISPLAY_PRIORITY,this);
			if(resume_thread(tid) < B_OK)
			{
				be_app->PostMessage(B_QUIT_REQUESTED);
				return;
			}
		}
		else
		{
			for(int y=0;y<480;y++)
			{
				// restore the framebuffer when switching back from
				// another workspace.
            	memcpy(frame_buffer+y*line_length,save_buffer+640*y,640);
            }
		}
		// set our color list.
		rgb_color palette[256];
		rgb_color c1;
		for(int i=0,j=0;i<256;i++,j++)
		{
			if(i<64)
			{
				c1.red = j*4; // greys
				c1.green = j*4;
				c1.blue = j*4;
				c1.alpha = 255;
			}
			if((i>=64) && (i<128))
			{
				c1.red = j*4; // reds
				c1.green = 0;
				c1.blue = 0;
				c1.alpha = 255;
			}
			if((i>=128) && (i<192))
			{
				c1.red = 0; // greens
				c1.green = j*4;
				c1.blue = 0;
				c1.alpha = 255;
			}
			if((i>=192) && (i<256))
			{
				c1.red = 0; // blues
				c1.green = 0;
				c1.blue = j*4;
				c1.alpha = 255;
			}
			if(j == 64)
				j=0;
			palette[i]=c1;
		}
		SetColorList(palette);
		
		// allow the rendering thread to run.
		thread_is_locked = false;
		release_sem(sem);
	}
	else /* !connected */
	{
		// block the rendering thread.
		if(!thread_is_locked)
		{
			acquire_sem(sem);
			thread_is_locked = true;
		}
		// kill the rendering and clean up when quitting
		if((((NApplication*)be_app)->is_quitting))
		{
			status_t ret;
			kill_thread(tid);
			wait_for_thread(tid,&ret);
			delete_sem(sem);
			delete_area(area);
			free(particle_list);
		}
		else
		{
			// set the color list black so that the screen doesn't seem
			// to freeze while saving the framebuffer
			rgb_color c={0,0,0,255};
			rgb_color palette[256];
			// build the palette
			for(int i=0;i<256;i++)
				palette[i] = c;
			// set the palette
			SetColorList(palette);
			// save the framebuffer
			for(int y=0;y<480;y++)
				memcpy(save_buffer+640*y,frame_buffer+y*line_length,640);
		}
	}
}


int32
NWindowScreen::Entry(void* p)
{
   return ((NWindowScreen*)p)->MyCode();
}


int32
NWindowScreen::MyCode()
{
	bigtime_t trgt = system_time() + frame_pause;
	srandom(system_time());
	// beforehand stuff
	particle_count = 1024*2;
	particle_list = (particle *)malloc(sizeof(particle)*particle_count);
	for (uint32 i=0; i<particle_count; i++) {
		uint32 rand_max = 0xffffffff;		
		particle_list[i].init_velocity = -((double)((rand_max>>1)+(random()%(rand_max>>1)))/rand_max)*3.333; // magic number
		particle_list[i].gravity = -(((double)((rand_max>>1)+(random()%(rand_max>>1)))/rand_max))*0.599; // more magic
		
		// make the particle initialy invisible and fixed, but at a random moment in time
		particle_list[i].lx = 0;
		particle_list[i].ly = 0;
		particle_list[i].last_draw = INVALID;
		particle_list[i].timeval = random() & 64;
		particle_list[i].x = 0; // this gets figured out at drawtime
		particle_list[i].y = 0; // same here
		particle_list[i].cos_z_theta = cos(random() % 360); // grab an angle
	}
	
	
	// the loop o' fun
	while (!(((NApplication*)be_app)->is_quitting)) {
		// try to sync with the vertical retrace
		if (BScreen(this).WaitForRetrace() != B_OK) {
			// snoze for a bit so that other threads can be happy. 
			// We are realtime priority you know
			if (system_time() < trgt)
				snooze(trgt - system_time());
			trgt = system_time() + frame_pause;
		}
		
		// gain access to the framebuffer before writing to it.
		acquire_sem(sem); // block until we're allowed to own the framebuffer
		
		///////////////////////////////
		// do neat stuff here //
		//////////////////////////////
		PRINT(("rendering a frame.\n"));
	
		
		// eye candy VII - particles! - my own cookin
		int32 x, y, cx,cy;
		set_frame_rate(60.); // woo. ntsc
		// calculate the center
		cx = width/2;
		cy = height/2;
		
		// palette test
		//set_frame_rate(0.1);
		//for(int i=0;i<256;i++)
		//	draw_line(i,0,i,height, i);
		
		PRINT(("Starting particle drawing loop\n"));
		particle *s = particle_list;
		for (uint32 i=0; i<particle_count; i++) {
			PRINT(("drawing particle %d\r", i));
			
			// save the old position
			s->lx = s->x;
			s->ly = s->y;
			
			PRINT(("cx=%d, cy=%d\n", cx,cy));
							
			// move the particle			
			// find y and x
			// (s->gravity/2)*(s->timeval*s->timeval) * 1.85 is magic
			y = s->y = (int32)(cy + (int32)((s->gravity/2)*(s->timeval*s->timeval)*1.94) + ((s->init_velocity - (s->gravity*s->timeval)) * s->timeval));
			x = s->x = (int32)(cx + (int32)(s->timeval * s->cos_z_theta)); // 3d rotation
		
			// interate timeval
			s->timeval++;
		
			// sanity check
			if(x <= 0)
				goto erase_and_reset;
			if(x > width)
				goto erase_and_reset;
			if(y < 0)
				goto erase; // invisible + erase last position
			if(y > height)
				goto erase_and_reset;
					
			// erase the previous position, if necessary
			if (s->last_draw != INVALID)
				set_pixel(s->lx,s->ly,0);
			
			// if it's visible, then draw it.
			set_pixel(s->x,s->y, 169);
			s->last_draw = 1;
			goto loop;
			
			erase_and_reset:
			if((s->lx <= width) && (s->lx >= 0) && (s->ly <= height) && (s->ly >= 0))
				set_pixel(s->lx, s->ly,0);
			s->x = 0;
			s->y = 0;
			s->lx = 0;
			s->ly = 0;
			s->timeval = 0;
			s->last_draw = INVALID;
			goto loop;
			
			erase:
			// erase it.
			if(s->last_draw != INVALID)
				set_pixel(s->lx, s->ly,0);
			s->lx = s->x;
			s->ly = s->y;
			s->last_draw = INVALID;
			loop:
				s++;
			//printf("end draw loop\n");
		}
		PRINT(("frame done\n"));
		
		//////////////////////////////////
		// stop doing neat stuff //
		/////////////////////////////////
		
		// release the semaphore while waiting. gotta release it
		// at some point or nasty things will happen!
		release_sem(sem);
		// loop for another frame!
	}
	return B_OK;
}

//////////////////////////////
// Misc - a place for demos to put their convenience functions
//////////////////////////////


//////////////////////////////
// My Silly Raster Lib 
//////////////////////////////

/*

	Functions:
void draw_line(int x1, int y1, int x2, int y2, int color);
void draw_rect(int x, int y, int w, int h, int color);
void fill_rect(int x, int y, int w, int h, int color);
void draw_ellipse(int x, int y, int xradius, int yradius, int color);
void fill_ellipse(int x, int y, int xradius, int yradius, int color);

*/

void
NWindowScreen::draw_line(int x1, int y1, int x2, int y2, int color)
{
	// Simple Bresenham's line drawing algorithm
	int d,x,y,ax,ay,sx,sy,dx,dy;
	
#define ABS(x) (((x)<0) ? -(x) : (x))
#define SGN(x) (((x)<0) ? -1 : 1)

	dx=x2-x1; ax=ABS(dx)<<1; sx=SGN(dx);
	dy=y2-y1; ay=ABS(dy)<<1; sy=SGN(dy);
	
	x=x1;
	y=y1;
	if(ax>ay)
	{
		d=ay-(ax>>1);
		for(;;)
		{
			set_pixel(x,y,color);
			if(x==x2) return;
			if(d>=0)
			{
				y+=sy;
				d-=ax;
			}
			x+=sx;
			d+=ay;
		}
	}
	else
	{
		d=ax-(ay>>1);
		for(;;)
		{
			set_pixel(x,y,color);
			if(y==y2) return;
			if(d>=0)
			{
				x+=sx;
				d-=ay;
			}
			y+=sy;
			d+=ax;
		}
	}
}


void
NWindowScreen::draw_rect(int x, int y, int w, int h, int color)
{
	draw_line(x,y,x+w,y,color);
	draw_line(x,y,x,y+h,color);
	draw_line(x,y+h,x+w,y+h,color);
	draw_line(x+w,y,x+w,y+h,color);
}


void
NWindowScreen::fill_rect(int x, int y, int w, int h, int color)
{
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			set_pixel(i, j, color);
		}
	}
}


void
NWindowScreen::draw_ellipse(int cx, int cy, int wide, int deep, int color)
{
	// if we're asked to draw a really small ellipse, put a single pixel in the buffer
	// and bail
	if((wide < 1) || (deep < 1))
	{
		set_pixel(cx,cy,color);
		return;
	}
	
	// MidPoint Ellipse algorithm. 
	// page 90 of Computer Graphics Principles and Practice 2nd edition (I highly recommend this book)
	int16 x, y;
	int16 wide_squared, deep_squared;
	double d;

	x = 0;	
	y = deep;
	wide_squared = wide * wide;
	deep_squared = deep * deep;
	d = deep_squared - (wide_squared*deep) + (wide_squared/4);
	
	ellipse_points(x, y, cx, cy, color);
	while((wide_squared*(y - 0.5)) > (deep_squared*(x + 1)))
	{
		if(d < 0)
			d += deep_squared*(2*x + 3);
		else
		{
			d += deep_squared*(2*x + 3) + wide_squared*(-2*y + 2);
			y--;
		}
		x++;
		ellipse_points(x, y, cx, cy, color);
	}
	
	d = deep_squared*((x+0.5)*(x+0.5)) + wide_squared*((y-1)*(y-1)) - deep_squared*wide_squared;
	while(y > 0)
	{
		if(d < 0)
		{
			d += deep_squared*(2*x + 2) + wide_squared*(-2*y + 3);
			x++;
		}
		else
			d += wide_squared*(-2*y + 3);
		y--;
		ellipse_points(x, y, cx, cy, color);
	}
}


void
NWindowScreen::fill_ellipse(int cx, int cy, int wide, int deep, int color)
{
	// if we're asked to draw a really small ellipse, put a single pixel in the buffer
	// and bail
	if((wide < 1) || (deep < 1))
	{
		set_pixel(cx,cy,color);
		return;
	}
	
	// MidPoint Ellipse algorithm. 
	// page 90 of Computer Graphics Principles and Practice 2nd edition (I highly recommend this book)
	int16 x, y;
	int16 wide_squared, deep_squared;
	double d;

	x = 0;	
	y = deep;
	wide_squared = wide * wide;
	deep_squared = deep * deep;
	d = deep_squared - (wide_squared*deep) + (wide_squared/4);
	
	ellipse_fill_points(x, y, cx, cy, color);
	while((wide_squared*(y - 0.5)) > (deep_squared*(x + 1)))
	{
		if(d < 0)
			d += deep_squared*(2*x + 3);
		else
		{
			d += deep_squared*(2*x + 3) + wide_squared*(-2*y + 2);
			y--;
		}
		x++;
		ellipse_fill_points(x, y, cx, cy, color);
	}
	
	d = deep_squared*((x+0.5)*(x+0.5)) + wide_squared*((y-1)*(y-1)) - deep_squared*wide_squared;
	while(y > 0)
	{
		if(d < 0)
		{
			d += deep_squared*(2*x + 2) + wide_squared*(-2*y + 3);
			x++;
		}
		else
			d += wide_squared*(-2*y + 3);
		y--;
		ellipse_fill_points(x, y, cx, cy, color);
	}
}


void
NWindowScreen::ellipse_points(int x, int y, int x_offset, int y_offset, int color)
{
	// fill four pixels for every iteration in draw_ellipse
	
	// the x_offset and y_offset values are needed since the midpoint ellipse algorithm 
	// assumes the midpoint to be at the origin
	// do a sanity check before each set_pixel, that way we clip to the edges
	
	int xCoord, yCoord;

	xCoord = x_offset + x;
	yCoord = y_offset + y;
	if((xCoord > 0) && (yCoord > 0) && (xCoord < width) && (yCoord < height))
		set_pixel(xCoord,yCoord,color);
	xCoord = x_offset - x;
	if((xCoord > 0) && (yCoord > 0) && (xCoord < width) && (yCoord < height))
		set_pixel(xCoord,yCoord,color);
	xCoord = x_offset + x;
	yCoord = y_offset - y;
	if((xCoord > 0) && (yCoord > 0) && (xCoord < width) && (yCoord < height))
		set_pixel(xCoord,yCoord,color);
	xCoord = x_offset - x;
	if((xCoord > 0) && (yCoord > 0) && (xCoord < width) && (yCoord < height))
		set_pixel(xCoord,yCoord,color);
}


void
NWindowScreen::ellipse_fill_points(int x, int y, int x_offset, int y_offset, int color)
{
	// put lines between two pixels twice. once for y positive, the other for y negative (symmetry)
	// for every iteration in fill_ellipse
	
	// the x_offset and y_offset values are needed since the midpoint ellipse algorithm 
	// assumes the midpoint to be at the origin
	// do a sanity check before each set_pixel, that way we clip to the edges
	
	int xCoord1, yCoord1;
	int xCoord2, yCoord2;

	xCoord1 = x_offset - x;
	yCoord1 = y_offset + y;
	xCoord2 = x_offset + x;
	yCoord2 = y_offset + y;
	if((xCoord1 > 0) && (yCoord1 > 0) && (xCoord1 < width) && (yCoord1 < height))	
		if((xCoord2 > 0) && (yCoord2 > 0) && (xCoord2 < width) && (yCoord2 < height))
			draw_line(xCoord1,yCoord1,xCoord2,yCoord2,color);
	
	xCoord1 = x_offset - x;
	yCoord1 = y_offset - y;
	xCoord2 = x_offset + x;
	yCoord2 = y_offset - y;
	if((xCoord1 > 0) && (yCoord1 > 0) && (xCoord1 < width) && (yCoord1 < height))	
		if((xCoord2 > 0) && (yCoord2 > 0) && (xCoord2 < width) && (yCoord2 < height))
			draw_line(xCoord1,yCoord1,xCoord2,yCoord2,color);

}
