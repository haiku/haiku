//******************************************************************************
//
//******************************************************************************

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#define DEBUG 1
#include <Debug.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <Application.h>
#include <Window.h>
#include <Box.h>
#include <File.h>

#include <Path.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Shelf.h>
#include <ByteOrder.h>

#define MAGIC_1	'pjpp'
#define MAGIC_2	'jahh'
#define MAGIC_1_SWAP	'ppjp'
#define MAGIC_2_SWAP	'hhaj'

/*------------------------------------------------------------*/
BEntry *archive_file(bool create = TRUE);

class TWindow : public BWindow {
public:
					TWindow(BRect frame, const char *title, BPositionIO *stream);
					~TWindow();
virtual bool		QuitRequested();
virtual void		Quit();

private:
		BBox		*fMainView;
		BShelf		*fShelf;
		BPositionIO	*fArchiveStream;
};

/*------------------------------------------------------------*/

int main(int argc, char* argv[])
{
	BApplication	*app = new BApplication("application/x-vnd.Be-MYTE");
	BEntry			*entry;
	BRect			frame;
	bool			frame_set =  FALSE;
	TWindow			*w = NULL;
	BFile			*stream = NULL;

	if ((entry = archive_file())) {
		long	d1 = 0;
		long	d2 = 0;
		long	err;

		stream = new BFile(entry, O_RDWR);

		// need to consider Endian issues here!
		err = stream->Read(&d1, sizeof(d1));
		err = stream->Read(&d2, sizeof(d2));

		if ((d1 == MAGIC_1) && (d2 == MAGIC_2)) {
			err = stream->Read(&frame, sizeof(frame));
			frame_set = TRUE;
		} else if ((d1 == MAGIC_1_SWAP) &&
			(d2 == MAGIC_2_SWAP)) {
				err = stream->Read(&frame, sizeof(frame));
				swap_data(B_RECT_TYPE, &frame, sizeof(BRect), B_SWAP_ALWAYS);
				frame_set = TRUE;
		}
		delete entry;
	}
	if (!frame_set)
		frame = BRect(100, 50, 300, 400);

	w = new TWindow(frame, "Container", stream);
	w->Show();
	app->Run();
	delete stream;
	delete app;
	return 0;
}

/* ---------------------------------------------------------------- */

BEntry *archive_file(bool create)
{
	BPath		path;
	BEntry		*entry = NULL;
//	long		err;

	
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return NULL;
	path.Append ("Container_data");

	if (create) {
		int	fd;
		fd  = open(path.Path(), O_RDWR);
		if (fd < 0)
			fd = creat(path.Path(), 0777);
		if (fd > 0)
			close(fd);
	}

	entry = new BEntry(path.Path());
	if (entry->InitCheck() != B_NO_ERROR) {
		delete entry;
		entry = NULL;
	}
	return entry;
}

/*------------------------------------------------------------*/

TWindow::TWindow(BRect frame, const char *title, BPositionIO *stream)
	: BWindow(frame, title, B_TITLED_WINDOW, 0)
{
	BRect	b;
	Lock();

	b = frame;
	b.OffsetTo(B_ORIGIN);

	BView *parent = new BBox(b, "parent", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	parent->SetViewColor(216, 216, 216, 0);
	AddChild(parent);

	fArchiveStream = stream;

	b.InsetBy(5,5);
	fMainView = new BBox(b, "MainView", B_FOLLOW_ALL);
	fMainView->SetViewColor(216, 216, 216, 0);
	fMainView->SetLabel("The Drop Zone");
	parent->AddChild(fMainView);

	fShelf = new BShelf(fArchiveStream, fMainView);
	fShelf->SetDisplaysZombies(true);
	fArchiveStream->Seek(0, SEEK_SET);

	Unlock();
}

/*------------------------------------------------------------*/
TWindow::~TWindow()
{
}

/*------------------------------------------------------------*/

void TWindow::Quit()
{
	delete fShelf;	// by deleting the Shelf we'll save the state
	fShelf = NULL;
	BWindow::Quit();
}

/*------------------------------------------------------------*/

bool TWindow::QuitRequested()
{
	long c = be_app->CountWindows();

	if (c == 1) {
		be_app->PostMessage(B_QUIT_REQUESTED);

//		BFile	*file;
		if (fArchiveStream) {
			long	err;
			long	d1 = MAGIC_1;
			long	d2 = MAGIC_2;
			err = fArchiveStream->Write(&d1, sizeof(d1));
			err = fArchiveStream->Write(&d2, sizeof(d2));

			BRect frame = Frame();
			err = fArchiveStream->Write(&frame, sizeof(frame));

//+			delete fShelf;	// by deleting the Shelf we'll save the state
		}
	}
	return TRUE;
}
