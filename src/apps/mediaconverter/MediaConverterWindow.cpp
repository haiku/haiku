// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#include "MediaConverterWindow.h"

#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <FilePanel.h>
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
#include "StatusView.h"
#include "Strings.h"


// #pragma mark - DirectoryFilter


class DirectoryFilter : public BRefFilter 
{ 
public: 
        DirectoryFilter(){}; 
        virtual bool Filter(const entry_ref *ref, 
                BNode *node, struct stat *st, const char *filetype)
		{ 
			return node->IsDirectory(); 
		}
private: 
}; 


// #pragma mark - FileFormatMenuItem


class FileFormatMenuItem : public BMenuItem {
	public:
				FileFormatMenuItem(media_file_format* format);
	virtual		~FileFormatMenuItem();
	
	media_file_format fFileFormat;
};


FileFormatMenuItem::FileFormatMenuItem(media_file_format *format)
	: BMenuItem(format->pretty_name, new BMessage(FORMAT_SELECT_MESSAGE))
{
	memcpy(&fFileFormat, format, sizeof(fFileFormat));
}


FileFormatMenuItem::~FileFormatMenuItem()
{
}


// #pragma mark - CodecMenuItem


class CodecMenuItem : public BMenuItem {
	public:
				CodecMenuItem(media_codec_info *ci, uint32 msg_type);
	virtual		~CodecMenuItem();
	
	media_codec_info fCodecInfo;
};


CodecMenuItem::CodecMenuItem(media_codec_info *ci, uint32 msg_type)
	: BMenuItem(ci->pretty_name, new BMessage(msg_type))
{
	memcpy(&fCodecInfo, ci, sizeof(fCodecInfo));
}


CodecMenuItem::~CodecMenuItem()
{
}


// #pragma mark - MediaConverterWindow


