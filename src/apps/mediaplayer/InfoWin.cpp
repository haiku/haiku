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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <Catalog.h>
#include <Debug.h>
#include <MediaDefs.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <String.h>
#include <StringView.h>
#include <TextView.h>

#include "Controller.h"
#include "ControllerObserver.h"
#include "PlaylistItem.h"


#define NAME "File Info"
#define MIN_WIDTH 400

#define BASE_HEIGHT (32 + 32)

//const rgb_color kGreen = { 152, 203, 152, 255 };
const rgb_color kRed =   { 203, 152, 152, 255 };
const rgb_color kBlue =  {   0,   0, 220, 255 };
const rgb_color kGreen = { 171, 221, 161, 255 };
const rgb_color kBlack = {   0,   0,   0, 255 };

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "MediaPlayer-InfoWin"

// should later draw an icon
class InfoView : public BView {
public:
						InfoView(BRect frame, const char* name, float divider);
	virtual				~InfoView();
	virtual	void		Draw(BRect updateRect);

			status_t	SetIcon(const PlaylistItem* item);
			status_t	SetIcon(const char* mimeType);
			void		SetGenericIcon();

private:
			float		fDivider;
			BBitmap*	fIconBitmap;
};


InfoView::InfoView(BRect frame, const char *name, float divider)
	: BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	  fDivider(divider),
	  fIconBitmap(NULL)
{
	BRect rect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1);

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	fIconBitmap = new BBitmap(rect, B_RGBA32);
#else
	fIconBitmap = new BBitmap(rect, B_CMAP8);
#endif
}


InfoView::~InfoView()
{
	delete fIconBitmap;
}

void
InfoView::Draw(BRect updateRect)
{
	SetHighColor(kGreen);
	BRect r(Bounds());
	r.right = r.left + fDivider;
	FillRect(r);

	if (fIconBitmap) {
		float left = r.left + ( r.right - r.left ) / 2 - B_LARGE_ICON / 2;
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fIconBitmap, BPoint(left, r.top + B_LARGE_ICON / 2));
	}

	SetHighColor(ui_color(B_DOCUMENT_TEXT_COLOR));
	r.left = r.right;
	FillRect(r);
}


status_t
InfoView::SetIcon(const PlaylistItem* item)
{
	return item->GetIcon(fIconBitmap, B_LARGE_ICON);
}


status_t
InfoView::SetIcon(const char* mimeTypeString)
{
	if (!mimeTypeString)
		return B_BAD_VALUE;

	// get type icon
	BMimeType mimeType(mimeTypeString);
	status_t status = mimeType.GetIcon(fIconBitmap, B_LARGE_ICON);

	// get supertype icon
	if (status != B_OK) {
		BMimeType superType;
		status = mimeType.GetSupertype(&superType);
		if (status == B_OK)
			status = superType.GetIcon(fIconBitmap, B_LARGE_ICON);
	}

	return status;
}


void
InfoView::SetGenericIcon()
{
	// get default icon
	BMimeType genericType(B_FILE_MIME_TYPE);
	if (genericType.GetIcon(fIconBitmap, B_LARGE_ICON) != B_OK) {
		// clear bitmap
		uint8 transparent = 0;
		if (fIconBitmap->ColorSpace() == B_CMAP8)
			transparent = B_TRANSPARENT_MAGIC_CMAP8;

		memset(fIconBitmap->Bits(), transparent, fIconBitmap->BitsLength());
	}
}


// #pragma mark -


