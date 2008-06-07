#include <Application.h>
#include <Bitmap.h>
#include <View.h>
#include <Window.h>
#include <malloc.h>
#include <stdio.h>


#if 0
{
	// usage example for simple rects
	port_id port = find_port("drawing_debugger_port");
	if (port >= 0) {
		write_port_etc(port, 'RECT', &interestingRect, sizeof(BRect),
			B_RELATIVE_TIMEOUT, 0);
	}


	// usage example for regions
	port_id port = find_port("drawing_debugger_port");
	if (port >= 0) {
		int32 rectCount = region->CountRects();
		for (int32 i = 0; i < rectCount; i++) {
			BRect interestingRect = region->RectAt(i);
			write_port_etc(port, 'RECT', &interestingRect, sizeof(BRect),
				B_RELATIVE_TIMEOUT, 0);
		}
	}
}
#endif


class DrawingDebuggerView : public BView {
public:
								DrawingDebuggerView(BRect frame);
virtual							~DrawingDebuggerView();

virtual	void					Draw(BRect updateRect);

private:
static	int32					_PortListener(void *data);

		BBitmap *				fBitmap;
		BView *					fOffscreenView;
		thread_id				fListenerThread;
		bool					fStopListener;
};


class DrawingDebuggerWindow : public BWindow {
public:
								DrawingDebuggerWindow(BRect frame);

private:
		DrawingDebuggerView *	fView;
};


class DrawingDebuggerApp : public BApplication {
public:
								DrawingDebuggerApp();

private:
		DrawingDebuggerWindow *	fWindow;
};


rgb_color
random_color()
{
	rgb_color result;
	result.red = system_time() % 256;
	result.green = (system_time() >> 8) % 256;
	result.blue = (system_time() >> 16) % 256;
	return result;
}


DrawingDebuggerApp::DrawingDebuggerApp()
	:	BApplication("application/x.vnd-Haiku.DrawingDebugger")
{
	fWindow = new DrawingDebuggerWindow(BRect(200, 200, 999, 799));
	fWindow->Show();
}


DrawingDebuggerWindow::DrawingDebuggerWindow(BRect frame)
	:	BWindow(frame, "DrawingDebugger", B_TITLED_WINDOW,
			B_QUIT_ON_WINDOW_CLOSE)
{
	fView = new DrawingDebuggerView(frame.OffsetToSelf(0, 0));
	AddChild(fView);
}


DrawingDebuggerView::DrawingDebuggerView(BRect frame)
	:	BView(frame, "DrawingDebuggerView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fBitmap = new BBitmap(frame, B_RGB32, true);
	fOffscreenView= new BView(frame, "OffscreenView", B_FOLLOW_NONE, 0);
	fOffscreenView->SetViewColor(B_TRANSPARENT_32_BIT);
	fBitmap->AddChild(fOffscreenView);

	fStopListener = false;
	fListenerThread = spawn_thread(_PortListener, "port_listener",
		B_NORMAL_PRIORITY, this);
	resume_thread(fListenerThread);
}


DrawingDebuggerView::~DrawingDebuggerView()
{
	fStopListener = true;
	int32 returnCode = B_OK;
	wait_for_thread(fListenerThread, &returnCode);
	delete fBitmap;
}


void
DrawingDebuggerView::Draw(BRect updateRect)
{
	DrawBitmap(fBitmap, updateRect, updateRect);
}


int32
DrawingDebuggerView::_PortListener(void *data)
{
	DrawingDebuggerView *view = (DrawingDebuggerView *)data;
	port_id port = create_port(1000, "drawing_debugger_port");
	if (port < 0) {
		printf("failed to create port\n");
		return port;
	}

	size_t bufferSize = 1024;
	uint8 *buffer = (uint8 *)malloc(bufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	while (!view->fStopListener) {
		int32 code = 0;
		status_t result = read_port_etc(port, &code, buffer, bufferSize,
			B_RELATIVE_TIMEOUT, 100000);
		if (result == B_INTERRUPTED || result == B_TIMED_OUT)
			continue;
		if (result < B_OK) {
			printf("failed to read from port\n");
			return result;
		}

		switch (code) {
			case 'RECT':
			{
				BRect *rect = (BRect *)buffer;
				view->fBitmap->Lock();
				view->fOffscreenView->SetHighColor(random_color());
				view->fOffscreenView->FillRect(*rect, B_SOLID_HIGH);
				view->fOffscreenView->Sync();
				view->fBitmap->Unlock();

				view->LockLooper();
				view->Invalidate(*rect);
				view->UnlockLooper();
				break;
			}

			default:
				printf("unsupported code '%.4s'\n", (char *)&code);
				break;
		}
	}

	free(buffer);
	delete_port(port);
	return 0;
}


int
main(int argc, const char *argv[])
{
	DrawingDebuggerApp *app = new DrawingDebuggerApp();
	app->Run();
	delete app;
	return 0;
}
