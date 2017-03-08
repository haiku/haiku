// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, 2010 Stephan Aßmus. All Rights Reserved.
// Copyright 2010-2013, Haiku, Inc. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.


#include "MediaConverterWindow.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <Slider.h>
#include <StringView.h>
#include <TextControl.h>

#include "MediaFileInfoView.h"
#include "MediaFileListView.h"
#include "MessageConstants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaConverter"
#define VERSION "1.3.0"


static const unsigned int kMinSourceWidth = 12;
static const unsigned int kQualitySliderWidth = 28;
static const unsigned int kDurationWidth = 10;


// #pragma mark - DirectoryFilter


class DirectoryFilter : public BRefFilter {
public:
	DirectoryFilter() {};
	virtual bool Filter(const entry_ref* ref,
		BNode* node, struct stat_beos* st, const char* filetype)
	{
		// ToDo: Fix this properly in Tracker
		// If you create a folder, then rename it, node is NULL.
		// The BeBook says: "Note that the function is never sent an
		// abstract entry, so the node, st, and filetype arguments will
		// always be valid."
		return node == NULL ? false : node->IsDirectory();
	}
};


// #pragma mark - FileFormatMenuItem


class FileFormatMenuItem : public BMenuItem {
public:
	FileFormatMenuItem(media_file_format* format);
	virtual ~FileFormatMenuItem();

	media_file_format fFileFormat;
};


FileFormatMenuItem::FileFormatMenuItem(media_file_format* format)
	:
	BMenuItem(format->pretty_name, new BMessage(FORMAT_SELECT_MESSAGE))
{
	memcpy(&fFileFormat, format, sizeof(fFileFormat));
}


FileFormatMenuItem::~FileFormatMenuItem()
{
}


// #pragma mark - CodecMenuItem


class CodecMenuItem : public BMenuItem {
public:
	CodecMenuItem(media_codec_info* ci, uint32 message_type);
	virtual ~CodecMenuItem();

	media_codec_info fCodecInfo;
};


CodecMenuItem::CodecMenuItem(media_codec_info* ci, uint32 message_type)
	:
	BMenuItem(ci->pretty_name, new BMessage(message_type))
{
	memcpy(&fCodecInfo, ci, sizeof(fCodecInfo));
}


CodecMenuItem::~CodecMenuItem()
{
}


// #pragma mark - OutputBox


class OutputBox : public BBox {
public:
	OutputBox(border_style border, BView* child);
	virtual void FrameResized(float width, float height)
	{
		MediaConverterWindow* window
			= dynamic_cast<MediaConverterWindow*>(Window());
		if (window != NULL)
			window->TruncateOutputFolderPath();
		BBox::FrameResized(width, height);
	}
};


OutputBox::OutputBox(border_style border, BView* child)
	:
	BBox(border, child)
{
}


// #pragma mark - MediaConverterWindow


