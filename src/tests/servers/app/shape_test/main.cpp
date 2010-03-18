#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Message.h>
#include <Shape.h>
#include <View.h>
#include <Window.h>


static const char* kAppSignature = "application/x.vnd-Haiku.ShapeTest";


class TestView : public BView {
public:
								TestView(BRect frame, const char* name,
									uint32 resizeFlags, uint32 flags);

	virtual	void				Draw(BRect updateRect);
};


TestView::TestView(BRect frame, const char* name, uint32 resizeFlags,
		uint32 flags)
	:
	BView(frame, name, resizeFlags, flags)
{
}


void
TestView::Draw(BRect updateRect)
{
	BRect r(Bounds());

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	SetHighColor(0, 0, 0, 240);

	class TranslateIterator : public BShapeIterator {
	public:
		TranslateIterator()
			:
			fOffsetX(0),
			fOffsetY(0)
		{
		}

		TranslateIterator(float offsetX, float offsetY)
			:
			fOffsetX(offsetX),
			fOffsetY(offsetY)
		{
		}

		void SetOffset(float offsetX, float offsetY)
		{
			fOffsetX = offsetX;
			fOffsetY = offsetY;
		}

		virtual status_t IterateMoveTo(BPoint* point)
		{
			point->x += fOffsetX;
			point->y += fOffsetY;
			return B_OK;
		}

		virtual status_t IterateLineTo(int32 lineCount, BPoint* linePts)
		{
			while (lineCount--) {
				linePts->x += fOffsetX;
				linePts->y += fOffsetY;
				linePts++;
			}
			return B_OK;
		}

		virtual status_t IterateBezierTo(int32 bezierCount, BPoint* bezierPts)
		{
			while (bezierCount--) {
				bezierPts[0].x += fOffsetX;
				bezierPts[0].y += fOffsetY;
				bezierPts[1].x += fOffsetX;
				bezierPts[1].y += fOffsetY;
				bezierPts[2].x += fOffsetX;
				bezierPts[2].y += fOffsetY;
				bezierPts += 3;
			}
			return B_OK;
		}

		virtual status_t IterateArcTo(float& rx, float& ry, float& angle,
			bool largeArc, bool counterClockWise, BPoint& point)
		{
			point.x += fOffsetX;
			point.y += fOffsetY;
			return B_OK;
		}

	private:
		float fOffsetX;
		float fOffsetY;
	} translator;

	MovePenTo(B_ORIGIN);

	const float arcRX = 50;
	const float arcRY = 80;

	BShape shape;
	shape.MoveTo(BPoint(20, 10));
	shape.LineTo(BPoint(10, 90));
	shape.LineTo(BPoint(90, 100));
	shape.ArcTo(arcRX, arcRY, 45, true, true, BPoint(100, 20));
	shape.Close();

	StrokeShape(&shape);

	shape.Clear();
	shape.MoveTo(BPoint(20, 10));
	shape.LineTo(BPoint(10, 90));
	shape.LineTo(BPoint(90, 100));
	shape.ArcTo(arcRX, arcRY, 45, false, true, BPoint(100, 20));
	shape.Close();

	translator.SetOffset(10, 10);
	translator.Iterate(&shape);

	SetHighColor(140, 30, 50, 255);
	StrokeShape(&shape);

	shape.Clear();
	shape.MoveTo(BPoint(20, 10));
	shape.LineTo(BPoint(10, 90));
	shape.LineTo(BPoint(90, 100));
	shape.ArcTo(arcRX, arcRY, 45, true, false, BPoint(100, 20));
	shape.Close();

	translator.SetOffset(20, 20);
	translator.Iterate(&shape);

	SetHighColor(140, 130, 50, 255);
	StrokeShape(&shape);

	shape.Clear();
	shape.MoveTo(BPoint(20, 10));
	shape.LineTo(BPoint(10, 90));
	shape.LineTo(BPoint(90, 100));
	shape.ArcTo(arcRX, arcRY, 45, false, false, BPoint(100, 20));
	shape.Close();

	translator.SetOffset(30, 30);
	translator.Iterate(&shape);

	SetHighColor(40, 130, 150, 255);
	StrokeShape(&shape);
}


// #pragma mark -


int
main(int argc, char** argv)
{
	BApplication app(kAppSignature);

	BWindow* window = new BWindow(BRect(50.0, 50.0, 300.0, 250.0),
		"BShape Test", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BView* view = new TestView(window->Bounds(), "test", B_FOLLOW_ALL,
		B_WILL_DRAW);
	window->AddChild(view);

	window->Show();

	app.Run();
	return 0;
}
