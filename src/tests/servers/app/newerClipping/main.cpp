
#include <Application.h>
#include <DirectWindow.h>
#include <View.h>

#include <stdio.h>
#include <stdlib.h>

#include "AccelerantHWInterface.h"
#include "Desktop.h"
#include "DirectWindowBuffer.h"
#include "DrawingEngine.h"
#include "DrawView.h"
#include "ViewLayer.h"
#include "WindowLayer.h"

class App : public BApplication {
 public:
						App();

	virtual void		ReadyToRun();
};

class Window : public BDirectWindow {
 public:
								Window(const char* title);
	virtual						~Window();

	virtual	bool				QuitRequested();

	virtual void				DirectConnected(direct_buffer_info* info);

			void				AddWindow(BRect frame, const char* name);
			void				Test();
 private:
		DrawView*				fView;
		Desktop*				fDesktop;
		bool					fQuit;
		DirectWindowBuffer		fBuffer;
		AccelerantHWInterface	fInterface;
		DrawingEngine			fEngine;

};

// constructor
App::App()
	: BApplication("application/x-vnd.stippi.ClippingTest")
{
	srand(real_time_clock_usecs());
}

// ReadyToRun
void
App::ReadyToRun()
{
	Window* win = new Window("clipping");
	win->Show();

//	win->Test();
}

// constructor
Window::Window(const char* title)
	: BDirectWindow(BRect(50, 50, 800, 650), title,
					B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS),
	  fQuit(false),
	  fBuffer(),
	  fInterface(),
	  fEngine(&fInterface, &fBuffer)
{
	fInterface.Initialize();
	fView = new DrawView(Bounds());
	AddChild(fView);
	fView->MakeFocus(true);

	fDesktop = new Desktop(fView, &fEngine);
	fDesktop->Run();
}

// destructor
Window::~Window()
{
	fInterface.Shutdown();
	fDesktop->Lock();
	fDesktop->Quit();
}

