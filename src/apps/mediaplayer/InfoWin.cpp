/*
 * InfoWin.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include "InfoWin.h"

#include <View.h>
#include <stdio.h>
#include <string.h>
#include <String.h>
#include <Debug.h>
#include <MediaDefs.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <TextView.h>
#include <math.h>
#include "MainWin.h"

#define NAME "File Info"
#define MIN_WIDTH 350

#define BASE_HEIGHT (32+32)

//const rgb_color kGreen = { 152, 203, 152, 255 };
const rgb_color kRed =   { 203, 152, 152, 255 };
const rgb_color kBlue =  {   0,   0, 220, 255 };
const rgb_color kGreen = { 171, 221, 161, 255 };
const rgb_color kBlack = {   0,   0,   0, 255 };


// should later draw an icon
class InfoView : public BView {
public:
	InfoView(BRect frame, const char *name, float divider)
		: BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
		, fDivider(divider)
		{ }
	virtual ~InfoView()
		{ }
	void Draw(BRect updateRect);
	float fDivider;
};


void
InfoView::Draw(BRect updateRect)
{
	SetHighColor(kGreen);
	BRect r(Bounds());
	r.right = r.left + fDivider;
	FillRect(r);
	SetHighColor(ui_color(B_DOCUMENT_TEXT_COLOR));
	r.left = r.right;
	FillRect(r);
}


InfoWin::InfoWin(MainWin *mainWin)
	: BWindow(BRect(100,100,100+MIN_WIDTH-1,350), NAME, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE /* | B_WILL_ACCEPT_FIRST_CLICK */),
	  fMainWin(mainWin),
	  fController()
{
	BRect rect;
	if (fMainWin->Lock()) {
		rect = fMainWin->Frame();
		MoveTo(rect.right, rect.top);
		fMainWin->Unlock();
	}
	
	rect = Bounds();

	// accomodate for big fonts
	float div;
	div = MAX(2*32, be_plain_font->StringWidth("Container") + 5);

	fInfoView = new InfoView(rect, "background", div);
	fInfoView->SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
	AddChild(fInfoView);

	BFont bigFont(be_plain_font);
	bigFont.SetSize(bigFont.Size()+6);
	font_height fh;
	bigFont.GetHeight(&fh);
	fFilenameView = new BStringView(BRect(div+10, 20,
										  rect.right - 10,
										  20 + fh.ascent + 5),
									"filename", "");
	fFilenameView->SetFont(&bigFont);
	fFilenameView->SetViewColor(fInfoView->ViewColor());
	fFilenameView->SetLowColor(fInfoView->ViewColor());
#ifdef B_BEOS_VERSION_DANO /* maybe we should support that as well ? */
	fFilenameView->SetTruncation(B_TRUNCATE_END);
#endif
	AddChild(fFilenameView);
									
	
	rect.top = BASE_HEIGHT;

	BRect lr(rect);
	BRect cr(rect);
	lr.right = div - 1;
	cr.left = div + 1;
	BRect tr;
	tr = lr.OffsetToCopy(0,0).InsetByCopy(1,1);
	fLabelsView = new BTextView(lr, "labels", tr, B_FOLLOW_BOTTOM);
	fLabelsView->SetViewColor(kGreen);
	fLabelsView->SetAlignment(B_ALIGN_RIGHT);
	fLabelsView->SetWordWrap(false);
	AddChild(fLabelsView);
	tr = cr.OffsetToCopy(0,0).InsetByCopy(1,1);
	fContentsView = new BTextView(cr, "contents", tr, B_FOLLOW_BOTTOM);
	fContentsView->SetWordWrap(false);
	AddChild(fContentsView);

	fLabelsView->MakeSelectable();
	fContentsView->MakeSelectable();


	Show();
}


InfoWin::~InfoWin()
{
	printf("InfoWin::~InfoWin\n");
	//fInfoListView->MakeEmpty();
	//delete [] fInfoItems;
}


void
InfoWin::ResizeToPreferred()
{
#if 0
	int i;
	float height = BASE_HEIGHT;
	BListItem *li;
	for (i = 0; (li = fInfoListView->ItemAt(i)); i++) {
		height += li->Height();
	}
	ResizeTo(Bounds().Width(), height);
#endif
}


