#include <Application.h>
#include <Window.h>
#include <Box.h>
#include <View.h>
#include <Shape.h>
#include <Picture.h>


class PictureView : public BBox {
	public:
		PictureView(BRect frame);
		~PictureView();
		
		virtual void Draw(BRect update);
		virtual void AllAttached(void);
	private:
		BPicture *fPicture;
};

PictureView::PictureView(BRect frame)
	:	BBox(frame, "pict_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
		fPicture(NULL)
{
}

PictureView::~PictureView()
{
	delete fPicture;
}

void
PictureView::AllAttached(void)
{
	BeginPicture(new BPicture);
	
	BShape shape;
	BPoint bezier[3] = {BPoint(100,0), BPoint(100, 100), BPoint(25, 50)};
	shape.MoveTo(BPoint(150,0));
	shape.LineTo(BPoint(200,100));
	shape.BezierTo(bezier);
	shape.Close();
	StrokeShape(&shape);
	
	PushState();
	const rgb_color blue = { 0, 0, 240, 0 };
	SetHighColor(blue);
	SetLineMode(B_BUTT_CAP, B_BEVEL_JOIN);
	SetPenSize(7);
	StrokeRect(BRect(10, 220, 50, 260));

	FillRect(BRect(65, 245, 120, 300));
	PopState();
	
	StrokeEllipse(BPoint(50, 150), 50, 50);
	
	FillEllipse(BPoint(100, 120), 50, 50);
	
	StrokeArc(BRect(0, 200, 50, 250), 180, 180);
	
	FillArc(BPoint(150, 250), 50, 50, 0, 125);
	
	const rgb_color red = { 240, 0, 0, 0 };
	SetHighColor(red);
	SetFontSize(20);
	DrawString("BPicture test", BPoint(30, 20));

	fPicture = EndPicture();
}

void
PictureView::Draw(BRect update)
{
	if (fPicture)
		DrawPicture(fPicture, B_ORIGIN);
}


int
main(void)
{		
	BApplication *pictureApp = new BApplication("application/x-vnd.picture");
	BWindow *pictureWindow = new BWindow(BRect(100, 100, 300, 400), "BPicture test",
								B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE);
	PictureView *pictureView = new PictureView(pictureWindow->Bounds());
	pictureWindow->AddChild(pictureView);
	
	pictureWindow->Show();
	pictureApp->Run();
	
	delete pictureApp;
	
	return 0;
}

