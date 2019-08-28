/*
 * Copyright 2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#include <Application.h>
#include <Bitmap.h>
#include <File.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <Window.h>

#include <stdio.h>


#pragma mark - VideoView


class VideoView: public BView
{
	public:
		VideoView(BMediaTrack* track, int32 width, int32 height);
		void Draw(BRect);
		void KeyDown(const char*, int32);

	private:
		BMediaTrack*	fMediaTrack;
		BBitmap			fBitmap;
};


VideoView::VideoView(BMediaTrack* track, int32 width, int32 height)
	: BView(BRect(0, 0, width, height), "Video", B_FOLLOW_NONE,
		B_WILL_DRAW)
	, fMediaTrack(track)
	, fBitmap(BRect(0, 0, width - 1, height - 1), B_RGB32)
{
}


void
VideoView::Draw(BRect r)
{
	DrawBitmap(&fBitmap);
}
		

void
VideoView::KeyDown(const char*, int32)
{
	puts("Next frame");
	int64 count = 1;
	fMediaTrack->ReadFrames(fBitmap.Bits(), &count);
	Invalidate();
}


#pragma mark - VideoWindow


class VideoWindow: public BWindow
{
public:
	VideoWindow(const char* videoFile);
	~VideoWindow();

private:
	BFile*			fFile;
	BMediaFile*		fMediaFile;
	BMediaTrack*	fMediaTrack;
};


VideoWindow::VideoWindow(const char* path)
	: BWindow(BRect(60, 120, 700, 600), "Video Decoder",
		B_DOCUMENT_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	, fMediaTrack(NULL)
{
	fFile = new BFile(path, B_READ_ONLY);
	fMediaFile = new BMediaFile(fFile);
	media_format format;

	int i;
	for (i = fMediaFile->CountTracks(); --i >= 0;) {
		BMediaTrack* mediaTrack = fMediaFile->TrackAt(i);

		mediaTrack->EncodedFormat(&format);
		if (format.IsVideo()) {
			fMediaTrack = mediaTrack;
			fMediaTrack->DecodedFormat(&format);
			break;
		}
	}

	if (fMediaTrack) {
		printf("Found video track\n%ld x %ld ; %lld frames, %f seconds\n",
			format.Width(), format.Height(), fMediaTrack->CountFrames(),
			fMediaTrack->Duration() / 1000000.f);
	} else {
		return;
	}

	BView* view = new VideoView(fMediaTrack, format.Width(), format.Height());
	AddChild(view);
	view->MakeFocus();
}


VideoWindow::~VideoWindow()
{
	delete fMediaFile; // Also deletes the track
	delete fFile;
}


#pragma mark - Main


int main(int argc, char* argv[])
{
	if (argc < 2)
		return -1;

	BApplication app("application/x-vnd.Haiku-VideoDecoder");

	BWindow* window = new VideoWindow(argv[1]);
	window->Show();

	app.Run();
}
