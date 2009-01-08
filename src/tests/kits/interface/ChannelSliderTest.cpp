#include <Application.h>
#include <ChannelSlider.h>
#include <Window.h>

class MainWindow : public BWindow {
public:
	MainWindow()
		:BWindow(BRect(50, 50, 250, 360), "window", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	{
		BChannelSlider *slider = new BChannelSlider(BRect(10, 10, 20, 20),
			"vertical slider", "Verticalp", new BMessage('test'), 4);
		slider->SetOrientation(B_VERTICAL);
		AddChild(slider);
		slider->ResizeToPreferred();

		slider = new BChannelSlider(BRect(10, 10, 20, 20), "vertical slider",
			"Verticalp", new BMessage('test'), B_VERTICAL,  4);
		AddChild(slider);
		slider->SetLimitLabels("Wminp", "Wmaxp");
		slider->ResizeToPreferred();
		slider->MoveBy(slider->Bounds().Width() + 10.0, 0.0);

		BChannelSlider *horizontal = new BChannelSlider(BRect(150, 10, 160, 20),
			 "horizontal slider", "Horizontal", new BMessage('test'), 3);
		AddChild(horizontal);
		horizontal->ResizeToPreferred();

		horizontal = new BChannelSlider(BRect(150, 10, 160, 20),
			 "horizontal slider", "Horizontalp", new BMessage('test'),
			 B_HORIZONTAL, 3);
		AddChild(horizontal);
		horizontal->SetLimitLabels("Wminp", "Wmaxp");
		horizontal->ResizeToPreferred();
		horizontal->MoveBy(0.0, horizontal->Bounds().Height() + 10.0);

		ResizeTo(horizontal->Frame().right + 10, slider->Frame().bottom + 10);
	}

	virtual bool QuitRequested() { be_app->PostMessage(B_QUIT_REQUESTED); return BWindow::QuitRequested() ; }
};


class App : public BApplication {
public:
	App() : BApplication("application/x-vnd.channelslidertest")
	{
	}

	virtual void ReadyToRun()
	{
		(new MainWindow())->Show();
	}

};

int main()
{
	App app;

	app.Run();

	return 0;
}