MediaConverterWindow::MediaConverterWindow(BRect frame)
	: BWindow(frame, "MediaConverter", B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)

	, fVideoQuality(75)
	, fAudioQuality(75)

	, fSaveFilePanel(NULL)
	, fOpenFilePanel(NULL)

	, fOutputDirSpecified(false)

	, fEnabled(true)
	, fConverting(false)
	, fCancelling(false)
{
	char defaultdirectory[] = "/boot/home";
	fOutputDir.SetTo(defaultdirectory);

#if B_BEOS_VERSION >= 0x530 // 0x520 RC2 0x530 RC3
	// load Locale files for ZETA
	entry_ref mypath;
	app_info info;

	be_app->GetAppInfo(&info);
	mypath = info.ref;

	BPath path(&mypath);
	path.GetParent(&path);
	path.Append("Language/Dictionaries"); 
    path.Append("MediaConverter");
	be_locale.LoadLanguageFile(path.Path()); 

#endif

    BRect dummyRect(0, 0, 10, 10);
    fMenuBar = new BMenuBar(dummyRect, "menubar");
	_CreateMenu();
    AddChild(fMenuBar);

	// background
	BRect r(frame);
	r.OffsetTo(0, 0);
	BView *background = new BView(r, NULL, B_FOLLOW_ALL_SIDES, 0);
	rgb_color c = ui_color(B_PANEL_BACKGROUND_COLOR);
	background->SetViewColor(c);
	background->SetLowColor(c);
	r.InsetBy(5, 25);

	// file list view box
	BRect r2(r);
	r2.bottom = r2.top + 420;
	r2.right = r2.left + 150;
	fBox1 = new BBox(r2, NULL, B_FOLLOW_ALL);

	BRect r3(r2);
	r3.OffsetTo(0, 0);
	r3.InsetBy(8, 8);
	r3.top += be_bold_font->Size() - 3;
	bool useHorizontalScrollBar = false;
	if (useHorizontalScrollBar)
		r3.bottom -= B_H_SCROLL_BAR_HEIGHT;
	r3.right -= B_V_SCROLL_BAR_WIDTH;
	fListView = new MediaFileListView(r3, B_FOLLOW_ALL);
	BScrollView *scroller = new BScrollView(NULL, fListView,
		B_FOLLOW_ALL, 0, useHorizontalScrollBar, true);
	fBox1->AddChild(scroller);
	background->AddChild(fBox1);

	// info box
	r2.left = r2.right + 5;
	r2.right = r.right - 5;
	r2.bottom = r2.top + 120;
	fBox2 = new BBox(r2, NULL, B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	
	r3 = r2;
	r3.OffsetTo(0, 0);
	r3.InsetBy(5, 5);
	r3.top += 12;
	fInfoView = new MediaFileInfoView(r3, B_FOLLOW_ALL);
	fBox2->AddChild(fInfoView);
	background->AddChild(fBox2);

	r2.top = r2.bottom + 5;
	r2.bottom = r2.top + 295;
	fBox3 = new BBox(r2, NULL, B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM);

	r3 = r2;
	r3.OffsetTo(0, 0);
	r3.InsetBy(8, 8);
	r3.top += be_bold_font->Size() - 3;
	
	BRect r4(r3);
	r4.bottom = r4.top + 20;
	BPopUpMenu* popmenu = new BPopUpMenu("");
	fFormatMenu = new BMenuField(r4, NULL, FORMAT_LABEL, popmenu,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fBox3->AddChild(fFormatMenu);
	
	r4.top = r4.bottom + 5;
	r4.bottom = r4.top + 20;
	popmenu = new BPopUpMenu("");
	fAudioMenu = new BMenuField(r4, NULL, AUDIO_LABEL, popmenu,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fBox3->AddChild(fAudioMenu);

	r4.top = r4.bottom + 5;
	r4.bottom = r4.top + 20;
	popmenu = new BPopUpMenu("");
	fVideoMenu = new BMenuField(r4, NULL, VIDEO_LABEL, popmenu,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fBox3->AddChild(fVideoMenu);

	// output folder
	r4.top = r4.bottom + 5;
	r4.bottom = r4.top + 20;
	r4.right = 80;
	fDestButton = new BButton(r4, NULL, OUTPUT_FOLDER_LABEL,
		new BMessage(OUTPUT_FOLDER_MESSAGE),
		B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fBox3->AddChild(fDestButton);
	fDestButton->ResizeToPreferred();
	BRect buttonFrame2(fDestButton->Frame());
	buttonFrame2.OffsetTo(r.right - buttonFrame2.Width(),
		r.bottom - buttonFrame2.Height());

	fOutputFolder = new BStringView(r4, NULL, defaultdirectory,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW);
	fBox3->AddChild(fOutputFolder);
	fOutputFolder->MoveBy(buttonFrame2.Width() + 10, 5);
	fOutputFolder->ResizeToPreferred();

	// start/end duration
	r4.top = r4.bottom + 10;
	r4.bottom = r4.top + 20;
	r4.right = r3.right;
	fStartDurationTC = new BTextControl(r4, NULL, "", "0", NULL,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fBox3->AddChild(fStartDurationTC);
	fStartDurationTC->SetText("0");
	
	r4.top = r4.bottom + 5;
	r4.bottom = r4.top + 20;
	fEndDurationTC = new BTextControl(r4, NULL, "", "0", NULL,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fBox3->AddChild(fEndDurationTC);
	fEndDurationTC->SetText("0");
 	
	r4.top = r4.bottom + 5;
	r4.bottom = r4.top + 50;

	// Video Quality
	fVideoQualitySlider = new BSlider(r4, "VSlider", "" ,
		new BMessage(VIDEO_QUALITY_CHANGED_MESSAGE), 1, 100, B_HORIZONTAL,
		B_BLOCK_THUMB, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fVideoQualitySlider->SetValue(fVideoQuality);
	fVideoQualitySlider->SetEnabled(false);
    fBox3->AddChild(fVideoQualitySlider);

	r4.top = r4.bottom + 5;
	r4.bottom = r4.top + 50;

	// Audio Quality
	fAudioQualitySlider = new BSlider(r4,"ASlider", "" ,
		new BMessage(AUDIO_QUALITY_CHANGED_MESSAGE), 1, 100, B_HORIZONTAL,
		B_BLOCK_THUMB, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fAudioQualitySlider->SetValue(fAudioQuality);
	fAudioQualitySlider->SetEnabled(false);
    fBox3->AddChild(fAudioQualitySlider);
	background->AddChild(fBox3);

	// buttons
	r2.top = r2.bottom + 15;
	r2.bottom = r2.top + 20;
	r2.left = r.left - 120;
	r2.right = r.right;

	fPreviewButton = new BButton(r2, NULL, PREVIEW_BUTTON_LABEL,
		new BMessage(PREVIEW_MESSAGE), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	background->AddChild(fPreviewButton);
	fPreviewButton->SetEnabled(false);

	fConvertButton = new BButton(r2, NULL, CONVERT_LABEL,
		new BMessage(CONVERT_BUTTON_MESSAGE),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	background->AddChild(fConvertButton);

	// Status view
	r2.bottom = r2.top + 20;
	r2.left = r.left;
	r2.right = r.right;

	fStatusView2 = new StatusView(r2, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	background->AddChild(fStatusView2);

	r2.top += 15;
	r2.bottom += 20;

	fStatusView = new StatusView(r2, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	background->AddChild(fStatusView);
	AddChild(background);

	SetStatusMessage("");
	_UpdateLabels();

	SetSizeLimits(frame.Width(), 32000, frame.Height(), 32000);
}


MediaConverterWindow::~MediaConverterWindow()
{
}


// #pragma mark -


/*
void 
MediaConverterWindow::DispatchMessage(BMessage *msg, BHandler *handler)
{
	if (msg->WasDropped() && msg->what == B_SIMPLE_DATA) {
	
		printf("Dispatch 1\n");
		DetachCurrentMessage();
		msg->what = B_REFS_RECEIVED;
		BMessenger(be_app).SendMessage(msg);
		delete msg;
	} else {
		BWindow::DispatchMessage(msg, handler);
	}
}
*/


void 
MediaConverterWindow::MessageReceived(BMessage *msg)
{
	status_t status;
	entry_ref ref;
	entry_ref inRef;

	BString string, string2;

	BEntry entry("/boot/beos/apps/MediaPlayer", true);
	char buffer[40];
	char buffer2[B_PATH_NAME_LENGTH];
	char *argv[3];
	argv[0] = "-pos";
	BMediaFile *inFile(NULL);
	int32 srcIndex = 0;
	BPath name;
	BEntry inentry;
	int32 value;
   	BRect ButtonRect;

	switch (msg->what) {
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
			if (msg->WasDropped())
			{
				DetachCurrentMessage();
				msg->what = B_REFS_RECEIVED;
				BMessenger(be_app).SendMessage(msg);
				delete msg;
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
				fConvertButton->SetLabel(CANCEL_LABEL);
				fConverting = true;
				SetStatusMessage(CONVERT_LABEL);
				SetEnabled(false, true);
				BMessenger(be_app).SendMessage(START_CONVERSION_MESSAGE);
			} else if (!fCancelling) {
				fCancelling = true;
				SetStatusMessage(CANCELLING_LABEL B_UTF8_ELLIPSIS);
				BMessenger(be_app).SendMessage(CANCEL_CONVERSION_MESSAGE);			
			}
			break;
	
		case CONVERSION_DONE_MESSAGE:
			SetStatusMessage(fCancelling ? CONV_CANCEL_LABEL : CONV_COMPLETE_LABEL);
			fConverting = false;
			fCancelling = false;
			{
				bool enable = CountSourceFiles() > 0;
				SetEnabled(enable, enable);
			}
			fConvertButton->SetLabel(CONVERT_LABEL);
	
	
			break;
	
		case OUTPUT_FOLDER_MESSAGE:
		//	 Execute Save Panel
			if (!fSaveFilePanel) {
			    BButton *SelectThisDir;
	
				fSaveFilePanel = new BFilePanel(B_OPEN_PANEL, NULL, NULL, B_DIRECTORY_NODE, true, new BMessage(FOLDER_SELECT_MESSAGE), NULL, false, true);
				fSaveFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, SELECT_LABEL);
				fSaveFilePanel->Window()->SetTitle(SAVE_DIR_LABEL);
				fSaveFilePanel->SetTarget(this);
	
				fSaveFilePanel->Window()->Lock();
				ButtonRect = fSaveFilePanel->Window()->ChildAt(0)->FindView("cancel button")->Frame();
				ButtonRect.right  = ButtonRect.left - 20;
				ButtonRect.left = ButtonRect.right - 130;
				SelectThisDir = new BButton(ButtonRect, NULL, SELECT_DIR_LABEL,
									 new BMessage(SELECT_THIS_DIR_MESSAGE), B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
				SelectThisDir->SetTarget(this);
				fSaveFilePanel->Window()->ChildAt(0)->AddChild(SelectThisDir);
				fSaveFilePanel->Window()->Unlock();
	
				BRefFilter *filter;
				filter = new DirectoryFilter;
				fSaveFilePanel->SetRefFilter(filter);
			}
			fSaveFilePanel->Show();
			break;
	
		case FOLDER_SELECT_MESSAGE:
		//	 "SELECT" Button at Save Panel Pushed
			fSaveFilePanel->GetNextSelectedRef(&inRef);
			inentry.SetTo(&inRef, true);
			_SetOutputFolder(inentry);
			fOutputDirSpecified = true;
			break;
	
		case SELECT_THIS_DIR_MESSAGE:
		//	 "THIS DIR" Button at Save Panel Pushed
			fSaveFilePanel->GetPanelDirectory(&inRef);
			fSaveFilePanel->Hide();
			inentry.SetTo(&inRef, true);
			_SetOutputFolder(inentry);
			fOutputDirSpecified = true;
			break;
	
		case OPEN_FILE_MESSAGE:
		//	 Execute Open Panel
			if (!fOpenFilePanel) {
				fOpenFilePanel = new BFilePanel(B_OPEN_PANEL, NULL, NULL, B_FILE_NODE, true, NULL, NULL, false, true);
				fOpenFilePanel->SetTarget(this);
			}
			fOpenFilePanel->Show();
			break;
	
		case B_REFS_RECEIVED:
		//	Media Files Seleced by Open Panel
			DetachCurrentMessage();
			msg->what = B_REFS_RECEIVED;
			BMessenger(be_app).SendMessage(msg);
			// fall through
	
		case B_CANCEL:
			break;
	
		case DISP_ABOUT_MESSAGE: {
			(new BAlert(ABOUT_TITLE_LABEL B_UTF8_ELLIPSIS,
					"MediaConverter\n"
					VERSION"\n"
					B_UTF8_COPYRIGHT" 1999, Be Incorporated.\n"
					B_UTF8_COPYRIGHT" 2000-2004 Jun Suzuki\n"
					B_UTF8_COPYRIGHT" 2007 Stephan Aßmus",
					OK_LABEL))->Go();
			break;
		}

		case QUIT_MESSAGE:
			MediaConverterWindow::QuitRequested();
			break;

		case PREVIEW_MESSAGE:
			entry.GetRef(&ref);
			string = "";
			string << fStartDurationTC->Text();
			string << "000";
			
			strcpy(buffer,string.String());
			argv[1] = buffer;
			srcIndex = fListView->CurrentSelection();
			status = GetSourceFileAt(srcIndex, &inFile, &inRef);
			if (status == B_OK) {
				inentry.SetTo(&inRef);
				inentry.GetPath(&name);
	
				strcpy(buffer, string.String());
				
				strcpy(buffer2, name.Path());
				argv[2]= buffer2;
			}
	
			status = be_roster->Launch(&ref, 3, argv);
	
			if (status != B_OK) {
				string2 << LAUNCH_ERROR << strerror(status);
				(new BAlert("", string2.String(), OK_LABEL))->Go();
			}
			break;

		case VIDEO_QUALITY_CHANGED_MESSAGE:
			msg->FindInt32("be:value",&value); 
			sprintf(buffer, VIDEO_QUALITY_LABEL, (int8)value);
			fVideoQualitySlider->SetLabel(buffer);
			fVideoQuality = value;
			break;

		case AUDIO_QUALITY_CHANGED_MESSAGE:
			msg->FindInt32("be:value",&value); 
			sprintf(buffer, AUDIO_QUALITY_LABEL, (int8)value);
			fAudioQualitySlider->SetLabel(buffer);
			fAudioQuality = value;
			break;

		default:
			BWindow::MessageReceived(msg);
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
		SetStatusMessage(CANCELLING_LABEL);
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
	BMenu *menu = fAudioMenu->Menu();
	BMenuItem *item;
	// clear out old audio codec menu items
	while ((item = menu->RemoveItem((int32)0)) != NULL) {
		delete item;
	}

	bool separator = true;

	// get selected file format
	FileFormatMenuItem *ffmi = (FileFormatMenuItem*)fFormatMenu->Menu()->FindMarked();
	media_file_format *mf_format = &(ffmi->fFileFormat);

	media_format format, outfmt;
	memset(&format, 0, sizeof(format));
	media_codec_info codec_info;
	int32 cookie = 0;
	CodecMenuItem* cmi;

	// add available audio encoders to menu
	format.type = B_MEDIA_RAW_AUDIO;
	format.u.raw_audio = media_raw_audio_format::wildcard;	
	while (get_next_encoder(&cookie, mf_format, &format, &outfmt, &codec_info) == B_OK) {
		if (separator) {
			menu->AddItem(new BMenuItem("No Audio",
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
		((BInvoker *)item)->Invoke();
	} else {
		item = new BMenuItem(NONE_LABEL, NULL);
		menu->AddItem(item);
		item->SetMarked(true);
		fAudioMenu->SetEnabled(false);
		fAudioQualitySlider->SetEnabled(false);
	}

	// clear out old video codec menu items
	menu = fVideoMenu->Menu();
	while ((item = menu->RemoveItem((int32)0)) != NULL) {
		delete item;
	}

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
	while (get_next_encoder(&cookie, mf_format, &format, &outfmt, &codec_info) == B_OK) {
		if (separator) {
			menu->AddItem(new BMenuItem("No Video",
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
		((BInvoker *)item)->Invoke();
	} else {
		item = new BMenuItem(NONE_LABEL, NULL);
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

	FileFormatMenuItem *formatItem =
		dynamic_cast<FileFormatMenuItem *>(fFormatMenu->Menu()->FindMarked());
	if (formatItem != NULL) {
		*format = &(formatItem->fFileFormat);
	}

	*audio = *video = NULL;
	CodecMenuItem *codecItem =
		dynamic_cast<CodecMenuItem *>(fAudioMenu->Menu()->FindMarked());
	if (codecItem != NULL) {
		*audio =  &(codecItem->fCodecInfo);
	}

	codecItem = dynamic_cast<CodecMenuItem *>(fVideoMenu->Menu()->FindMarked());
	if (codecItem != NULL) {
		*video =  &(codecItem->fCodecInfo);
	}
}


void 
MediaConverterWindow::BuildFormatMenu()
{
	BMenu *menu = fFormatMenu->Menu();
	BMenuItem *item;
	// clear out old format menu items
	while ((item = menu->RemoveItem((int32)0)) != NULL) {
		delete item;
	}

	// add menu items for each file format
	media_file_format mfi;
	int32 cookie = 0;
	FileFormatMenuItem *ff_item;
	while (get_next_file_format(&cookie, &mfi) == B_OK) {
		ff_item = new FileFormatMenuItem(&mfi);
		menu->AddItem(ff_item);
	}
	
	// mark first item
	item = menu->ItemAt(0);
	if (item != NULL) {
		item->SetMarked(true);
		((BInvoker *)item)->Invoke();
	}
}

void 
MediaConverterWindow::SetFileMessage(const char *message)
{
	fStatusView->SetStatus(message);
}

void 
MediaConverterWindow::SetStatusMessage(const char *message)
{
	fStatusView2->SetStatus(message);
}


// #pragma mark -


void
MediaConverterWindow::AddSourceFile(BMediaFile* file, const entry_ref& ref)
{
	fListView->AddItem(file, ref);

	if (!fOutputDirSpecified) {
		BEntry entry(&ref);
		entry.GetParent(&entry);
		_SetOutputFolder(entry);
	}
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
	} else {
		return B_ERROR;
	}
}


void 
MediaConverterWindow::SourceFileSelectionChanged()
{
	int32 selected = fListView->CurrentSelection();
	BMediaFile* file = NULL;
	entry_ref* _ref = NULL;
	entry_ref ref;
	bool enabled = false;
	if (GetSourceFileAt(selected, &file, &ref) == B_OK) {
		_ref = &ref;
		enabled = true;
	}

	fPreviewButton->SetEnabled(enabled);
	fVideoQualitySlider->SetEnabled(enabled);
	fAudioQualitySlider->SetEnabled(enabled);
	fStartDurationTC->SetEnabled(enabled);
	fEndDurationTC->SetEnabled(enabled);

	fInfoView->Update(file, _ref);

	// HACK: get the fInfoView to update the duration "synchronously"
	UpdateIfNeeded();

	// update duration text controls
	fStartDurationTC->SetText("0");
	BString duration;
	duration << fInfoView->Duration() / 1000;
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


// #pragma mark -


void
MediaConverterWindow::_UpdateLabels()
{
	char buffer[255];

	if (fBox1 != NULL)
		fBox1->SetLabel(SOURCE_BOX_LABEL);
	if (fBox2 != NULL)
		fBox2->SetLabel(INFO_BOX_LABEL);
	if (fBox3 != NULL)
		fBox3->SetLabel(OUTPUT_BOX_LABEL);

	BRect r(Bounds());
	float w = 0;

	if (fConvertButton != NULL) {
		fConvertButton->SetLabel(CONVERT_LABEL);
		fConvertButton->ResizeToPreferred();
		BRect buttonFrame(fConvertButton->Frame());
		w = buttonFrame.Width();
		buttonFrame.OffsetTo(r.right - w - 10, r.bottom - buttonFrame.Height() - 25);
		fConvertButton->MoveTo(buttonFrame.LeftTop());
	}

	if (fPreviewButton != NULL) {
		fPreviewButton->SetLabel(PREVIEW_BUTTON_LABEL);
		fPreviewButton->ResizeToPreferred();
		BRect buttonFrame(fPreviewButton->Frame());
		w = buttonFrame.Width();
		buttonFrame.OffsetTo(r.right - w - buttonFrame.Width() - 40,
						 r.bottom - buttonFrame.Height() - 25);
		fPreviewButton->MoveTo(buttonFrame.LeftTop());
	}
		
	if (fDestButton != NULL) {
		fDestButton->SetLabel(OUTPUT_FOLDER_LABEL);
		fDestButton->ResizeToPreferred();
	}

	if (fOutputFolder != NULL) {
		BRect destrect;
		destrect = fDestButton->Frame();
		fOutputFolder->MoveTo(destrect.right + 10, destrect.top + 5);
		fOutputFolder->ResizeToPreferred();
	}

	sprintf(buffer, VIDEO_QUALITY_LABEL, (int8)fVideoQuality);
	if (fVideoQualitySlider != NULL) {
		fVideoQualitySlider->SetLabel(buffer);
	    fVideoQualitySlider->SetLimitLabels(SLIDER_LOW_LABEL,
	    	SLIDER_HIGH_LABEL);
	}

	sprintf(buffer, AUDIO_QUALITY_LABEL, (int8)fAudioQuality);
	
	if (fAudioQualitySlider != NULL) {
		fAudioQualitySlider->SetLabel(buffer);
	    fAudioQualitySlider->SetLimitLabels(SLIDER_LOW_LABEL,
	    	SLIDER_HIGH_LABEL);
	}

	float maxLabelLen = 0;
	if (fStartDurationTC != NULL) {
		fStartDurationTC->SetLabel(START_LABEL);
		maxLabelLen = fStartDurationTC->StringWidth(START_LABEL);
	}
	if (fEndDurationTC != NULL) {
		fEndDurationTC->SetLabel(END_LABEL);
		maxLabelLen = MAX(maxLabelLen, fEndDurationTC->StringWidth(END_LABEL));
	}
	if (fStartDurationTC != NULL)
		fStartDurationTC->SetDivider(maxLabelLen + 5);
	if (fEndDurationTC != NULL)
		fEndDurationTC->SetDivider(maxLabelLen + 5);
	
	if (fFormatMenu != NULL) {
		fFormatMenu->SetLabel(FORMAT_LABEL);
		maxLabelLen = MAX(maxLabelLen, fFormatMenu->StringWidth(
			fFormatMenu->Label()));
	}
	if (fAudioMenu != NULL) {
		fAudioMenu->SetLabel(AUDIO_LABEL);
		maxLabelLen = MAX(maxLabelLen, fAudioMenu->StringWidth(
			fAudioMenu->Label()));
	}
	if (fVideoMenu != NULL) {
		fVideoMenu->SetLabel(VIDEO_LABEL);
		maxLabelLen = MAX(maxLabelLen, fVideoMenu->StringWidth(
			fVideoMenu->Label()));
	}
	maxLabelLen += 10;

	if (fStartDurationTC != NULL)
		fStartDurationTC->SetDivider(maxLabelLen);
	if (fEndDurationTC != NULL)
		fEndDurationTC->SetDivider(maxLabelLen);
	if (fFormatMenu != NULL)
		fFormatMenu->SetDivider(maxLabelLen + 3);
	if (fAudioMenu != NULL)
		fAudioMenu->SetDivider(maxLabelLen + 3);
	if (fVideoMenu != NULL)
		fVideoMenu->SetDivider(maxLabelLen + 3);

	SetFileMessage(DROP_MEDIA_FILE_LABEL);
}


void
MediaConverterWindow::_DestroyMenu()
{
	BMenu* Menu;

	while ((Menu = fMenuBar->SubmenuAt(0)) != NULL) {
		fMenuBar->RemoveItem(Menu);
		delete Menu;
	}
}


void
MediaConverterWindow::_CreateMenu()
{
    BMenuItem* item;
    BMenu* menu;

    menu = new BMenu(FILE_MENU_LABEL);
	item = new BMenuItem(OPEN_MENU_LABEL B_UTF8_ELLIPSIS,
		new BMessage(OPEN_FILE_MESSAGE));
	menu->AddItem(item);
	menu->AddSeparatorItem();
	item = new BMenuItem(ABOUT_MENU_LABEL B_UTF8_ELLIPSIS,
		new BMessage(DISP_ABOUT_MESSAGE));
	menu->AddItem(item);
	menu->AddSeparatorItem();
	item = new BMenuItem(QUIT_MENU_LABEL, new BMessage(QUIT_MESSAGE));
	menu->AddItem(item);

    fMenuBar->AddItem(menu);
}


void
MediaConverterWindow::_SetOutputFolder(BEntry entry)
{
	BPath path;
	entry.GetPath(&path);
	fOutputFolder->SetText(path.Path());
	fOutputFolder->ResizeToPreferred();
	fOutputDir.SetTo(path.Path());
}


