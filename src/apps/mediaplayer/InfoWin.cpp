/*
 * InfoWin.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright 2015 Axel Dörfler <axeld@pinc-software.de>
 *
 * Released under the terms of the MIT license.
 */


#include "InfoWin.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Debug.h>
#include <LayoutBuilder.h>
#include <MediaDefs.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Screen.h>
#include <String.h>
#include <StringFormat.h>
#include <StringForRate.h>
#include <StringView.h>
#include <TextView.h>

#include "Controller.h"
#include "ControllerObserver.h"
#include "PlaylistItem.h"


#define MIN_WIDTH 500


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaPlayer-InfoWin"


class IconView : public BView {
public:
								IconView(const char* name, int32 iconSize);
	virtual						~IconView();

			status_t			SetIcon(const PlaylistItem* item);
			status_t			SetIcon(const char* mimeType);
			void				SetGenericIcon();

	virtual	void				GetPreferredSize(float* _width, float* _height);
	virtual void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);

private:
			BBitmap*			fIconBitmap;
};


IconView::IconView(const char* name, int32 iconSize)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fIconBitmap(NULL)
{
	fIconBitmap = new BBitmap(BRect(0, 0, iconSize - 1, iconSize - 1),
		B_RGBA32);
	SetExplicitMaxSize(PreferredSize());
}


IconView::~IconView()
{
	delete fIconBitmap;
}


status_t
IconView::SetIcon(const PlaylistItem* item)
{
	return item->GetIcon(fIconBitmap, B_LARGE_ICON);
}


status_t
IconView::SetIcon(const char* mimeTypeString)
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
IconView::SetGenericIcon()
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


void
IconView::GetPreferredSize(float* _width, float* _height)
{
	if (_width != NULL) {
		*_width = fIconBitmap->Bounds().Width()
			+ 2 * be_control_look->DefaultItemSpacing();
	}
	if (_height != NULL) {
		*_height = fIconBitmap->Bounds().Height()
			+ 2 * be_control_look->DefaultItemSpacing();
	}
}


void
IconView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}


void
IconView::Draw(BRect updateRect)
{
	BRect rect(Bounds());

	if (fIconBitmap != NULL) {
		// Draw bitmap centered within the view
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fIconBitmap, BPoint(rect.left
				+ (rect.Width() - fIconBitmap->Bounds().Width()) / 2,
			rect.top + (rect.Height() - fIconBitmap->Bounds().Height()) / 2));
	}
}


// #pragma mark -


