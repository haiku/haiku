// main.cpp

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Archivable.h>
#include <Dragger.h>
#include <File.h>
#include <Message.h>
#include <View.h>
#include <Window.h>

static const char* kAppSignature = "application/x.vnd-Haiku.ArchivedView";

class TestView : public BView {

 public:
							TestView(BRect frame, const char* name,
									 uint32 resizeFlags, uint32 flags);

							TestView(BMessage* archive);

	virtual	status_t		Archive(BMessage* into, bool deep = true) const;

	static	BArchivable*	Instantiate(BMessage* archive);

	virtual	void			AttachedToWindow();
	virtual	void			DetachedFromWindow();

	virtual	void			Draw(BRect updateRect);

 private:
			int32			fData;
};


// constructor
TestView::TestView(BRect frame, const char* name,
				   uint32 resizeFlags, uint32 flags)
	: BView(frame, name, resizeFlags, flags),
	  fData(42)
{
	SetViewColor(216, 216, 216);

	BRect r = Bounds();
	r.top = r.bottom - 7;
	r.left = r.right - 7;
	BDragger *dw = new BDragger(r, this, 0);
	AddChild(dw);
}

// constructor
TestView::TestView(BMessage* archive)
	: BView(archive)
{
	if (archive->FindInt32("data", &fData) < B_OK)
		fData = 5;

	printf("data: %ld\n", fData);
}

// AttachedToWindow
void
TestView::AttachedToWindow()
{
}

// DetachedFromWindow
void
TestView::DetachedFromWindow()
{
	BFile file("/tmp/archived_view",
			   B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	status_t err = file.InitCheck();
	if (err != B_OK) {
		printf("error creating file for archiving: %s\n", strerror(err));
		return;
	}
	BMessage archive;
	err = Archive(&archive);
	if (err != B_OK) {
		printf("error archiving: %s\n", strerror(err));
		return;
	}
	err = archive.Flatten(&file);
	if (err != B_OK) {
		printf("error flattening: %s\n", strerror(err));
		return;
	}
}

// Draw
void
TestView::Draw(BRect updateRect)
{
printf("TestView::Draw(");
updateRect.PrintToStream();
	BRect r(Bounds());

	rgb_color light = tint_color(ViewColor(), B_LIGHTEN_2_TINT);
	rgb_color shadow = tint_color(ViewColor(), B_DARKEN_2_TINT);

	BeginLineArray(4);
		AddLine(r.LeftTop(), r.RightTop(), light);
		AddLine(r.RightTop(), r.RightBottom(), shadow);
		AddLine(r.RightBottom(), r.LeftBottom(), shadow);
		AddLine(r.LeftBottom(), r.LeftTop(), light);
	EndLineArray();
}

// Archive
status_t
TestView::Archive(BMessage* into, bool deep) const
{
	status_t ret = BView::Archive(into, deep);

	if (ret == B_OK)
		ret = into->AddInt32("data", fData);

	if (ret == B_OK)
		ret = into->AddString("add_on", kAppSignature);

	if (ret == B_OK)
		ret = into->AddString("class", "TestView");

	return ret;
}

// Instantiate
BArchivable *
TestView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive , "TestView"))
		return NULL;

	return new TestView(archive);
}

// #pragma mark -

// show_window
void
show_window(BRect frame, const char* name)
{
	BWindow* window = new BWindow(frame, name,
								  B_TITLED_WINDOW,
								  B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BView* view = NULL;
	BFile file("/tmp/archived_view", B_READ_ONLY);
	if (file.InitCheck() == B_OK) {
		printf("found archive file\n");
		BMessage archive;
		if (archive.Unflatten(&file) == B_OK) {
			printf("unflattened archive message\n");
			BArchivable* archivable = instantiate_object(&archive);
			view = dynamic_cast<BView*>(archivable);
			if (view)
				printf("instatiated BView\n");
		}
	}

	if (!view)
		view = new TestView(window->Bounds(), "test", B_FOLLOW_ALL, B_WILL_DRAW);

	window->Show();

	window->Lock();
	window->AddChild(view);
	window->Unlock();
}

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication(kAppSignature);

	BRect frame(50.0, 50.0, 300.0, 250.0);
	show_window(frame, "BView Archiving Test");

	app->Run();

	delete app;
	return 0;
}