void
InfoWin::Update(uint32 which)
{
	status_t err;
	//char buf[256];
	printf("InfoWin::Update(0x%08lx)\n", which);
	rgb_color vFgCol = ui_color(B_DOCUMENT_TEXT_COLOR);

	fLabelsView->SelectAll();
	fContentsView->SelectAll();
	fLabelsView->Clear();
	fContentsView->Clear();
	fLabelsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kBlue);
	fLabelsView->Insert("File Info\n");
	fContentsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &vFgCol);
	fContentsView->Insert("\n");

	fLabelsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kRed);
	//fContentsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &vFgCol);

	// lock the Main Window as we must access some fields there...
	if (fMainWin->LockWithTimeout(500000) < B_OK)
		return; // XXX: resend msg to ourselves ?

	Controller *c = fMainWin->fController;
	BMediaFile *mf = c->fMediaFile;
	
	if (which & INFO_VIDEO && c->VideoTrackCount() > 0) {
		fLabelsView->Insert("Video\n\n");
		BString s;
		media_format fmt;
		media_raw_video_format vfmt;
		float fps;
		err = c->fVideoTrack->EncodedFormat(&fmt);
		//string_for_format(fmt, buf, sizeof(buf));
		//printf("%s\n", buf);
		if (err < 0) {
			s << "(" << strerror(err) << ")";
		} else if (fmt.type == B_MEDIA_ENCODED_VIDEO) {
			vfmt = fmt.u.encoded_video.output;
			media_codec_info mci;
			err = c->fVideoTrack->GetCodecInfo(&mci);
			if (err < 0)
				s << "(" << strerror(err) << ")";
			else
				s << mci.pretty_name; //<< "(" << mci.short_name << ")";
		} else if (fmt.type == B_MEDIA_RAW_VIDEO) {
			vfmt = fmt.u.raw_video;
			s << "raw video";
		} else
			s << "unknown format";
		s << "\n";
		s << fmt.Width() << " x " << fmt.Height();
		// encoded has output as 1st field...
		fps = vfmt.field_rate;
		s << ", " << fps << " fps";
		s << "\n";
		fContentsView->Insert(s.String());
	}
	if (which & INFO_AUDIO && c->AudioTrackCount() > 0) {
		fLabelsView->Insert("Sound\n\n");
		BString s;
		media_format fmt;
		media_raw_audio_format afmt;
		err = c->fAudioTrack->EncodedFormat(&fmt);
		//string_for_format(fmt, buf, sizeof(buf));
		//printf("%s\n", buf);
		if (err < 0) {
			s << "(" << strerror(err) << ")";
		} else if (fmt.type == B_MEDIA_ENCODED_AUDIO) {
			afmt = fmt.u.encoded_audio.output;
			media_codec_info mci;
			err = c->fAudioTrack->GetCodecInfo(&mci);
			if (err < 0)
				s << "(" << strerror(err) << ")";
			else
				s << mci.pretty_name; //<< "(" << mci.short_name << ")";
		} else if (fmt.type == B_MEDIA_RAW_AUDIO) {
			afmt = fmt.u.raw_audio;
			s << "raw audio";
		} else
			s << "unknown format";
		s << "\n";
		uint32 bitps = 8 * (afmt.format & media_raw_audio_format::B_AUDIO_SIZE_MASK);
		uint32 chans = afmt.channel_count;
		float sr = afmt.frame_rate;

		if (bitps)
			s << bitps << " Bit ";
		if (chans == 1)
			s << "Mono";
		else if (chans == 2)
			s << "Stereo";
		else
			s << chans << "Channels";
		s << ", ";
		if (sr)
			s << sr/1000;
		else
			s << "?";
		s<< " kHz";
		s << "\n";
		fContentsView->Insert(s.String());
	}
	if (which & INFO_STATS && fMainWin->fHasFile) {
		// TODO: check for live streams (no duration)
		fLabelsView->Insert("Duration\n");
		BString s;
		bigtime_t d = c->Duration();
		bigtime_t v;

		//s << d << "µs; ";
		
		d /= 1000;
		
		v = d / (3600 * 1000);
		d = d % (3600 * 1000);
		if (v)
			s << v << ":";
		v = d / (60 * 1000);
		d = d % (60 * 1000);
		s << v << ":";
		v = d / 1000;
		d = d % 1000;
		s << v;
		if (d)
			s << "." << d / 10;
		s << "\n";
		fContentsView->Insert(s.String());
		// TODO: demux/video/audio/... perfs (Kb/s)
	}
	if (which & INFO_TRANSPORT) {
	}
	if ((which & INFO_FILE) && fMainWin->fHasFile) {
		media_file_format ff;
		if (mf && (mf->GetFileFormatInfo(&ff) == B_OK)) {
			fLabelsView->Insert("Container\n");
			BString s;
			s << ff.pretty_name;
			s << "\n";
			fContentsView->Insert(s.String());
		}
		fLabelsView->Insert("Location\n");
		// TODO: make Controller save the entry_ref (url actually).
		fContentsView->Insert("file://\n");
		fFilenameView->SetText(c->fName.String());
	}
	if (which & INFO_COPYRIGHT && mf && mf->Copyright()) {
		
		fLabelsView->Insert("Copyright\n\n");
		BString s;
		s << mf->Copyright();
		s << "\n\n";
		fContentsView->Insert(s.String());
	}

	// we can unlock the main window now and let it work
	fMainWin->Unlock();
	
	// now resize the window to the list view size...
	ResizeToPreferred();

	if (IsHidden())
		Show();
}