// QuitRequested
bool
Window::QuitRequested()
{
/*	if (!fQuit) {
		fDesktop->PostMessage(MSG_QUIT);
		fQuit = true;
		return false;
	}*/
be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

// #pragma mark -

void
fill_line_8(uint8* buffer, int32 pixels, uint8 r, uint8 g, uint8 b)
{
	for (int32 i = 0; i < pixels; i++) {
		*buffer++ = b;
		*buffer++ = g;
		*buffer++ = r;
		*buffer++ = 255;
	}
}

void
fill_line_32(uint8* buffer, int32 pixels, uint8 r, uint8 g, uint8 b)
{
	uint32 pixel;
	uint32* handle = (uint32*)buffer;
	for (int32 i = 0; i < pixels; i++) {
		pixel = 0xff000000 | (r << 16) | (g << 8) | (b);
		*handle++ = pixel;
	}
}

void
fill_line_64(uint8* buffer, int32 pixels, uint8 r, uint8 g, uint8 b)
{
	uint64 pixel;
	uint64* handle = (uint64*)buffer;
	pixels /= 2;
	for (int32 i = 0; i < pixels; i++) {
		pixel = 0xff000000 | (r << 16) | (g << 8) | (b);
		pixel = pixel << 32;
		pixel |= 0xff000000 | (r << 16) | (g << 8) | (b);
		*handle++ = pixel;
	}
}

void
test1(uint8* buffer, uint32 bpr)
{
	uint8* handle = buffer;

/*	bigtime_t start8 = system_time();
	for (int32 x = 0; x < 1000; x++) {
		handle = buffer;
		for (int32 i = 0; i < 64; i++) {
			fill_line_8(handle, 512, 255, 0, 255);
			handle += bpr;
		}
	}

	bigtime_t start32 = system_time();
	for (int32 x = 0; x < 1000; x++) {
		handle = buffer;
		for (int32 i = 0; i < 64; i++) {
			fill_line_32(handle, 512, 255, 0, 255);
			handle += bpr;
		}
	}

	bigtime_t start64 = system_time();
	for (int32 x = 0; x < 1000; x++) {*/
		handle = buffer;
		for (int32 i = 0; i < 640; i++) {
			fill_line_64(handle, 512, 0, 255, 255);
			handle += bpr;
		}
/*	}


	bigtime_t finish = system_time();
	printf("8:  %lld\n", start32 - start8);
	printf("32: %lld\n", start64 - start32);
	printf("64: %lld\n", finish - start64);*/
}
	
// #pragma mark -

void
blend_line_8(uint8* buffer, int32 pixels, uint8 r, uint8 g, uint8 b, uint8 a)
{
	for (int32 i = 0; i < pixels; i++) {
		buffer[0] = ((b - buffer[0]) * a + (buffer[0] << 8)) >> 8;
		buffer[1] = ((g - buffer[1]) * a + (buffer[1] << 8)) >> 8;
		buffer[2] = ((r - buffer[2]) * a + (buffer[2] << 8)) >> 8;;
		buffer[3] = a;
		buffer += 4;
	}
}

union pixel {
	uint32	data32;
	uint8	data8[4];
};

void
blend_line_32(uint8* buffer, int32 pixels, uint8 r, uint8 g, uint8 b, uint8 a)
{
	pixel p;
	pixels /= 2;
	for (int32 i = 0; i < pixels; i++) {
		p.data32 = *(uint32*)buffer;

		p.data8[0] = ((b - p.data8[0]) * a + (p.data8[0] << 8)) >> 8;
		p.data8[1] = ((g - p.data8[1]) * a + (p.data8[1] << 8)) >> 8;
		p.data8[2] = ((r - p.data8[2]) * a + (p.data8[2] << 8)) >> 8;
		p.data8[3] = 255;
		*((uint32*)buffer) = p.data32;
		buffer += 4;

		p.data32 = *(uint32*)buffer;

		p.data8[0] = ((b - p.data8[0]) * a + (p.data8[0] << 8)) >> 8;
		p.data8[1] = ((g - p.data8[1]) * a + (p.data8[1] << 8)) >> 8;
		p.data8[2] = ((r - p.data8[2]) * a + (p.data8[2] << 8)) >> 8;
		p.data8[3] = 255;
		*((uint32*)buffer) = p.data32;
		buffer += 4;
	}
}

union pixel2 {
	uint64	data64;
	uint8	data8[8];
};

// gfxcpy32
// * numBytes is expected to be a multiple of 4
inline
void
gfxcpy32(uint8* dst, uint8* src, int32 numBytes)
{
	uint64* d64 = (uint64*)dst;
	uint64* s64 = (uint64*)src;
	int32 numBytesStart = numBytes;
	while (numBytes >= 32) {
		*d64++ = *s64++;
		*d64++ = *s64++;
		*d64++ = *s64++;
		*d64++ = *s64++;
		numBytes -= 32;
	}
	if (numBytes >= 16) {
		*d64++ = *s64++;
		*d64++ = *s64++;
		numBytes -= 16;
	}
	if (numBytes >= 8) {
		*d64++ = *s64++;
		numBytes -= 8;
	}
	if (numBytes == 4) {
		uint32* d32 = (uint32*)(dst + numBytesStart - numBytes);
		uint32* s32 = (uint32*)(src + numBytesStart - numBytes);
		*d32 = *s32;
	}
}

void
blend_line_64(uint8* buffer, int32 pixels, uint8 r, uint8 g, uint8 b, uint8 a)
{
	pixel2 p;
	pixels /= 2;

	r = (r * a) >> 8;
	g = (g * a) >> 8;
	b = (b * a) >> 8;
	a = 255 - a;

	uint8 tempBuffer[pixels * 8];

	uint8* t = tempBuffer;
	uint8* s = buffer;

	for (int32 i = 0; i < pixels; i++) {
		p.data64 = *(uint64*)s;

		t[0] = ((p.data8[0] * a) >> 8) + b;
		t[1] = ((p.data8[1] * a) >> 8) + g;
		t[2] = ((p.data8[2] * a) >> 8) + r;

		t[4] = ((p.data8[4] * a) >> 8) + b;
		t[5] = ((p.data8[5] * a) >> 8) + g;
		t[6] = ((p.data8[6] * a) >> 8) + r;

		t += 8;
		s += 8;
	}

	gfxcpy32(buffer, tempBuffer, pixels * 8);
}
void
test2(uint8* buffer, uint32 bpr)
{
	uint8* handle = buffer;

/*	bigtime_t start8 = system_time();
//	for (int32 x = 0; x < 10; x++) {
		handle = buffer;
		for (int32 i = 0; i < 64; i++) {
			blend_line_8(handle, 512, 255, 0, 0, 20);
			handle += bpr;
		}
//	}

	bigtime_t start32 = system_time();
//	for (int32 x = 0; x < 10; x++) {
		handle = buffer;
		for (int32 i = 0; i < 64; i++) {
			blend_line_32(handle, 512, 255, 0, 0, 20);
			handle += bpr;
		}
//	}*/

	bigtime_t start64 = system_time();
//	for (int32 x = 0; x < 10; x++) {
		handle = buffer;
		for (int32 i = 0; i < 640; i++) {
			blend_line_64(handle, 512, 255, 0, 0, 200);
			handle += bpr;
		}
//	}

	bigtime_t finish = system_time();
//	printf("8:  %lld\n", start32 - start8);
//	printf("32: %lld\n", start64 - start32);
	printf("blend 64: %lld\n", finish - start64);
}
	
// #pragma mark -

// DirectConnected
void
Window::DirectConnected(direct_buffer_info* info)
{
	// TODO: for some reason, this deadlocks
	// on B_DIRECT_STOP... be aware
//	fDesktop->LockClipping();

//	fEngine.Lock();
	
	switch(info->buffer_state & B_DIRECT_MODE_MASK) {
		case B_DIRECT_START:
		case B_DIRECT_MODIFY:
			fBuffer.SetTo(info);
			fDesktop->SetOffset(info->window_bounds.left, info->window_bounds.top);
			test2((uint8*)info->bits, info->bytes_per_row);
			break;
		case B_DIRECT_STOP:
			fBuffer.SetTo(NULL);
			break;
	}

//	fDesktop->SetMasterClipping(&fBuffer.WindowClipping());

//	fEngine.Unlock();

//	fDesktop->UnlockClipping();
}

// AddWindow
void
Window::AddWindow(BRect frame, const char* name)
{
	WindowLayer* window = new WindowLayer(frame, name,
										  fDesktop->GetDrawingEngine(),
										  fDesktop);

	// add a coupld children
	frame.OffsetTo(B_ORIGIN);
	frame.InsetBy(5.0, 5.0);
	if (frame.IsValid()) {
		ViewLayer* layer1 = new ViewLayer(frame, "View 1",
										  B_FOLLOW_ALL,
										  B_FULL_UPDATE_ON_RESIZE, 
										  (rgb_color){ 180, 180, 180, 255 });

		frame.OffsetTo(B_ORIGIN);
		frame.InsetBy(15.0, 15.0);
		if (frame.IsValid()) {

			BRect f = frame;
			f.bottom = f.top + f.Height() / 3 - 3;
			f.right = f.left + f.Width() / 3 - 3;

			// top row of views
			ViewLayer* layer;
			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_TOP,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new ViewLayer(f, "View", B_FOLLOW_RIGHT | B_FOLLOW_TOP,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			// middle row of views
			f = frame;
			f.bottom = f.top + f.Height() / 3 - 3;
			f.right = f.left + f.Width() / 3 - 3;
			f.OffsetBy(0, f.Height() + 6);

			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new ViewLayer(f, "View", B_FOLLOW_ALL,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			f.OffsetBy(f.Width() + 6, 0);

			layer = new ViewLayer(f, "View", B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			// bottom row of views
			f = frame;
			f.bottom = f.top + f.Height() / 3 - 3;
			f.right = f.left + f.Width() / 3 - 3;
			f.OffsetBy(0, 2 * f.Height() + 12);

			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			f.OffsetBy(f.Width() + 6, 0);

//			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);

			f.OffsetBy(f.Width() + 6, 0);

//			layer = new ViewLayer(f, "View", B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM,
			layer = new ViewLayer(f, "View", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
								  B_FULL_UPDATE_ON_RESIZE, (rgb_color){ 120, 120, 120, 255 });
			layer1->AddChild(layer);
		}

		window->AddChild(layer1);
	}

	window->Run();

	BMessage message(MSG_ADD_WINDOW);
	message.AddPointer("window", (void*)window);
	fDesktop->PostMessage(&message);
}

// Test
void
Window::Test()
{
	BRect frame(20, 20, 330, 230);
//	AddWindow(frame, "Window 1");
//	AddWindow(frame, "Window 2");
	for (int32 i = 0; i < 20; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}

/*	frame.Set(10, 80, 320, 290);
	for (int32 i = 20; i < 40; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}

	frame.Set(20, 140, 330, 230);
	for (int32 i = 40; i < 60; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}

	frame.Set(20, 200, 330, 230);
	for (int32 i = 60; i < 80; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}

	frame.Set(20, 260, 330, 230);
// 99 windows are ok, the 100th looper does not run anymore,
// I guess I'm hitting a BeOS limit (max loopers per app?)
	for (int32 i = 80; i < 99; i++) {
		BString name("Window ");
		frame.OffsetBy(20, 15);
		name << i + 1;
		AddWindow(frame, name.String());
	}*/
}

// main
int
main(int argc, const char* argv[])
{
	srand((long int)system_time());

	App app;
	app.Run();
	return 0;
}
