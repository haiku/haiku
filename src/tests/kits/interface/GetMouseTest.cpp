#include <Application.h>
#include <View.h>
#include <Window.h>

#include <stdio.h>

class view : public BView {
public:
	view(BRect rect)
	: BView(rect, "test view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
	{
	}
	
	virtual void
	MouseDown(BPoint where)
	{
		printf("Mouse DOWN !!! %lld\n", system_time());
		
		BPoint mouseWhere;
		ulong buttons;	
		do {
			GetMouse(&mouseWhere, &buttons);
			snooze(10000);
		} while (buttons != 0);
					
		printf("Mouse UP !!! %lld\n", system_time());
	}
};


class window : public BWindow {
public:
	window() :
		BWindow(BRect(100, 100, 400, 300), "test window", B_DOCUMENT_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	{
		AddChild(new view(Bounds()));
	}
	
	virtual void
	DispatchMessage(BMessage *message, BHandler *handler)
	{
		bigtime_t when;
		message->FindInt64("when", &when);
		switch (message->what) {
			case B_MOUSE_MOVED:
				printf("B_MOUSE_MOVED: %lld\n", when);
				break;
			case B_MOUSE_UP:
				printf("B_MOUSE_UP: %lld\n", when);
				break;
			case B_MOUSE_DOWN:
				printf("B_MOUSE_DOWN: %lld\n", when);
				break;
			default:
				break;
		}
		BWindow::DispatchMessage(message, handler);
	}
};

int
main()
{
	BApplication app("application/x-vnd.getmousetest");
	
	(new window())->Show();

	app.Run();
}