bool
InfoWin::QuitRequested()
{
	Hide();
	return false;
}


void
InfoWin::Show()
{
	// notify the main window first
	fMainWin->fInfoWinShowing = true;
	BWindow::Show();
	//SetPulseRate(1000000);
}


void
InfoWin::Hide()
{
	SetPulseRate(0);
	BWindow::Hide();
	fMainWin->fInfoWinShowing = false;
}


void
InfoWin::Pulse()
{
	if (IsHidden())
		return;
	Update(INFO_STATS);
}

void
InfoWin::FrameResized(float new_width, float new_height)
{
#if 0
	if (new_width != Bounds().Width() || new_height != Bounds().Height()) {
		debugger("size wrong\n");
	}
	
	bool no_menu = fNoMenu || fIsFullscreen;
	bool no_controls = fNoControls || fIsFullscreen;
	
	printf("FrameResized enter: new_width %.0f, new_height %.0f\n", new_width, new_height);
	
	int max_video_width  = int(new_width) + 1;
	int max_video_height = int(new_height) + 1 - (no_menu  ? 0 : fMenuBarHeight) - (no_controls ? 0 : fControlsHeight);

	ASSERT(max_video_height >= 0);
	
	int y = 0;
	
	if (no_menu) {
		if (!fMenuBar->IsHidden())
			fMenuBar->Hide();
	} else {
//		fMenuBar->MoveTo(0, y);
		fMenuBar->ResizeTo(new_width, fMenuBarHeight - 1);
		if (fMenuBar->IsHidden())
			fMenuBar->Show();
		y += fMenuBarHeight;
	}
	
	if (max_video_height == 0) {
		if (!fVideoView->IsHidden())
			fVideoView->Hide();
	} else {
//		fVideoView->MoveTo(0, y);
//		fVideoView->ResizeTo(max_video_width - 1, max_video_height - 1);
		ResizeVideoView(0, y, max_video_width, max_video_height);
		if (fVideoView->IsHidden())
			fVideoView->Show();
		y += max_video_height;
	}
	
	if (no_controls) {
		if (!fControls->IsHidden())
			fControls->Hide();
	} else {
		fControls->MoveTo(0, y);
		fControls->ResizeTo(new_width, fControlsHeight - 1);
		if (fControls->IsHidden())
			fControls->Show();
//		y += fControlsHeight;
	}
#endif
	
	printf("FrameResized leave\n");
}


void
InfoWin::MessageReceived(BMessage *msg)
{
	uint32 which;
	switch (msg->what) {
	case M_UPDATE_INFO:
		if (msg->FindInt32("which", (int32 *)&which) < B_OK)
			which = INFO_ALL;
		Update(which);
		break;
	default:
		BWindow::MessageReceived(msg);
		break;
	}
}