InfoWin::InfoWin(BPoint leftTop, Controller* controller)
	:
	BWindow(BRect(leftTop.x, leftTop.y, leftTop.x + MIN_WIDTH - 1,
		leftTop.y + 300), B_TRANSLATE("File info"), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE),
	fController(controller),
	fControllerObserver(new ControllerObserver(this,
		OBSERVE_FILE_CHANGES | OBSERVE_TRACK_CHANGES | OBSERVE_STAT_CHANGES))
{
	fIconView = new IconView("background", B_LARGE_ICON);

	fFilenameView = _CreateInfo("filename");
	BFont bigFont(be_plain_font);
	bigFont.SetSize(bigFont.Size() * 1.5f);
	fFilenameView->SetFont(&bigFont);

	// Create info views

	BStringView* containerLabel = _CreateLabel("containerLabel",
		B_TRANSLATE("Container"));
	fContainerInfo = _CreateInfo("container");

	fVideoSeparator = _CreateSeparator();
	fVideoLabel = _CreateLabel("videoLabel", B_TRANSLATE("Video"));
	fVideoFormatInfo = _CreateInfo("videoFormat");
	fVideoConfigInfo = _CreateInfo("videoConfig");
	fDisplayModeLabel = _CreateLabel("displayModeLabel",
		B_TRANSLATE("Display mode"));
	fDisplayModeInfo = _CreateInfo("displayMode");

	fAudioSeparator = _CreateSeparator();
	fAudioLabel = _CreateLabel("audioLabel", B_TRANSLATE("Audio"));
	fAudioFormatInfo = _CreateInfo("audioFormat");
	fAudioConfigInfo = _CreateInfo("audioConfig");

	BStringView* durationLabel = _CreateLabel("durationLabel",
		B_TRANSLATE("Duration"));
	fDurationInfo = _CreateInfo("duration");

	BStringView* locationLabel = _CreateLabel("locationLabel",
		B_TRANSLATE("Location"));
	fLocationInfo = _CreateInfo("location");

	fCopyrightSeparator = _CreateSeparator();
	fCopyrightLabel = _CreateLabel("copyrightLabel", B_TRANSLATE("Copyright"));
	fCopyrightInfo = _CreateInfo("copyright");

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(fIconView, 0)
			.Add(fFilenameView, 1)
			.End()
		.AddGrid(2, 13)
			.Add(containerLabel, 0, 0)
			.Add(fContainerInfo, 1, 0)
			.Add(fVideoSeparator, 0, 1)
			.Add(fVideoLabel, 0, 2)
			.Add(fVideoFormatInfo, 1, 2)
			.Add(fVideoConfigInfo, 1, 3)
			.Add(fDisplayModeLabel, 0, 4)
			.Add(fDisplayModeInfo, 1, 4)
			.Add(fAudioSeparator, 0, 5)
			.Add(fAudioLabel, 0, 6)
			.Add(fAudioFormatInfo, 1, 6)
			.Add(fAudioConfigInfo, 1, 7)
			.Add(_CreateSeparator(), 0, 8)
			.Add(durationLabel, 0, 9)
			.Add(fDurationInfo, 1, 9)
			.Add(_CreateSeparator(), 0, 10)
			.Add(locationLabel, 0, 11)
			.Add(fLocationInfo, 1, 11)
			.Add(fCopyrightSeparator, 0, 12)
			.Add(fCopyrightLabel, 0, 12)
			.Add(fCopyrightInfo, 1, 12)
			.SetColumnWeight(0, 0)
			.SetColumnWeight(1, 1)
			.SetSpacing(B_USE_DEFAULT_SPACING, 0)
			.SetExplicitMinSize(BSize(MIN_WIDTH, B_SIZE_UNSET));

	fController->AddListener(fControllerObserver);
	Update();

	UpdateSizeLimits();

	// Move window on screen if needed
	BScreen screen(this);
	if (screen.Frame().bottom < Frame().bottom)
		MoveBy(0, screen.Frame().bottom - Frame().bottom);
	if (screen.Frame().right < Frame().right)
		MoveBy(0, screen.Frame().right - Frame().right);

	Show();
}


