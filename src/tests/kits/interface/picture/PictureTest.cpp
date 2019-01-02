#include <Application.h>
#include <Box.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Picture.h>
#include <Shape.h>
#include <View.h>
#include <Window.h>


#include "SVGViewView.h"


class Svg2PictureWindow : public BWindow {
public:
	Svg2PictureWindow(BRect frame, const char *filename)
		:   BWindow(frame, "Svg2Picture", B_TITLED_WINDOW, 0) {
        
        	BView *view = new Svg2PictureView(Bounds(), filename);
        	AddChild(view);
	}
};


class OriginalView : public BBox {
public:
	OriginalView(BRect frame);
	virtual void Draw(BRect update);
};


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
	view->DrawString("BPicture ", BPoint(30, 20));
	view->DrawString("test");

	// DrawLine with pen position
	const rgb_color purple = { 200, 0, 220, 0 };
	view->SetHighColor(purple);
	view->StrokeLine(BPoint(50, 30), BPoint(30, 50));
	view->StrokeLine(BPoint(80, 50));
	view->StrokeLine(BPoint(50, 30));
}


// OriginalView
OriginalView::OriginalView(BRect frame)
	:	BBox(frame, "original_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}


void
OriginalView::Draw(BRect updateRect)
{
	DrawStuff(this);
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
	BeginPicture(new BPicture);
	
	DrawStuff(this);

	BPicture *picture = EndPicture();
	if (picture == NULL)
		return;

	BMessage message;
	picture->Archive(&message);
	message.PrintToStream();

	BMallocIO stream;
	
	status_t status = picture->Flatten(&stream);
	delete picture;

	if (status != B_OK)
		printf("Error flattening BPicture: %s\n", strerror(status));
	
	if (status == B_OK) {
		stream.Seek(0, SEEK_SET);
		fPicture = new BPicture();
		status = fPicture->Unflatten(&stream);
		if (status != B_OK) {
			printf("Error unflattening BPicture: %s\n", strerror(status));
			return;
		}
	}

	BMessage message2;
	fPicture->Archive(&message2);
	message2.PrintToStream();
}


void
PictureView::Draw(BRect update)
{
	if (fPicture)
		DrawPicture(fPicture, B_ORIGIN);
}


// #pragma mark -


int
main()
{		
	BApplication pictureApp("application/x-vnd.picture");

	BWindow *pictureWindow = new BWindow(BRect(100, 100, 500, 400),
		"BPicture test", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_QUIT_ON_WINDOW_CLOSE);

	BRect rect(pictureWindow->Bounds());
	rect.right -= (rect.Width() + 1) / 2;
	OriginalView *testView = new OriginalView(rect);
	
	rect.OffsetBy(rect.Width() + 1, 0);
	PictureView *pictureView = new PictureView(rect);
	
	pictureWindow->AddChild(testView);
	pictureWindow->AddChild(pictureView);
	pictureWindow->Show();

	BPath path;
	if (find_directory(B_SYSTEM_DATA_DIRECTORY, &path) == B_OK) {
		path.Append("artwork/lion.svg");
		BEntry entry(path.Path());
		if (entry.Exists()) {
			BWindow *svgWindow = new Svg2PictureWindow(BRect(300, 300, 600, 600),
				path.Path());
			svgWindow->Show();
		}
	}

	pictureApp.Run();
	return 0;
}

