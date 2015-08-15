#include <Application.h>
#include <ChannelSlider.h>
#include <Window.h>

#include <string>


struct limit_label {
	std::string min_label;
	std::string max_label;
};


const struct limit_label kLabels[] = {
	{ "min_label_1", "max_label_1" },
	{ "min_label_2", "max_label_2" },
	{ "min_label_3", "max_label_3" },
};


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

		for (int32 i = 0; i < horizontal->CountChannels(); i++) {
			horizontal->SetLimitLabelsFor(i, kLabels[i].min_label.c_str(),
												kLabels[i].max_label.c_str());
		}

		for (int32 i = 0; i < horizontal->CountChannels(); i++) {
			if (strcmp(horizontal->MinLimitLabelFor(i), kLabels[i].min_label.c_str()) != 0)
				printf("wrong min label for channel %ld\n", i);
			if (strcmp(horizontal->MaxLimitLabelFor(i), kLabels[i].max_label.c_str()) != 0)
				printf("wrong max label for channel %ld\n", i);
		}
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
