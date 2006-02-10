#include <Application.h>
#include <ChannelSlider.h>
#include <Window.h>

class MainWindow : public BWindow {
public:
	MainWindow()
		:BWindow(BRect(50, 50, 250, 360), "window", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	{
		BChannelSlider *slider = new BChannelSlider(BRect(10, 10, 20, 20),
			"vertical slider", "vertical slider", new BMessage('test'), 4);
		slider->SetOrientation(B_VERTICAL);
		AddChild(slider);
		slider->ResizeToPreferred();
		BChannelSlider *horizontal = new BChannelSlider(BRect(100, 20, 136, 30),
			 "horizontal slider", "horizontal slider", new BMessage('test'), 3);
		AddChild(horizontal);
		horizontal->ResizeToPreferred();
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
