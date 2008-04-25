#include <stdio.h>

#include <Application.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <Window.h>

class View : public BView {
public:
	View(BRect frame)
		: BView(frame, "target view", B_FOLLOW_ALL,
			B_WILL_DRAW | B_FRAME_EVENTS)
	{
	}

	virtual void Draw(BRect updateRect)
	{
		BRect b = Bounds().OffsetToCopy(B_ORIGIN);
		b.bottom = b.bottom * 2.0;
		StrokeLine(b.LeftTop(), b.RightBottom());
	}

	virtual	void AttachedToWindow()
	{
		UpdateScrollbar(Bounds().Height());
	}

	virtual void FrameResized(float width, float height)
	{
		UpdateScrollbar(height);
	}

	void UpdateScrollbar(float height)
	{
		BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
		if (!scrollBar) {
			printf("no vertical scroll bar\n");
			return;
		}
		float smallStep, bigStep;
		scrollBar->GetSteps(&smallStep, &bigStep);
		printf("scrollbar steps: %.1f, %.1f, proportion: %.1f\n",
			smallStep, bigStep, scrollBar->Proportion());

		scrollBar->SetRange(0.0, height);
		scrollBar->SetSteps(5.0, height / 2);

		scrollBar->GetSteps(&smallStep, &bigStep);
		printf("scrollbar steps: %.1f, %.1f, proportion: %.1f, "
			"range: %.1f\n",
			smallStep, bigStep, scrollBar->Proportion(),
			height);
	}
};


int
main(int argc, char* argv[])
{
	BApplication app("application/x-vnd.stippi.scrollbar_test");

	BRect frame(50, 50, 350, 350);
	BWindow* window = new BWindow(frame, "BScrollBar Test",
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	frame = window->Bounds();
	frame.right -= B_V_SCROLL_BAR_WIDTH;
	View* view = new View(frame);

	BScrollView* scrollView = new BScrollView("scroll view", view,
		B_FOLLOW_ALL, 0, false, true, B_NO_BORDER);

	window->AddChild(scrollView);
	window->Show();

	app.Run();
	return 0;
}


