#include <stdio.h>
#include <SupportDefs.h>

#include <Application.h>
#include <String.h>
#include <View.h>
#include <Window.h>


class TruncateView : public BView {
public:
	TruncateView(BRect frame)
		:	BView(frame, "TruncateView", B_FOLLOW_ALL, B_WILL_DRAW)
	{
	}

	void Draw(BRect updateRect)
	{
		const float kSpacing = 20.0;
		const float kTopOffset = kSpacing + 5;
		const uint32 kTruncateModes[]
			= { B_TRUNCATE_BEGINNING, B_TRUNCATE_MIDDLE, B_TRUNCATE_END };
		const uint32 kTruncateModeCount
			= sizeof(kTruncateModes) / sizeof(kTruncateModes[0]);

		BString theString("ö&ä|ü-é#");
		BPoint point(10, kTopOffset);
		BString truncated;
		for (float length = 5; length < 65; length += 3) {
			SetHighColor(255, 0, 0);
			StrokeRect(BRect(point.x, point.y - kSpacing, point.x + length,
				point.y + kSpacing * kTruncateModeCount));
			SetHighColor(0, 0, 0);

			for (uint32 i = 0; i < kTruncateModeCount; i++) {
				truncated = theString;
				TruncateString(&truncated, kTruncateModes[i], length);
				DrawString(truncated.String(), point);
				point.y += kSpacing;
			}

			point.x += length + kSpacing;
			point.y = kTopOffset;
		}
	}
};


int
main(int argc, char *argv[])
{
	BApplication app("application/x-vnd.Haiku-TruncateString");

	BRect frame(100, 200, 1200, 300);
	BWindow *window = new BWindow(frame, "TruncateString", B_TITLED_WINDOW,
		B_QUIT_ON_WINDOW_CLOSE);

	TruncateView *view = new TruncateView(frame.OffsetToSelf(0, 0));
	window->AddChild(view);

	window->Show();
	app.Run();
	return 0;
}
