#include <Application.h>
#include <Box.h>
#include <Picture.h>
#include <Region.h>
#include <Shape.h>
#include <View.h>
#include <Window.h>



class PictureView : public BBox {
public:
	PictureView(BRect frame);
	~PictureView();
	
	virtual void Draw(BRect update);
	virtual void AllAttached();

private:
	BPicture *fPicture;
};


static void
DrawStuff(BView *view)
{
	// StrokeShape
	BShape shape;
	BPoint bezier[3] = {BPoint(100,0), BPoint(100, 100), BPoint(25, 50)};
	shape.MoveTo(BPoint(150,0));
	shape.LineTo(BPoint(200,100));
	shape.BezierTo(bezier);
	shape.Close();
	view->StrokeShape(&shape);
	
	// Stroke/FillRect, Push/PopState, SetHighColor, SetLineMode, SetPenSize
	view->PushState();
	const rgb_color blue = { 0, 0, 240, 0 };
	view->SetHighColor(blue);
	view->SetLineMode(B_BUTT_CAP, B_BEVEL_JOIN);
	view->SetPenSize(7);
	view->StrokeRect(BRect(10, 220, 50, 260));
	view->FillRect(BRect(65, 245, 120, 300));
	view->PopState();
	
	// Stroke/FillEllipse
	view->StrokeEllipse(BPoint(50, 150), 50, 50);
	view->FillEllipse(BPoint(100, 120), 50, 50);
	
	// Stroke/FillArc
	view->StrokeArc(BRect(0, 200, 50, 250), 180, 180);
	view->FillArc(BPoint(150, 250), 50, 50, 0, 125);
	
	// DrawString, SetHighColor, SetFontSize
	const rgb_color red = { 240, 0, 0, 0 };
	view->SetHighColor(red);
	view->SetFontSize(20);
	view->DrawString("BPicture test", BPoint(30, 20));

	BRegion region;
	region.Include(BRect(10, 10, 40, 40));
	region.Include(BRect(30, 30, 100, 50));
	view->ConstrainClippingRegion(&region);
}




// PictureView
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
PictureView::AllAttached()
{	
	SetDiskMode("picture", 0);

	DrawStuff(this);

	BPicture *picture = EndPicture();
	if (picture == NULL)
		return;

}

void
PictureView::Draw(BRect update)
{
	DrawPicture("picture", 0, B_ORIGIN);
}


// main
int
main()
{		
	BApplication pictureApp("application/x-vnd.SetDiskModeTest");
	BWindow *pictureWindow = new BWindow(BRect(100, 100, 500, 400),
				"SetDiskMode test", B_TITLED_WINDOW,
				B_NOT_RESIZABLE|B_NOT_ZOOMABLE|B_QUIT_ON_WINDOW_CLOSE);
	
	
	BRect rect(pictureWindow->Bounds());
	
	PictureView *pictureView = new PictureView(rect);
	
	pictureWindow->AddChild(pictureView);
	pictureWindow->Show();

	pictureApp.Run();
	
	return 0;
}