InfoWin::InfoWin(BPoint leftTop, Controller* controller)
	:
	BWindow(BRect(leftTop.x, leftTop.y, leftTop.x + MIN_WIDTH - 1,
		leftTop.y + 300), B_TRANSLATE(NAME), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fController(controller),
	fControllerObserver(new ControllerObserver(this,
		OBSERVE_FILE_CHANGES | OBSERVE_TRACK_CHANGES | OBSERVE_STAT_CHANGES))
{
	BRect rect = Bounds();

	// accomodate for big fonts
	float div = max_c(2 * 32, be_plain_font->StringWidth(
		B_TRANSLATE("Display Mode")) + 10);

	fInfoView = new InfoView(rect, "background", div);
	fInfoView->SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
	AddChild(fInfoView);

	BFont bigFont(be_plain_font);
	bigFont.SetSize(bigFont.Size() + 6);
	font_height fh;
	bigFont.GetHeight(&fh);
	fFilenameView = new BStringView(
		BRect(div + 10, 20, rect.right - 10, 20 + fh.ascent + 5),
		"filename", "");
	AddChild(fFilenameView);
	fFilenameView->SetFont(&bigFont);
	fFilenameView->SetViewColor(fInfoView->ViewColor());
	fFilenameView->SetLowColor(fInfoView->ViewColor());
#ifdef B_BEOS_VERSION_DANO /* maybe we should support that as well ? */
	fFilenameView->SetTruncation(B_TRUNCATE_END);
#endif

	rect.top = BASE_HEIGHT;

	BRect lr(rect);
	BRect cr(rect);
	lr.right = div - 1;
	cr.left = div + 1;
	BRect tr;
	tr = lr.OffsetToCopy(0, 0).InsetByCopy(5, 1);
	fLabelsView = new BTextView(lr, "labels", tr, B_FOLLOW_BOTTOM);
	fLabelsView->SetViewColor(kGreen);
	fLabelsView->SetAlignment(B_ALIGN_RIGHT);
	fLabelsView->SetWordWrap(false);
	AddChild(fLabelsView);
	tr = cr.OffsetToCopy(0, 0).InsetByCopy(10, 1);
	fContentsView = new BTextView(cr, "contents", tr, B_FOLLOW_BOTTOM);
	fContentsView->SetWordWrap(false);
	AddChild(fContentsView);

	fLabelsView->MakeSelectable();
	fContentsView->MakeSelectable();

	fController->AddListener(fControllerObserver);
	Update();

	Show();
}


InfoWin::~InfoWin()
{
	fController->RemoveListener(fControllerObserver);
	delete fControllerObserver;
}


// #pragma mark -


void
InfoWin::FrameResized(float newWidth, float newHeight)
{
}


void
InfoWin::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_CONTROLLER_FILE_FINISHED:
			break;
		case MSG_CONTROLLER_FILE_CHANGED:
			Update(INFO_ALL);
			break;
		case MSG_CONTROLLER_VIDEO_TRACK_CHANGED:
			Update(/*INFO_VIDEO | INFO_STATS*/INFO_ALL);
			break;
		case MSG_CONTROLLER_AUDIO_TRACK_CHANGED:
			Update(/*INFO_AUDIO | INFO_STATS*/INFO_ALL);
			break;
		case MSG_CONTROLLER_VIDEO_STATS_CHANGED:
		case MSG_CONTROLLER_AUDIO_STATS_CHANGED:
			Update(/*INFO_STATS*/INFO_ALL);
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


bool
InfoWin::QuitRequested()
{
	Hide();
	return false;
}


void
InfoWin::Pulse()
{
	if (IsHidden())
		return;
	Update(INFO_STATS);
}


// #pragma mark -


void
InfoWin::ResizeToPreferred()
{
}


void
InfoWin::Update(uint32 which)
{
printf("InfoWin::Update(0x%08lx)\n", which);
	fLabelsView->SetText("");
	fContentsView->SetText("");
	fLabelsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kBlue);
	fLabelsView->Insert(" ");
	fContentsView->SetFontAndColor(be_plain_font, B_FONT_ALL);
//	fContentsView->Insert("");

	if (!fController->Lock())
		return;

	fLabelsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kRed);

	status_t err;
	// video track format information
	if ((which & INFO_VIDEO) && fController->VideoTrackCount() > 0) {
		BString label = B_TRANSLATE("Video");
		fLabelsView->Insert(label << "\n\n\n");
		BString s;
		media_format format;
		media_raw_video_format videoFormat;
		err = fController->GetEncodedVideoFormat(&format);
		if (err < B_OK) {
			s << "(" << strerror(err) << ")\n";
		} else if (format.type == B_MEDIA_ENCODED_VIDEO) {
			videoFormat = format.u.encoded_video.output;
			media_codec_info mci;
			err = fController->GetVideoCodecInfo(&mci);
			if (err < B_OK) {
				s << B_TRANSLATE("Haiku Media Kit: ") << strerror(err);
				if (format.user_data_type == B_CODEC_TYPE_INFO) {
					s << (char *)format.user_data << " "
						<< B_TRANSLATE("(not supported)");
				}
			} else
				s << mci.pretty_name; //<< "(" << mci.short_name << ")";
		} else if (format.type == B_MEDIA_RAW_VIDEO) {
			videoFormat = format.u.raw_video;
			s << B_TRANSLATE("raw video");
		} else
			s << B_TRANSLATE("unknown format");
		s << "\n";
		s << format.Width() << " x " << format.Height();
		// encoded has output as 1st field...
		char fpsString[20];
		snprintf(fpsString, sizeof(fpsString), B_TRANSLATE("%f fps"),
			videoFormat.field_rate);
		s << ", " << fpsString << "\n\n";
		fContentsView->Insert(s.String());
	}

	// audio track format information
	if ((which & INFO_AUDIO) && fController->AudioTrackCount() > 0) {
		BString label = B_TRANSLATE("Audio");
		fLabelsView->Insert(label << "\n\n\n");
		BString s;
		media_format format;
		media_raw_audio_format audioFormat;
		err = fController->GetEncodedAudioFormat(&format);
		//string_for_format(format, buf, sizeof(buf));
		//printf("%s\n", buf);
		if (err < 0) {
			s << "(" << strerror(err) << ")\n";
		} else if (format.type == B_MEDIA_ENCODED_AUDIO) {
			audioFormat = format.u.encoded_audio.output;
			media_codec_info mci;
			err = fController->GetAudioCodecInfo(&mci);
			if (err < 0) {
				s << B_TRANSLATE("Haiku Media Kit: ") << strerror(err);
				if (format.user_data_type == B_CODEC_TYPE_INFO) {
					s << (char *)format.user_data << " "
						<< B_TRANSLATE("(not supported)");
				}
			} else
				s << mci.pretty_name; //<< "(" << mci.short_name << ")";
		} else if (format.type == B_MEDIA_RAW_AUDIO) {
			audioFormat = format.u.raw_audio;
			s << B_TRANSLATE("raw audio");
		} else
			s << B_TRANSLATE("unknown format");
		s << "\n";
		uint32 bitsPerSample = 8 * (audioFormat.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK);
		uint32 channelCount = audioFormat.channel_count;
		float sr = audioFormat.frame_rate;

		if (bitsPerSample > 0) {
			char bitString[20];
			snprintf(bitString, sizeof(bitString), B_TRANSLATE("%d Bit"),
				bitsPerSample);
			s << bitString << " ";
		}
		if (channelCount == 1)
			s << B_TRANSLATE("Mono");
		else if (channelCount == 2)
			s << B_TRANSLATE("Stereo");
		else {
			char channelString[20];
			snprintf(channelString, sizeof(channelString),
				B_TRANSLATE("%d Channels"), channelCount);
			s << channelString;
		}
		s << ", ";
		if (sr > 0.0) {
			char rateString[20];
			snprintf(rateString, sizeof(rateString),
				B_TRANSLATE("%d kHz"), sr / 1000);
			s << rateString;
		} else {
			BString rateString = B_TRANSLATE("%d kHz");
			rateString.ReplaceFirst("%d", "??");
			s << rateString;
		}
		s << "\n\n";
		fContentsView->Insert(s.String());
	}

	// statistics
	if ((which & INFO_STATS) && fController->HasFile()) {
		BString label = B_TRANSLATE("Duration");
		fLabelsView->Insert(label << "\n");
		BString s;
		bigtime_t d = fController->TimeDuration();
		bigtime_t v;

		//s << d << "µs; ";

		d /= 1000;

		v = d / (3600 * 1000);
		d = d % (3600 * 1000);
		bool hours = v > 0;
		if (hours)
			s << v << ":";
		v = d / (60 * 1000);
		d = d % (60 * 1000);
		s << v << ":";
		v = d / 1000;
		s << v;
		if (hours)
			s << " " << B_TRANSLATE_COMMENT("h", "Hours");
		else
			s << " " << B_TRANSLATE_COMMENT("min", "Minutes");
		s << "\n";
		fContentsView->Insert(s.String());
		// TODO: demux/video/audio/... perfs (Kb/s)

		BString content = B_TRANSLATE("Display mode");
		fLabelsView->Insert(content << "\n");
		if (fController->IsOverlayActive()) {
			content = B_TRANSLATE("Overlay");
			fContentsView->Insert(content << "\n");
		} else {
			content = B_TRANSLATE("DrawBitmap");
			fContentsView->Insert(content << "\n");
		}

		fLabelsView->Insert("\n");
		fContentsView->Insert("\n");
	}

	if (which & INFO_TRANSPORT) {
		// Transport protocol info (file, http, rtsp, ...)
	}

	if (which & INFO_FILE) {
		bool iconSet = false;
		if (fController->HasFile()) {
			const PlaylistItem* item = fController->Item();
			iconSet = fInfoView->SetIcon(item) == B_OK;
			media_file_format fileFormat;
			BString s;
			if (fController->GetFileFormatInfo(&fileFormat) == B_OK) {
				BString label = B_TRANSLATE("Container");
				fLabelsView->Insert(label << "\n");
				s << fileFormat.pretty_name;
				s << "\n";
				fContentsView->Insert(s.String());
				if (!iconSet)
					iconSet = fInfoView->SetIcon(fileFormat.mime_type) == B_OK;
			} else
				fContentsView->Insert("\n");
			BString label = B_TRANSLATE("Location");
			fLabelsView->Insert(label << "\n");
			if (fController->GetLocation(&s) < B_OK)
				s = B_TRANSLATE("<unknown>");
			s << "\n";
			fContentsView->Insert(s.String());
			if (fController->GetName(&s) < B_OK)
				s = B_TRANSLATE("<unnamed media>");
			fFilenameView->SetText(s.String());
		} else {
			fFilenameView->SetText(B_TRANSLATE("<no media>"));
		}
		if (!iconSet)
			fInfoView->SetGenericIcon();
	}

	if ((which & INFO_COPYRIGHT) && fController->HasFile()) {
		BString s;
		if (fController->GetCopyright(&s) == B_OK && s.Length() > 0) {
			BString label = B_TRANSLATE("Copyright");
			fLabelsView->Insert(label << "\n\n");
			s << "\n\n";
			fContentsView->Insert(s.String());
		}
	}

	fController->Unlock();

	ResizeToPreferred();
}
