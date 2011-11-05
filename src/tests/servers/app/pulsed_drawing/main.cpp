#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <View.h>
#include <Window.h>


class PulsedView : public BView {
public:
	PulsedView(BRect frame)
		:
		BView(frame, "pulsed view", B_FOLLOW_ALL, B_WILL_DRAW)
	{
	}

	virtual void Draw(BRect updateRect)
	{
		SetHighColor(rand() % 255, rand() % 255, rand() % 255, 255);
		FillRect(updateRect);
	}
};


class PulsedApplication : public BApplication {
public:
	PulsedApplication()
		:
		BApplication("application/x-vnd.haiku.pulsed_drawing")
	{
		BRect frame(100, 100, 400, 300);
		BWindow* window = new BWindow(frame, "Pulsed Drawing",
			B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_QUIT_ON_WINDOW_CLOSE);

		fView = new PulsedView(frame.OffsetToCopy(0, 0));
		window->AddChild(fView);
		window->Show();

		SetPulseRate(1 * 1000 * 1000);
	}

	virtual void Pulse()
	{
		if (!fView->LockLooper())
			return;

		fView->Invalidate();
		fView->UnlockLooper();
	}

private:
	PulsedView* fView;
};


int
main(int argc, char* argv[])
{
	PulsedApplication app;
	app.Run();
	return 0;
}
