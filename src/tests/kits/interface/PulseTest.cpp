#include <Application.h>
#include <View.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>

static void
ChangeColor(rgb_color &color)
{
	color.red = rand() % 255;
	color.green = rand() % 255;
}


class PulseView : public BView {
public:
	PulseView(BRect rect, const char *name, uint32 resizeMode, uint32 flags)
		: BView(rect, name, resizeMode, flags)
	{
		fLeft = Bounds().OffsetToCopy(B_ORIGIN);
		fLeft.right -= Bounds().Width() / 2;
		fRight = fLeft.OffsetByCopy(fLeft.Width(), 0);
		fColor.red = 255;
		fColor.green = 255;
		fColor.blue = 255;
	}
	
	virtual void Pulse()
	{
		SetHighColor(fColor);
		BRect rect = fRight;
		
		if (fLeftTurn)
			rect = fLeft;
		
		FillRect(rect, B_SOLID_HIGH);
		
		fLeftTurn = !fLeftTurn;
		
		ChangeColor(fColor);	
	}
	
	BRect fLeft;
	BRect fRight;	
	
	bool fLeftTurn;
	rgb_color fColor;
};

void
show_window(BWindow *window)
{
	BView *view = new PulseView(window->Bounds(), "pulse view", B_FOLLOW_ALL, B_PULSE_NEEDED|B_WILL_DRAW);
	window->SetPulseRate(500000);
	window->AddChild(view);
	window->Show();
}


int main()
{
	srand(time(NULL));
	BApplication app("application/x-vnd.pulse_test");
	BWindow *window = new BWindow(BRect(100, 100, 400, 300), "pulse test",
		B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE | B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE);
	show_window(window);	
	app.Run();
}
