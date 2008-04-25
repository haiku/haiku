#include <stdio.h>

#include <Application.h>
#include <Region.h>
#include <View.h>
#include <Window.h>

enum {
	MSG_REDRAW = 'shhd'
};

#define TEST_DELAYS 0

class View : public BView {
public:
	View(BRect frame, uint8 r, uint8 g, uint8 b, uint8 a)
		: BView(frame, "view", B_FOLLOW_ALL, B_WILL_DRAW)
	{
		SetDrawingMode(B_OP_ALPHA);
		SetHighColor(r, g, b, a);
	}

	virtual void Draw(BRect updateRect)
	{
		BRegion region;
		GetClippingRegion(&region);
		BMessage message(MSG_REDRAW);
		int32 count = region.CountRects();
		for (int32 i = 0; i < count; i++)
			message.AddRect("rect", region.RectAt(i));
		be_app->PostMessage(&message);
	}

	void AsyncRedraw(BRegion& region)
	{
		if (!LockLooper())
			return;

#if 0
		ConstrainClippingRegion(&region);
		FillRect(Bounds());
		ConstrainClippingRegion(NULL);
#else
		PushState();
		ConstrainClippingRegion(&region);
		FillRect(Bounds());
		PopState();
#endif

		UnlockLooper();
	}
};


class Application : public BApplication {
public:
	Application()
		: BApplication("application/x-vnd.stippi-async.drawing")
		, fDelay(true)
	{
		BWindow* window = new BWindow(BRect(50, 50, 350, 250),
			"Test Window", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_QUIT_ON_WINDOW_CLOSE);
		fView = new View(window->Bounds(), 255, 80, 155, 128);
		window->AddChild(fView);
		window->Show();

		window = new BWindow(BRect(150, 150, 450, 350),
			"Drag Window", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_QUIT_ON_WINDOW_CLOSE);
		window->Show();

		SetPulseRate(100000);
	}

	virtual void Pulse()
	{
		fDelay = true;
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_REDRAW:
			{
#if TEST_DELAYS
				if (fDelay) {
					snooze(200000);
					fDelay = false;
				}
#endif
				BRegion region;
				BRect rect;
				for (int32 i = 0;
					message->FindRect("rect", i, &rect) == B_OK; i++)
					region.Include(rect);
				fView->AsyncRedraw(region);
				break;
			}

			default:
				BApplication::MessageReceived(message);
				break;
		}
	}
private:
	View*		fView;
	bool		fDelay;
};


int
main(int argc, char* argv[])
{
	Application app;
	app.Run();
	return 0;
}