InfoWin::~InfoWin()
{
	fController->RemoveListener(fControllerObserver);
	delete fControllerObserver;
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
			Update(INFO_VIDEO | INFO_STATS);
			break;
		case MSG_CONTROLLER_AUDIO_TRACK_CHANGED:
			Update(INFO_AUDIO | INFO_STATS);
			break;
		case MSG_CONTROLLER_VIDEO_STATS_CHANGED:
		case MSG_CONTROLLER_AUDIO_STATS_CHANGED:
			Update(INFO_STATS);
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
InfoWin::Update(uint32 which)
{
	if (!fController->Lock())
		return;

	if ((which & INFO_FILE) != 0)
		_UpdateFile();

	// video track format information
	if ((which & INFO_VIDEO) != 0)
		_UpdateVideo();

	// audio track format information
	if ((which & INFO_AUDIO) != 0)
		_UpdateAudio();

	// statistics
	if ((which & INFO_STATS) != 0) {
		_UpdateDuration();
		// TODO: demux/video/audio/... perfs (Kb/info)
	}

	if ((which & INFO_TRANSPORT) != 0) {
		// Transport protocol info (file, http, rtsp, ...)
	}

	if ((which & INFO_COPYRIGHT)!=0)
		_UpdateCopyright();

	fController->Unlock();
}


void
InfoWin::_UpdateFile()
{
	bool iconSet = false;
	if (fController->HasFile()) {
		const PlaylistItem* item = fController->Item();
		iconSet = fIconView->SetIcon(item) == B_OK;
		media_file_format fileFormat;
		status_t status = fController->GetFileFormatInfo(&fileFormat);
		if (status == B_OK) {
			fContainerInfo->SetText(fileFormat.pretty_name);
			if (!iconSet)
				iconSet = fIconView->SetIcon(fileFormat.mime_type) == B_OK;
		} else
			fContainerInfo->SetText(strerror(status));

		BString info;
		if (fController->GetLocation(&info) != B_OK)
			info = B_TRANSLATE("<unknown>");
		fLocationInfo->SetText(info.String());
		fLocationInfo->SetToolTip(info.String());

		if (fController->GetName(&info) != B_OK || info.IsEmpty())
			info = B_TRANSLATE("<unnamed media>");
		fFilenameView->SetText(info.String());
		fFilenameView->SetToolTip(info.String());
	} else {
		fFilenameView->SetText(B_TRANSLATE("<no media>"));
		fContainerInfo->SetText("-");
		fLocationInfo->SetText("-");
	}

	if (!iconSet)
		fIconView->SetGenericIcon();
}


void
InfoWin::_UpdateVideo()
{
	bool visible = fController->VideoTrackCount() > 0;
	if (visible) {
		BString info;
		media_format format;
		media_raw_video_format videoFormat = {};
		status_t status = fController->GetEncodedVideoFormat(&format);
		if (status != B_OK) {
			info << "(" << strerror(status) << ")\n";
		} else if (format.type == B_MEDIA_ENCODED_VIDEO) {
			videoFormat = format.u.encoded_video.output;
			media_codec_info mci;
			status = fController->GetVideoCodecInfo(&mci);
			if (status != B_OK) {
				if (format.user_data_type == B_CODEC_TYPE_INFO) {
					info << (char *)format.user_data << " "
						<< B_TRANSLATE("(not supported)");
				} else
					info = strerror(status);
			} else
				info << mci.pretty_name; //<< "(" << mci.short_name << ")";
		} else if (format.type == B_MEDIA_RAW_VIDEO) {
			videoFormat = format.u.raw_video;
			info << B_TRANSLATE("raw video");
		} else
			info << B_TRANSLATE("unknown format");

		fVideoFormatInfo->SetText(info.String());

		info.SetToFormat(B_TRANSLATE_COMMENT("%" B_PRIu32 " × %" B_PRIu32,
			"The '×' is the Unicode multiplication sign U+00D7"),
			format.Width(), format.Height());

		// encoded has output as 1st field...
		char fpsString[20];
		snprintf(fpsString, sizeof(fpsString), B_TRANSLATE("%.3f fps"),
			videoFormat.field_rate);
		info << ", " << fpsString;

		fVideoConfigInfo->SetText(info.String());

		if (fController->IsOverlayActive())
			fDisplayModeInfo->SetText(B_TRANSLATE("Overlay"));
		else
			fDisplayModeInfo->SetText(B_TRANSLATE("DrawBitmap"));
	}

	fVideoSeparator->SetVisible(visible);
	_SetVisible(fVideoLabel, visible);
	_SetVisible(fVideoFormatInfo, visible);
	_SetVisible(fVideoConfigInfo, visible);
	_SetVisible(fDisplayModeLabel, visible);
	_SetVisible(fDisplayModeInfo, visible);
}


void
InfoWin::_UpdateAudio()
{
	bool visible = fController->AudioTrackCount() > 0;
	if (visible) {
		BString info;
		media_format format;
		media_raw_audio_format audioFormat = {};

		status_t status = fController->GetEncodedAudioFormat(&format);
		if (status != B_OK) {
			info << "(" << strerror(status) << ")\n";
		} else if (format.type == B_MEDIA_ENCODED_AUDIO) {
			audioFormat = format.u.encoded_audio.output;
			media_codec_info mci;
			status = fController->GetAudioCodecInfo(&mci);
			if (status != B_OK) {
				if (format.user_data_type == B_CODEC_TYPE_INFO) {
					info << (char *)format.user_data << " "
						<< B_TRANSLATE("(not supported)");
				} else
					info = strerror(status);
			} else
				info = mci.pretty_name;
		} else if (format.type == B_MEDIA_RAW_AUDIO) {
			audioFormat = format.u.raw_audio;
			info = B_TRANSLATE("raw audio");
		} else
			info = B_TRANSLATE("unknown format");

		fAudioFormatInfo->SetText(info.String());

		uint32 bitsPerSample = 8 * (audioFormat.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK);
		uint32 channelCount = audioFormat.channel_count;
		float sr = audioFormat.frame_rate;

		info.Truncate(0);

		if (bitsPerSample > 0) {
			char bitString[20];
			snprintf(bitString, sizeof(bitString), B_TRANSLATE("%d Bit"),
				bitsPerSample);
			info << bitString << " ";
		}

		static BStringFormat channelFormat(B_TRANSLATE(
			"{0, plural, =1{Mono} =2{Stereo} other{# Channels}}"));
		channelFormat.Format(info, channelCount);

		info << ", ";
		if (sr > 0.0) {
			char rateString[20];
			snprintf(rateString, sizeof(rateString),
				B_TRANSLATE("%.3f kHz"), sr / 1000);
			info << rateString;
		} else {
			BString rateString = B_TRANSLATE("%d kHz");
			rateString.ReplaceFirst("%d", "??");
			info << rateString;
		}
		if (format.type == B_MEDIA_ENCODED_AUDIO) {
			float br = format.u.encoded_audio.bit_rate;
			char string[20] = "";
			if (br > 0.0)
				info << ", " << string_for_rate(br, string, sizeof(string));
		}

		fAudioConfigInfo->SetText(info.String());
	}

	fAudioSeparator->SetVisible(visible);
	_SetVisible(fAudioLabel, visible);
	_SetVisible(fAudioFormatInfo, visible);
	_SetVisible(fAudioConfigInfo, visible);
}


void
InfoWin::_UpdateDuration()
{
	if (!fController->HasFile()) {
		fDurationInfo->SetText("-");
		return;
	}

	BString info;

	bigtime_t d = fController->TimeDuration() / 1000;
	bigtime_t v = d / (3600 * 1000);
	d = d % (3600 * 1000);
	bool hours = v > 0;
	if (hours)
		info << v << ":";
	v = d / (60 * 1000);
	d = d % (60 * 1000);
	info << v << ":";
	v = d / 1000;
	if (v < 10)
		info << '0';
	info << v;
	if (hours)
		info << " " << B_TRANSLATE_COMMENT("h", "Hours");
	else
		info << " " << B_TRANSLATE_COMMENT("min", "Minutes");

	fDurationInfo->SetText(info.String());
}


void
InfoWin::_UpdateCopyright()
{
	BString info;

	bool visible = fController->HasFile()
		&& fController->GetCopyright(&info) == B_OK && !info.IsEmpty();
	if (visible)
		fCopyrightInfo->SetText(info.String());

	fCopyrightSeparator->SetVisible(visible);
	_SetVisible(fCopyrightLabel, visible);
	_SetVisible(fCopyrightInfo, visible);
}


// #pragma mark -


BStringView*
InfoWin::_CreateLabel(const char* name, const char* label)
{
	static const rgb_color kLabelColor = tint_color(
		ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_3_TINT);

	BStringView* view = new BStringView(name, label);
	view->SetAlignment(B_ALIGN_RIGHT);
	view->SetHighColor(kLabelColor);

	return view;
}


BStringView*
InfoWin::_CreateInfo(const char* name)
{
	BStringView* view = new BStringView(name, "");
	view->SetExplicitMinSize(BSize(200, B_SIZE_UNSET));
	view->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	view->SetTruncation(B_TRUNCATE_SMART);

	return view;
}


BLayoutItem*
InfoWin::_CreateSeparator()
{
	return BSpaceLayoutItem::CreateVerticalStrut(
		be_control_look->ComposeSpacing(B_USE_HALF_ITEM_SPACING));
}


void
InfoWin::_SetVisible(BView* view, bool visible)
{
	bool hidden = view->IsHidden(view);
	if (hidden && visible)
		view->Show();
	else if (!hidden && !visible)
		view->Hide();
}