MediaConverterWindow::MediaConverterWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("MediaConverter"),
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE
		| B_NOT_V_RESIZABLE | B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS),
	fVideoQuality(75),
	fAudioQuality(75),
	fSaveFilePanel(NULL),
	fOpenFilePanel(NULL),
	fOutputDirSpecified(false),
	fEnabled(true),
	fConverting(false),
	fCancelling(false)
{
	BPath outputDir;
	if (find_directory(B_USER_DIRECTORY, &outputDir) != B_OK)
		outputDir.SetTo("/boot/home");
	fOutputDir.SetTo(outputDir.Path());

	fMenuBar = new BMenuBar("menubar");
	_CreateMenu();

	float padding = be_control_look->DefaultItemSpacing();

	fListView = new MediaFileListView();
	fListView->SetExplicitMinSize(BSize(padding * kMinSourceWidth, B_SIZE_UNSET));
	BScrollView* scroller = new BScrollView(NULL, fListView, 0, false, true);

	// file list view box
	fSourcesBox = new BBox(B_FANCY_BORDER, scroller);
	fSourcesBox->SetLayout(new BGroupLayout(B_HORIZONTAL, 0));
		// fSourcesBox's layout adjusted in _UpdateLabels

	// info box
	fInfoView = new MediaFileInfoView();
	fInfoView->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_VERTICAL_UNSET));
	fInfoBox = new BBox(B_FANCY_BORDER, fInfoView);

	// output menu fields
	fFormatMenu = new BMenuField(NULL, B_TRANSLATE("File format:"),
		new BPopUpMenu(""));
	fAudioMenu = new BMenuField(NULL, B_TRANSLATE("Audio encoding:"),
		new BPopUpMenu(""));
	fVideoMenu = new BMenuField(NULL, B_TRANSLATE("Video encoding:"),
		new BPopUpMenu(""));

	// output folder
	fDestButton = new BButton(B_TRANSLATE("Output folder"),
		new BMessage(OUTPUT_FOLDER_MESSAGE));
	BAlignment labelAlignment(be_control_look->DefaultLabelAlignment());
	fOutputFolder = new BStringView(NULL, outputDir.Path());
	fOutputFolder->SetExplicitAlignment(labelAlignment);

	// start/end duration
	fStartDurationTC = new BTextControl(NULL, "0", NULL);
	BLayoutItem* startDuration = fStartDurationTC->CreateTextViewLayoutItem();
	startDuration->SetExplicitSize(BSize(padding * kDurationWidth, B_SIZE_UNSET));
	startDuration->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));
	fEndDurationTC = new BTextControl(NULL, "0", NULL);
	BLayoutItem* endDuration = fEndDurationTC->CreateTextViewLayoutItem();
	endDuration->SetExplicitSize(BSize(padding * kDurationWidth, B_SIZE_UNSET));
	endDuration->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));

	// video quality
	fVideoQualitySlider = new BSlider("VSlider", "" ,
		new BMessage(VIDEO_QUALITY_CHANGED_MESSAGE), 1, 100, B_HORIZONTAL);
	fVideoQualitySlider->SetModificationMessage(
		new BMessage(VIDEO_QUALITY_CHANGED_MESSAGE));
	fVideoQualitySlider->SetValue(fVideoQuality);
	fVideoQualitySlider->SetEnabled(false);
	fVideoQualitySlider->SetExplicitSize(BSize(padding * kQualitySliderWidth,
		B_SIZE_UNSET));

	// audio quality
	fAudioQualitySlider = new BSlider("ASlider", "" ,
		new BMessage(AUDIO_QUALITY_CHANGED_MESSAGE), 1, 100, B_HORIZONTAL);
	fAudioQualitySlider->SetModificationMessage(
		new BMessage(AUDIO_QUALITY_CHANGED_MESSAGE));
	fAudioQualitySlider->SetValue(fAudioQuality);
	fAudioQualitySlider->SetEnabled(false);
	fAudioQualitySlider->SetExplicitSize(BSize(padding * kQualitySliderWidth,
		B_SIZE_UNSET));

	// output format box
	BView* outputGrid = BLayoutBuilder::Grid<>()
		.Add(fFormatMenu->CreateLabelLayoutItem(), 0, 0)
		.Add(fFormatMenu->CreateMenuBarLayoutItem(), 1, 0)
		.Add(fAudioMenu->CreateLabelLayoutItem(), 0, 1)
		.Add(fAudioMenu->CreateMenuBarLayoutItem(), 1, 1)
		.Add(fVideoMenu->CreateLabelLayoutItem(), 0, 2)
		.Add(fVideoMenu->CreateMenuBarLayoutItem(), 1, 2)
		.Add(fDestButton, 0, 3)
		.Add(fOutputFolder, 1, 3)
		.Add(fStartDurationTC->CreateLabelLayoutItem(), 0, 4)
		.Add(startDuration, 1, 4)
		.Add(fEndDurationTC->CreateLabelLayoutItem(), 0, 5)
		.Add(endDuration, 1, 5)
		.Add(fVideoQualitySlider, 0, 6, 2, 1)
		.Add(fAudioQualitySlider, 0, 7, 2, 1)
		.View();
	outputGrid->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_USE_FULL_HEIGHT));
	fOutputBox = new OutputBox(B_FANCY_BORDER, outputGrid);
	fOutputBox->SetLayout(new BGroupLayout(B_HORIZONTAL, 0));
		// fOutputBox's layout adjusted in _UpdateLabels

	// buttons
	fPreviewButton = new BButton(B_TRANSLATE("Preview"),
		new BMessage(PREVIEW_MESSAGE));
	fPreviewButton->SetEnabled(false);

	fConvertButton = new BButton(B_TRANSLATE("Convert"),
		new BMessage(CONVERT_BUTTON_MESSAGE));

	// Status views
	fStatus = new BStringView(NULL, NULL);
	fStatus->SetExplicitAlignment(labelAlignment);
	fFileStatus = new BStringView(NULL, NULL);
	fFileStatus->SetExplicitAlignment(labelAlignment);

	SetStatusMessage("");
	_UpdateLabels();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0, 0, 0, 0)
		.Add(fMenuBar)
		.AddSplit(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING, 0)
			.Add(fSourcesBox)
			.AddGroup(B_VERTICAL, B_USE_ITEM_SPACING)
				.Add(fInfoBox)
				.Add(fOutputBox)
			.End()
		.End()
		.AddGrid(B_USE_ITEM_SPACING)
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING)
			.Add(fStatus, 0, 0)
			.Add(fFileStatus, 0, 1)
			.Add(BSpaceLayoutItem::CreateGlue(), 1, 0)
			.Add(fPreviewButton, 2, 0)
			.Add(fConvertButton, 3, 0)
		.End();
}


