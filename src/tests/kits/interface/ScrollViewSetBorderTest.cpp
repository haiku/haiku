#include <Application.h>
#include <Button.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ScrollView.h>
#include <View.h>
#include <Window.h>


class Window : public BWindow {
public:
		Window() : BWindow(BRect(100, 100, 300, 300), "", B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS |
			B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE)
		{
			BGroupLayout* base = new BGroupLayout(B_HORIZONTAL);
			SetLayout(base);

			BView* view = new BView("", B_WILL_DRAW, NULL);
			view->SetViewColor(255, 0, 0, 255);
			view->SetExplicitMinSize(BSize(B_SIZE_UNSET, 200));

			fScrollView = new BScrollView("mit", view, B_NAVIGABLE_JUMP, true,
				true, B_NO_BORDER);

			BView* view2 = new BView(BRect(0, 0, 200, 200), "", B_FOLLOW_ALL,
				B_WILL_DRAW);
			view2->SetViewColor(255, 0, 0, 255);

			fScrollView2 = new BScrollView("ohne", view2, B_FOLLOW_ALL,
				B_NAVIGABLE_JUMP, true, true, B_NO_BORDER);

			BButton* one = new BButton("No Border", new BMessage('nobd'));
			BButton* two = new BButton("Plain Border", new BMessage('plbd'));
			BButton* three = new BButton("Fancy Border", new BMessage('fcbd'));

			base->AddView(BGroupLayoutBuilder(B_VERTICAL, 5.0)
				.Add(fScrollView)
				.Add(fScrollView2)
				.AddGroup(B_HORIZONTAL, 5.0)
					.Add(one)
					.Add(two)
					.Add(three)
				.End()
				.SetInsets(10.0, 10.0, 10.0, 10.0));

			PrintToStream();
		}

		void MessageReceived(BMessage* message)
		{
			switch(message->what) {
				case 'nobd':
					fScrollView->SetBorder(B_NO_BORDER);
					fScrollView2->SetBorder(B_NO_BORDER);

					PrintToStream();
					break;

				case 'plbd':
					fScrollView->SetBorder(B_PLAIN_BORDER);
					fScrollView2->SetBorder(B_PLAIN_BORDER);

					PrintToStream();
					break;

				case 'fcbd':
					fScrollView->SetBorder(B_FANCY_BORDER);
					fScrollView2->SetBorder(B_FANCY_BORDER);

					PrintToStream();
					break;

				default:
					BWindow::MessageReceived(message);
					break;
			}
		}

		void PrintToStream()
		{
			BView* view = fScrollView->Target();
			BView* view2 = fScrollView2->Target();

			view->Bounds().PrintToStream();
			view->Frame().PrintToStream();

			view2->Bounds().PrintToStream();
			view2->Frame().PrintToStream();
		}

private:
	BScrollView*	fScrollView;
	BScrollView*	fScrollView2;
};


class Application : public BApplication {
public:
			Application() : BApplication("application/x-vnd.scrollview") {}
			~Application() {}

	void	ReadyToRun()
	{
		Window* win = new Window();
		win->Show();
	}
};


int main(int argc, char* argv[])
{
	Application app;
	return app.Run();
}
