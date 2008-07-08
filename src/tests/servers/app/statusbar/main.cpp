#include <stdio.h>

#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <StatusBar.h>
#include <Window.h>

enum {
	MSG_PULSE	= 'puls'
};

class Window : public BWindow {
public:
	Window(BRect frame)
		: BWindow(frame, "BStatusBar Test", B_TITLED_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS
			| B_QUIT_ON_WINDOW_CLOSE | B_NOT_ZOOMABLE),
		  fHomeFolderEntryCount(0),
		  fHomeFolderCurrentEntry(0)
	{
		frame = Bounds();
		BView* background = new BView(frame, "bg", B_FOLLOW_ALL, 0);
		background->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		AddChild(background);

		frame = background->Bounds();
		frame.InsetBy(10, 10);
		fStatusBar = new BStatusBar(frame, "status", "Label: ", "-Trailing");
		fStatusBar->SetResizingMode(B_FOLLOW_ALL);
		background->AddChild(fStatusBar);

		fHomeFolder.SetTo("/boot/home/");
		BEntry entry;
		while (fHomeFolder.GetNextEntry(&entry) == B_OK)
			fHomeFolderEntryCount++;

		fPulse = new BMessageRunner(BMessenger(this),
			new BMessage(MSG_PULSE), 1000000);
	}

	~Window()
	{
		delete fPulse;
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_PULSE: {
				BEntry entry;
				if (fHomeFolder.GetNextEntry(&entry) < B_OK) {
					fHomeFolderCurrentEntry = 0;
					fHomeFolder.Rewind();
					fStatusBar->Reset("Label: ", "-Trailing");
					if (fHomeFolder.GetNextEntry(&entry) < B_OK)
						break;
				} else
					fHomeFolderCurrentEntry++;
				char name[B_FILE_NAME_LENGTH];
				if (entry.GetName(name) < B_OK)
					break;
				float value = 100.0 * fHomeFolderCurrentEntry
					/ (fHomeFolderEntryCount - 1);
				fStatusBar->SetTo(value, "Text", name);
				break;
			}
			default:
				BWindow::MessageReceived(message);
		}
	}
private:
	BStatusBar*	fStatusBar;
	BDirectory	fHomeFolder;
	int32		fHomeFolderEntryCount;
	int32		fHomeFolderCurrentEntry;
	BMessageRunner*	fPulse;
};


int
main(int argc, char* argv[])
{
	BApplication app("application/x-vnd.stippi.statusbar_test");

	BRect frame(50, 50, 350, 350);
	Window* window = new Window(frame);
	window->Show();

	app.Run();
	return 0;
}