MediaConverterWindow::~MediaConverterWindow()
{
	delete fSaveFilePanel;
	delete fOpenFilePanel;
}


// #pragma mark -


void
MediaConverterWindow::MessageReceived(BMessage* message)
{
	entry_ref inRef;

	char buffer[40];
	BEntry inEntry;

	switch (message->what) {
		#if B_BEOS_VERSION <= B_BEOS_VERSION_6
		case B_LANGUAGE_CHANGED:
			LanguageChanged();
			break;
		#endif

		case INIT_FORMAT_MENUS:
			BuildFormatMenu();
			if (CountSourceFiles() == 0)
				SetEnabled(false, false);
			break;

		case B_SIMPLE_DATA:
			if (message->WasDropped()) {
				DetachCurrentMessage();
				message->what = B_REFS_RECEIVED;
				BMessenger(be_app).SendMessage(message);
				delete message;
			}
			break;

		case FORMAT_SELECT_MESSAGE:
			BuildAudioVideoMenus();
			break;
		case AUDIO_CODEC_SELECT_MESSAGE:
			break;
		case VIDEO_CODEC_SELECT_MESSAGE:
			break;

		case CONVERT_BUTTON_MESSAGE:
			if (!fConverting) {
				fConvertButton->SetLabel(B_TRANSLATE("Cancel"));
				fConverting = true;
				SetStatusMessage(B_TRANSLATE("Convert"));
				SetEnabled(false, true);
				BMessenger(be_app).SendMessage(START_CONVERSION_MESSAGE);
			} else if (!fCancelling) {
				fCancelling = true;
				SetStatusMessage(B_TRANSLATE("Cancelling" B_UTF8_ELLIPSIS));
				BMessenger(be_app).SendMessage(CANCEL_CONVERSION_MESSAGE);
			}
			break;

		case CONVERSION_DONE_MESSAGE:
		{
			SetStatusMessage(fCancelling ? B_TRANSLATE("Conversion cancelled")
				: B_TRANSLATE("Conversion completed"));
			fConverting = false;
			fCancelling = false;
			bool enable = CountSourceFiles() > 0;
			SetEnabled(enable, enable);
			fConvertButton->SetLabel(B_TRANSLATE("Convert"));
			break;
		}

		case OUTPUT_FOLDER_MESSAGE:
			// Execute Save Panel
			if (fSaveFilePanel == NULL) {
				BButton* selectThisDir;

				BMessage folderSelect(FOLDER_SELECT_MESSAGE);
				fSaveFilePanel = new BFilePanel(B_OPEN_PANEL, NULL, NULL,
					B_DIRECTORY_NODE, true, &folderSelect, NULL, false, true);
				fSaveFilePanel->SetButtonLabel(B_DEFAULT_BUTTON,
					B_TRANSLATE("Select"));
				fSaveFilePanel->SetTarget(this);

				fSaveFilePanel->Window()->Lock();
				fSaveFilePanel->Window()->SetTitle(
					B_TRANSLATE("MediaConverter+:SaveDirectory"));
				BRect buttonRect
					= fSaveFilePanel->Window()->ChildAt(0)->FindView(
						"cancel button")->Frame();
				buttonRect.right  = buttonRect.left - 20;
				buttonRect.left = buttonRect.right - 130;
				selectThisDir = new BButton(buttonRect, NULL,
					B_TRANSLATE("Select this folder"),
					new BMessage(SELECT_THIS_DIR_MESSAGE),
					B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
				selectThisDir->SetTarget(this);
				fSaveFilePanel->Window()->ChildAt(0)->AddChild(selectThisDir);
				fSaveFilePanel->Window()->Unlock();

				fSaveFilePanel->SetRefFilter(new DirectoryFilter);
			}
			fSaveFilePanel->Show();
			break;

		case FOLDER_SELECT_MESSAGE:
			// "SELECT" Button at Save Panel Pushed
			fSaveFilePanel->GetNextSelectedRef(&inRef);
			inEntry.SetTo(&inRef, true);
			_SetOutputFolder(inEntry);
			fOutputDirSpecified = true;
			fSaveFilePanel->Rewind();
			break;

		case SELECT_THIS_DIR_MESSAGE:
			// "THIS DIR" Button at Save Panel Pushed
			fSaveFilePanel->GetPanelDirectory(&inRef);
			fSaveFilePanel->Hide();
			inEntry.SetTo(&inRef, true);
			_SetOutputFolder(inEntry);
			fOutputDirSpecified = true;
			break;

		case OPEN_FILE_MESSAGE:
			// Execute Open Panel
			if (!fOpenFilePanel) {
				fOpenFilePanel = new BFilePanel(B_OPEN_PANEL, NULL, NULL,
					B_FILE_NODE, true, NULL, NULL, false, true);
				fOpenFilePanel->SetTarget(this);
			}
			fOpenFilePanel->Show();
			break;

		case B_REFS_RECEIVED:
			// Media Files Seleced by Open Panel
			DetachCurrentMessage();
			message->what = B_REFS_RECEIVED;
			BMessenger(be_app).SendMessage(message);
			// fall through

		case B_CANCEL:
			break;

		case QUIT_MESSAGE:
			MediaConverterWindow::QuitRequested();
			break;

		case PREVIEW_MESSAGE:
		{
			// Build the command line to launch the preview application.
			// TODO: Launch the default app instead of hardcoded MediaPlayer!
			int32 srcIndex = fListView->CurrentSelection();
			BMediaFile* inFile = NULL;
			status_t status = GetSourceFileAt(srcIndex, &inFile, &inRef);

			const char* argv[3];
			BString startPosString;
			BPath path;

			if (status == B_OK) {
				argv[0] = "-pos";
					// NOTE: -pos argument is currently not supported by Haiku
					// MediaPlayer.
				startPosString << fStartDurationTC->Text();
				startPosString << "000";
				argv[1] = startPosString.String();

				status = inEntry.SetTo(&inRef);
			}

			if (status == B_OK) {
				status = inEntry.GetPath(&path);
				if (status == B_OK)
					argv[2] = path.Path();
			}

			if (status == B_OK) {
				status = be_roster->Launch(
					"application/x-vnd.Haiku-MediaPlayer",
					3, (char**)argv, NULL);
			}

			if (status != B_OK && status != B_ALREADY_RUNNING) {
				BString errorString(B_TRANSLATE("Error launching: %strError%"));
				errorString.ReplaceFirst("%strError%", strerror(status));
				BAlert* alert = new BAlert(B_TRANSLATE("Error"),
					errorString.String(), B_TRANSLATE("OK"));
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go();
			}
			break;
		}

		case VIDEO_QUALITY_CHANGED_MESSAGE:
		{
			int32 value;
			message->FindInt32("be:value", &value);
			snprintf(buffer, sizeof(buffer),
				B_TRANSLATE("Video quality: %3d%%"), (int8)value);
			fVideoQualitySlider->SetLabel(buffer);
			fVideoQuality = value;
			break;
		}

		case AUDIO_QUALITY_CHANGED_MESSAGE:
		{
			int32 value;
			message->FindInt32("be:value", &value);
			snprintf(buffer, sizeof(buffer),
				B_TRANSLATE("Audio quality: %3d%%"), (int8)value);
			fAudioQualitySlider->SetLabel(buffer);
			fAudioQuality = value;
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


bool
MediaConverterWindow::QuitRequested()
{
	if (!fConverting) {
		BMessenger(be_app).SendMessage(B_QUIT_REQUESTED);
		return true;
	} else if (!fCancelling) {
		fCancelling = true;
		SetStatusMessage(B_TRANSLATE("Cancelling"));
		BMessenger(be_app).SendMessage(CANCEL_CONVERSION_MESSAGE);
	}

	return false;
}


// #pragma mark -


void
MediaConverterWindow::LanguageChanged()
{
	_DestroyMenu();
	_CreateMenu();
	_UpdateLabels();
	BuildAudioVideoMenus();
	Lock();
	fInfoView->Invalidate();
	Unlock();
}


void
MediaConverterWindow::BuildAudioVideoMenus()
{
	BMenu* menu = fAudioMenu->Menu();
	BMenuItem* item;

	// clear out old audio codec menu items
	while ((item = menu->RemoveItem((int32)0)) != NULL)
		delete item;

	bool separator = true;

	// get selected file format
	FileFormatMenuItem* ffmi
		= (FileFormatMenuItem*)fFormatMenu->Menu()->FindMarked();
	media_file_format* mf_format = &(ffmi->fFileFormat);

	media_format format, outfmt;
	memset(&format, 0, sizeof(format));
	media_codec_info codec_info;
	int32 cookie = 0;
	CodecMenuItem* cmi;

	// add available audio encoders to menu
	format.type = B_MEDIA_RAW_AUDIO;
	format.u.raw_audio = media_raw_audio_format::wildcard;
	while (get_next_encoder(&cookie, mf_format, &format, &outfmt, &codec_info)
			== B_OK) {
		if (separator) {
			menu->AddItem(new BMenuItem(
				B_TRANSLATE_CONTEXT("No audio", "Audio codecs list"),
				new BMessage(AUDIO_CODEC_SELECT_MESSAGE)));
			menu->AddSeparatorItem();
			separator = false;
		}

		cmi = new CodecMenuItem(&codec_info, AUDIO_CODEC_SELECT_MESSAGE);
		menu->AddItem(cmi);
		// reset media format struct
/*
		format.type = B_MEDIA_RAW_AUDIO;
		format.u.raw_audio = media_raw_audio_format::wildcard;
*/
	}

	// mark first audio encoder
	item = menu->ItemAt(0);
	if (item != NULL) {
		fAudioMenu->SetEnabled(fEnabled);
		fAudioQualitySlider->SetEnabled(fEnabled);
		item->SetMarked(true);
		((BInvoker*)item)->Invoke();
	} else {
		item = new BMenuItem(
			B_TRANSLATE_CONTEXT("None available", "Audio codecs"), NULL);
		menu->AddItem(item);
		item->SetMarked(true);
		fAudioMenu->SetEnabled(false);
		fAudioQualitySlider->SetEnabled(false);
	}

	// clear out old video codec menu items
	menu = fVideoMenu->Menu();
	while ((item = menu->RemoveItem((int32)0)) != NULL)
		delete item;

	separator = true;

	// construct a generic video format.  Some of these parameters
	// seem silly, but are needed for R4.5.x, which is more picky
	// than subsequent BeOS releases will be.
	memset(&format, 0, sizeof(format));
	format.type = B_MEDIA_RAW_VIDEO;
	format.u.raw_video.last_active = (uint32)(240 - 1);
	format.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	format.u.raw_video.display.format = B_RGB32;
	format.u.raw_video.display.line_width = (int32)320;
	format.u.raw_video.display.line_count = (int32)240;
	format.u.raw_video.display.bytes_per_row = 4 * 320;

	// add available video encoders to menu
	cookie = 0;
	while (get_next_encoder(&cookie, mf_format, &format, &outfmt, &codec_info)
			== B_OK) {
		if (separator) {
			menu->AddItem(new BMenuItem(
				B_TRANSLATE_CONTEXT("No video", "Video codecs list"),
				new BMessage(VIDEO_CODEC_SELECT_MESSAGE)));
			menu->AddSeparatorItem();
			separator = false;
		}

		cmi = new CodecMenuItem(&codec_info, VIDEO_CODEC_SELECT_MESSAGE);
		menu->AddItem(cmi);
	}

	// mark first video encoder
	item = menu->ItemAt(0);
	if (item != NULL) {
		fVideoMenu->SetEnabled(fEnabled);
		fVideoQualitySlider->SetEnabled(fEnabled);
		item->SetMarked(true);
		((BInvoker*)item)->Invoke();
	} else {
		item = new BMenuItem(
			B_TRANSLATE_CONTEXT("None available", "Video codecs"), NULL);
		menu->AddItem(item);
		item->SetMarked(true);
		fVideoMenu->SetEnabled(false);
		fVideoQualitySlider->SetEnabled(false);
	}
}

void
MediaConverterWindow::GetSelectedFormatInfo(media_file_format** format,
	media_codec_info** audio, media_codec_info** video)
{
	*audio = NULL;
	*video = NULL;
	*format = NULL;

	FileFormatMenuItem* formatItem =
		dynamic_cast<FileFormatMenuItem*>(fFormatMenu->Menu()->FindMarked());
	if (formatItem != NULL)
		*format = &(formatItem->fFileFormat);

	*audio = *video = NULL;
	CodecMenuItem* codecItem =
		dynamic_cast<CodecMenuItem*>(fAudioMenu->Menu()->FindMarked());
	if (codecItem != NULL)
		*audio =  &(codecItem->fCodecInfo);

	codecItem = dynamic_cast<CodecMenuItem*>(fVideoMenu->Menu()->FindMarked());
	if (codecItem != NULL)
		*video =  &(codecItem->fCodecInfo);
}


void
MediaConverterWindow::BuildFormatMenu()
{
	BMenu* menu = fFormatMenu->Menu();
	BMenuItem* item;

	// clear out old format menu items
	while ((item = menu->RemoveItem((int32)0)) != NULL)
		delete item;

	// add menu items for each file format
	media_file_format mfi;
	int32 cookie = 0;
	FileFormatMenuItem* ff_item;
	while (get_next_file_format(&cookie, &mfi) == B_OK) {
		if ((mfi.capabilities & media_file_format::B_WRITABLE) == 0)
			continue;
		ff_item = new FileFormatMenuItem(&mfi);
		menu->AddItem(ff_item);
	}

	// mark first item
	item = menu->ItemAt(0);
	if (item != NULL) {
		item->SetMarked(true);
		((BInvoker*)item)->Invoke();
	}
}


void
MediaConverterWindow::SetFileMessage(const char* message)
{
	fFileStatus->SetText(message);
}


void
MediaConverterWindow::SetStatusMessage(const char* message)
{
	fStatus->SetText(message);
}


// #pragma mark -


bool
MediaConverterWindow::AddSourceFile(BMediaFile* file, const entry_ref& ref)
{
	if (!fListView->AddMediaItem(file, ref))
		return false;

	if (!fOutputDirSpecified) {
		BEntry entry(&ref);
		entry.GetParent(&entry);
		_SetOutputFolder(entry);
	}

	return true;
}


void
MediaConverterWindow::RemoveSourceFile(int32 index)
{
	delete fListView->RemoveItem(index);
	fStartDurationTC->SetText("0");
	fEndDurationTC->SetText("0");
}


int32
MediaConverterWindow::CountSourceFiles()
{
	return fListView->CountItems();
}


status_t
MediaConverterWindow::GetSourceFileAt(int32 index, BMediaFile** _file,
	entry_ref* ref)
{
	MediaFileListItem* item = dynamic_cast<MediaFileListItem*>(
		fListView->ItemAt(index));
	if (item != NULL) {
		*_file = item->fMediaFile;
		*ref = item->fRef;
		return B_OK;
	} else
		return B_ERROR;
}


void
MediaConverterWindow::SourceFileSelectionChanged()
{
	int32 selected = fListView->CurrentSelection();
	BMediaFile* file = NULL;
	entry_ref ref;
	bool enabled = GetSourceFileAt(selected, &file, &ref) == B_OK;

	fPreviewButton->SetEnabled(enabled);
	fVideoQualitySlider->SetEnabled(enabled);
	fAudioQualitySlider->SetEnabled(enabled);
	fStartDurationTC->SetEnabled(enabled);
	fEndDurationTC->SetEnabled(enabled);

	BString duration;
	if (enabled) {
		fInfoView->Update(file, &ref);
		// HACK: get the fInfoView to update the duration "synchronously"
		UpdateIfNeeded();
		duration << fInfoView->Duration() / 1000;
	} else
		duration = "0";

	// update duration text controls
	fStartDurationTC->SetText("0");
	fEndDurationTC->SetText(duration.String());
}


// #pragma mark -


void
MediaConverterWindow::SetEnabled(bool enabled, bool convertEnabled)
{
	fConvertButton->SetEnabled(convertEnabled);
	if (enabled == fEnabled)
		return;

	fFormatMenu->SetEnabled(enabled);
	fAudioMenu->SetEnabled(enabled);
	fVideoMenu->SetEnabled(enabled);
	fListView->SetEnabled(enabled);
	fStartDurationTC->SetEnabled(enabled);
	fEndDurationTC->SetEnabled(enabled);

	fEnabled = enabled;
}


bool
MediaConverterWindow::IsEnabled()
{
	return fEnabled;
}


const char*
MediaConverterWindow::StartDuration() const
{
	return fStartDurationTC->Text();
}


const char*
MediaConverterWindow::EndDuration() const
{
	return fEndDurationTC->Text();
}


BDirectory
MediaConverterWindow::OutputDirectory() const
{
	return fOutputDir;
}


void
MediaConverterWindow::SetAudioQualityLabel(const char* label)
{
	fAudioQualitySlider->SetLabel(label);
}


void
MediaConverterWindow::SetVideoQualityLabel(const char* label)
{
	fVideoQualitySlider->SetLabel(label);
}


void
MediaConverterWindow::TruncateOutputFolderPath()
{
	BEntry entry;
	fOutputDir.GetEntry(&entry);
	BPath path;
	entry.GetPath(&path);
	BString pathString(path.Path());
	float maxWidth = fVideoMenu->MenuBar()->Frame().Width();

	fOutputFolder->TruncateString(&pathString, B_TRUNCATE_MIDDLE, maxWidth);
	fOutputFolder->SetText(pathString.String());
	if (fOutputFolder->StringWidth(path.Path()) > maxWidth)
		fOutputFolder->SetToolTip(path.Path());
	else
		fOutputFolder->SetToolTip((const char*)NULL);
}


// #pragma mark -


void
MediaConverterWindow::_UpdateLabels()
{
	if (fSourcesBox != NULL) {
		fSourcesBox->SetLabel(B_TRANSLATE("Source files"));
		_UpdateBBoxLayoutInsets(fSourcesBox);
	}

	if (fInfoBox != NULL)
		fInfoBox->SetLabel(B_TRANSLATE("File details"));

	if (fOutputBox != NULL) {
		fOutputBox->SetLabel(B_TRANSLATE("Output format"));
		_UpdateBBoxLayoutInsets(fOutputBox);
	}

	if (fConvertButton != NULL)
		fConvertButton->SetLabel(B_TRANSLATE("Convert"));

	if (fPreviewButton != NULL)
		fPreviewButton->SetLabel(B_TRANSLATE("Preview"));

	if (fDestButton != NULL)
		fDestButton->SetLabel(B_TRANSLATE("Output folder"));

	if (fVideoQualitySlider != NULL) {
		char buffer[40];
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("Video quality: %3d%%"),
			(int8)fVideoQuality);
		fVideoQualitySlider->SetLabel(buffer);
		fVideoQualitySlider->SetLimitLabels(B_TRANSLATE("Low"),
			B_TRANSLATE("High"));
	}

	if (fAudioQualitySlider != NULL) {
		char buffer[40];
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("Audio quality: %3d%%"),
			(int8)fAudioQuality);
		fAudioQualitySlider->SetLabel(buffer);
		fAudioQualitySlider->SetLimitLabels(B_TRANSLATE("Low"),
			B_TRANSLATE("High"));
	}

	if (fStartDurationTC != NULL)
		fStartDurationTC->SetLabel(B_TRANSLATE("Start [ms]: "));

	if (fEndDurationTC != NULL)
		fEndDurationTC->SetLabel(B_TRANSLATE("End   [ms]: "));

	if (fFormatMenu != NULL)
		fFormatMenu->SetLabel(B_TRANSLATE("File format:"));

	if (fAudioMenu != NULL)
		fAudioMenu->SetLabel(B_TRANSLATE("Audio encoding:"));

	if (fVideoMenu != NULL)
		fVideoMenu->SetLabel(B_TRANSLATE("Video encoding:"));

	SetFileMessage(B_TRANSLATE("Drop media files onto this window"));
}


void
MediaConverterWindow::_UpdateBBoxLayoutInsets(BBox* box)
{
	BTwoDimensionalLayout* layout
		= dynamic_cast<BTwoDimensionalLayout*>(box->GetLayout());
	if (layout != NULL) {
		float padding = be_control_look->DefaultItemSpacing();
		layout->SetInsets(padding, box->TopBorderOffset() + padding, padding,
			padding);
	}
}


void
MediaConverterWindow::_DestroyMenu()
{
	BMenu* menu;

	while ((menu = fMenuBar->SubmenuAt(0)) != NULL) {
		fMenuBar->RemoveItem(menu);
		delete menu;
	}
}


void
MediaConverterWindow::_CreateMenu()
{
	BMenu* menu;
	BMenuItem* item;

	menu = new BMenu(B_TRANSLATE_CONTEXT("File", "Menu"));
	item = new BMenuItem(B_TRANSLATE_CONTEXT("Open" B_UTF8_ELLIPSIS, "Menu"),
		new BMessage(OPEN_FILE_MESSAGE), 'O');
	menu->AddItem(item);
	menu->AddSeparatorItem();
	item = new BMenuItem(B_TRANSLATE_CONTEXT("Quit", "Menu"),
		new BMessage(QUIT_MESSAGE), 'Q');
	menu->AddItem(item);

	fMenuBar->AddItem(menu);
}


void
MediaConverterWindow::_SetOutputFolder(BEntry entry)
{
	BPath path;
	entry.GetPath(&path);
	if (access(path.Path(), W_OK) != -1) {
		fOutputDir.SetTo(&entry);
	} else {
		BString errorString(B_TRANSLATE("Error writing to location: %strPath%."
			" Defaulting to location: /boot/home"));
		errorString.ReplaceFirst("%strPath%", path.Path());
		BAlert* alert = new BAlert(B_TRANSLATE("Error"),
			errorString.String(), B_TRANSLATE("OK"));
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		fOutputDir.SetTo("/boot/home");
	}
	TruncateOutputFolderPath();
}
