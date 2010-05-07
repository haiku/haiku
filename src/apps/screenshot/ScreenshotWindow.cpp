/*
 * Copyright Karsten Heimrich, host.haiku@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Karsten Heimrich
 *		Fredrik Modéen
 *		Christophe Huriaux
 *		Wim van der Meer, WPJvanderMeer@gmail.com
 */


#include "ScreenshotWindow.h"


#include <stdio.h>
#include <stdlib.h>


#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <BitmapStream.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <FilePanel.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <LayoutItem.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <NodeInfo.h>
#include <Path.h>
#include <RadioButton.h>
#include <Region.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <StringView.h>
#include <SpaceLayoutItem.h>
#include <TextControl.h>
#include <TranslatorFormats.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <View.h>
#include <WindowInfo.h>


#include "PreviewView.h"


enum {
	kScreenshotType,
	kIncludeBorder,
	kShowMouse,
	kBackToSave,
	kTakeScreenshot,
	kImageOutputFormat,
	kLocationChanged,
	kChooseLocation,
	kFinishScreenshot,
	kShowOptions
};


// #pragma mark - DirectoryRefFilter


class DirectoryRefFilter : public BRefFilter {
public:
	virtual ~DirectoryRefFilter()
	{
	}

	virtual bool Filter(const entry_ref* ref, BNode* node,
		struct stat_beos* stat, const char* filetype)
	{
		return node->IsDirectory();
	}
};


// #pragma mark - ScreenshotWindow


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ScreenshotWindow"


ScreenshotWindow::ScreenshotWindow()
	:
	BWindow(BRect(0, 0, 200.0, 100.0), B_TRANSLATE("Retake screenshot"),
		B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE
		| B_QUIT_ON_WINDOW_CLOSE | B_AVOID_FRONT
		| B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fDelayControl(NULL),
	fScreenshot(NULL),
	fOutputPathPanel(NULL),
	fLastSelectedPath(NULL),
	fDelay(0),
	fTabHeight(0),
	fIncludeBorder(false),
	fIncludeMouse(false),
	fGrabActiveWindow(false),
	fShowConfigWindow(false),
	fSaveScreenshotSilent(false),
	fOutputFilename(NULL),
	fExtension(""),
	fImageFileType(B_PNG_FORMAT)
{
	BMessage settings = _ReadSettings();

	if (settings.FindInt32("type", &fImageFileType) != B_OK)
		fImageFileType = B_PNG_FORMAT;
	settings.FindBool("includeBorder", &fIncludeBorder);
	settings.FindBool("includeMouse", &fIncludeMouse);
	settings.FindBool("grabActiveWindow", &fGrabActiveWindow);
	settings.FindInt64("delay", &fDelay);
	settings.FindString("outputFilename", &fOutputFilename);

	_InitWindow(settings);
	CenterOnScreen();
	Show();
}


ScreenshotWindow::ScreenshotWindow(bigtime_t delay, bool includeBorder,
	bool includeMouse, bool grabActiveWindow, bool showConfigWindow,
	bool saveScreenshotSilent, int32 imageFileType, const char* outputFilename)
	:
	BWindow(BRect(0, 0, 200.0, 100.0), B_TRANSLATE("Retake screenshot"),
		B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE
		| B_QUIT_ON_WINDOW_CLOSE | B_AVOID_FRONT
		| B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fDelayControl(NULL),
	fScreenshot(NULL),
	fOutputPathPanel(NULL),
	fLastSelectedPath(NULL),
	fDelay(delay),
	fTabHeight(0),
	fIncludeBorder(includeBorder),
	fIncludeMouse(includeMouse),
	fGrabActiveWindow(grabActiveWindow),
	fShowConfigWindow(showConfigWindow),
	fSaveScreenshotSilent(saveScreenshotSilent),
	fOutputFilename(outputFilename),
	fExtension(""),
	fImageFileType(imageFileType)
{
	if (fSaveScreenshotSilent) {
		_TakeScreenshot(fDelay);
		_SaveScreenshot();
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	} else {
		BMessage settings = _ReadSettings();
		_InitWindow(settings);
		CenterOnScreen();
		Show();
	}
}


ScreenshotWindow::~ScreenshotWindow()
{
	if (fOutputPathPanel)
		delete fOutputPathPanel->RefFilter();

	delete fScreenshot;
	delete fOutputPathPanel;
}


void
ScreenshotWindow::MessageReceived(BMessage* message)
{
//	message->PrintToStream();
	switch (message->what) {
		case kScreenshotType:
			fGrabActiveWindow = false;
			if (fActiveWindow->Value() == B_CONTROL_ON)
				fGrabActiveWindow = true;
			fWindowBorder->SetEnabled(fGrabActiveWindow);
			break;

		case kIncludeBorder:
			fIncludeBorder = (fWindowBorder->Value() == B_CONTROL_ON);
			break;

		case kShowMouse:
			fIncludeMouse = (fShowMouse->Value() == B_CONTROL_ON);
			break;

		case kBackToSave:
		{
			BCardLayout* layout = dynamic_cast<BCardLayout*> (GetLayout());
			if (layout)
				layout->SetVisibleItem(1L);

			fSaveScreenshot->MakeDefault(true);

			SetTitle(B_TRANSLATE("Save screenshot"));
			break;
		}

		case kTakeScreenshot:
			fDelay = (atoi(fDelayControl->Text()) * 1000000) + 50000;
			Hide();
			_TakeScreenshot(fDelay);
			_UpdatePreviewPanel();
			Show();
			_UpdateFilenameSelection();
			break;

		case kImageOutputFormat:
			message->FindInt32("be:type", &fImageFileType);
			fNameControl->SetText(_FindValidFileName(
				fNameControl->Text()).String());
			_UpdateFilenameSelection();
			break;

		case kLocationChanged:
		{
			void* source = NULL;
			if (message->FindPointer("source", &source) == B_OK)
				fLastSelectedPath = static_cast<BMenuItem*> (source);

			fNameControl->SetText(_FindValidFileName(
				fNameControl->Text()).String());
			_UpdateFilenameSelection();
			break;
		}

		case kChooseLocation:
		{
			if (!fOutputPathPanel) {
				BMessenger target(this);
				fOutputPathPanel = new BFilePanel(B_OPEN_PANEL, &target, NULL,
					B_DIRECTORY_NODE, false, NULL, new DirectoryRefFilter());
				fOutputPathPanel->Window()->SetTitle(
					B_TRANSLATE("Choose folder"));
				fOutputPathPanel->SetButtonLabel(B_DEFAULT_BUTTON,
					B_TRANSLATE("Select"));
				fOutputPathPanel->SetButtonLabel(B_CANCEL_BUTTON,
					B_TRANSLATE("Cancel"));
			}
			fOutputPathPanel->Show();
			break;
		}

		case B_CANCEL:
			fLastSelectedPath->SetMarked(true);
			break;

		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BString path(BPath(&ref).Path());
				int32 index = _PathIndexInMenu(path);
				if (index < 0)
					_AddItemToPathMenu(path.String(), path,
						fOutputPathMenu->CountItems() - 2, true);
				else
					fOutputPathMenu->ItemAt(index)->SetMarked(true);
			}
			break;
		}

		case kFinishScreenshot:
			_WriteSettings();
			if (_SaveScreenshot() != B_OK)
				break;

		// fall through
		case B_QUIT_REQUESTED:
			_WriteSettings();
			be_app_messenger.SendMessage(B_QUIT_REQUESTED);
			break;

		case kShowOptions:
		{
			BCardLayout* layout = dynamic_cast<BCardLayout*> (GetLayout());

			if (layout)
				layout->SetVisibleItem(0L);
			SetDefaultButton(NULL);

			SetTitle(B_TRANSLATE("Take Screenshot"));
			fBackToSave->SetEnabled(true);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


BPath
ScreenshotWindow::_GetDirectory()
{
	BPath path;
	if (!fSaveScreenshotSilent) {
		BMessage* message = fLastSelectedPath->Message();
		const char* stringPath;
		if (!message || message->FindString("path", &stringPath) != B_OK) {
			fprintf(stderr, "failed to find path in message\n");
		} else
			path.SetTo(stringPath);
	} else {
		if (find_directory(B_USER_DIRECTORY, &path) != B_OK)
			fprintf(stderr, "failed to find user home folder\n");
	}
	return path;
}


void
ScreenshotWindow::_InitWindow(const BMessage& settings)
{
	BCardLayout* layout = new BCardLayout();
	SetLayout(layout);

	_SetupFirstLayoutItem(layout);
	_SetupSecondLayoutItem(layout, settings);

	if (!fShowConfigWindow) {
		_TakeScreenshot(0);
		_UpdatePreviewPanel();
		layout->SetVisibleItem(1L);
		fSaveScreenshot->MakeDefault(true);
	} else {
		layout->SetVisibleItem(0L);
		SetDefaultButton(NULL);
	}
}


void
ScreenshotWindow::_SetupFirstLayoutItem(BCardLayout* layout)
{
	BStringView* stringView = new BStringView("", B_TRANSLATE("Options"));
	stringView->SetFont(be_bold_font);
	stringView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fActiveWindow = new BRadioButton(B_TRANSLATE("Capture active window"),
		new BMessage(kScreenshotType));
	fWholeDesktop = new BRadioButton(B_TRANSLATE("Capture entire screen"),
		new BMessage(kScreenshotType));
	fWholeDesktop->SetValue(B_CONTROL_ON);

	BString delay;
	delay << fDelay / 1000000;
	fDelayControl = new BTextControl("",
		B_TRANSLATE("Take screenshot after a delay of"), delay.String(), NULL);
	_DisallowChar(fDelayControl->TextView());
	fDelayControl->TextView()->SetAlignment(B_ALIGN_RIGHT);

	BStringView* stringView2 = new BStringView("", B_TRANSLATE("seconds"));
	stringView2->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fWindowBorder = new BCheckBox(B_TRANSLATE("Include window border"),
		new BMessage(kIncludeBorder));
	fWindowBorder->SetEnabled(false);

	fShowMouse = new BCheckBox(B_TRANSLATE("Include mouse pointer"),
		new BMessage(kShowMouse));
	fShowMouse->SetValue(fIncludeMouse);

	BBox* divider = new BBox(B_FANCY_BORDER, NULL);
	divider->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));

	fBackToSave = new BButton("", B_TRANSLATE("Back to saving"),
		new BMessage(kBackToSave));
	fBackToSave->SetEnabled(false);

	fTakeScreenshot = new BButton("", B_TRANSLATE("Take screenshot"),
		new BMessage(kTakeScreenshot));

	layout->AddView(0, BGroupLayoutBuilder(B_VERTICAL)
		.Add(stringView)
		.Add(BGridLayoutBuilder()
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(15.0), 0, 0)
			.Add(fWholeDesktop, 1, 0)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(15.0), 0, 1)
			.Add(fActiveWindow, 1, 1)
			.SetInsets(0.0, 5.0, 0.0, 0.0))
		.AddGroup(B_HORIZONTAL)
			.AddStrut(30.0)
			.Add(fWindowBorder)
			.End()
		.AddStrut(10.0)
		.AddGroup(B_HORIZONTAL)
			.AddStrut(15.0)
			.Add(fShowMouse)
			.End()
		.AddStrut(5.0)
		.AddGroup(B_HORIZONTAL, 5.0)
			.AddStrut(10.0)
			.Add(fDelayControl->CreateLabelLayoutItem())
			.Add(fDelayControl->CreateTextViewLayoutItem())
			.Add(stringView2)
			.End()
		.AddStrut(10.0)
		.AddGlue()
		.Add(divider)
		.AddStrut(10)
		.AddGroup(B_HORIZONTAL, 10.0)
			.Add(fBackToSave)
			.AddGlue()
			.Add(new BButton("", B_TRANSLATE("Cancel"),
				new BMessage(B_QUIT_REQUESTED)))
			.Add(fTakeScreenshot)
			.End()
		.SetInsets(10.0, 10.0, 10.0, 10.0)
	);

	if (fGrabActiveWindow) {
		fWindowBorder->SetEnabled(true);
		fActiveWindow->SetValue(B_CONTROL_ON);
		fWindowBorder->SetValue(fIncludeBorder);
	}
}


void
ScreenshotWindow::_SetupSecondLayoutItem(BCardLayout* layout,
	const BMessage& settings)
{
	fPreview = new PreviewView();

	fNameControl = new BTextControl("", B_TRANSLATE("Name:"),
		B_TRANSLATE_COMMENT("screenshot1", "!! Filename of first "
			"screenshot !!"), NULL);

	_SetupOutputPathMenu(new BMenu(B_TRANSLATE("Please select")), settings);

	BMenuField* menuField2 = new BMenuField(B_TRANSLATE("Save in:"),
		fOutputPathMenu);

	fNameControl->SetText(_FindValidFileName(
		B_TRANSLATE_COMMENT("screenshot1", "!! Filename of first "
			"screenshot !!")).String());

	_SetupTranslatorMenu(new BMenu(B_TRANSLATE("Please select")));
	BMenuField* menuField = new BMenuField(B_TRANSLATE("Save as:"),
		fTranslatorMenu);

	BBox* divider = new BBox(B_FANCY_BORDER, NULL);
	divider->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));

	fSaveScreenshot  = new BButton("",
		B_TRANSLATE("Save"), new BMessage(kFinishScreenshot));

	BGridLayout* gridLayout = BGridLayoutBuilder(0.0, 5.0)
		.Add(fNameControl->CreateLabelLayoutItem(), 0, 0)
		.Add(fNameControl->CreateTextViewLayoutItem(), 1, 0)
		.Add(menuField->CreateLabelLayoutItem(), 0, 1)
		.Add(menuField->CreateMenuBarLayoutItem(), 1, 1)
		.Add(menuField2->CreateLabelLayoutItem(), 0, 2)
		.Add(menuField2->CreateMenuBarLayoutItem(), 1, 2);
	gridLayout->SetMinColumnWidth(1,
		menuField->StringWidth("SomethingLongHere"));

	layout->AddView(1, BGroupLayoutBuilder(B_VERTICAL)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10.0)
			.Add(fPreview)
			.AddGroup(B_VERTICAL)
				.Add(gridLayout->View())
				.AddGlue()
				.End())
		.AddStrut(10)
		.Add(divider)
		.AddStrut(10)
		.AddGroup(B_HORIZONTAL, 10.0)
			.Add(new BButton("", B_TRANSLATE("Options"),
				new BMessage(kShowOptions)))
			.AddGlue()
			.Add(new BButton("", B_TRANSLATE("Cancel"),
				new BMessage(B_QUIT_REQUESTED)))
			.Add(fSaveScreenshot)
			.End()
		.SetInsets(10.0, 10.0, 10.0, 10.0)
	);
}


void
ScreenshotWindow::_DisallowChar(BTextView* textView)
{
	for (uint32 i = 0; i < '0'; ++i)
		textView->DisallowChar(i);

	for (uint32 i = '9' + 1; i < 255; ++i)
		textView->DisallowChar(i);
}


void
ScreenshotWindow::_SetupTranslatorMenu(BMenu* translatorMenu)
{
	fTranslatorMenu = translatorMenu;

	BMessage message(kImageOutputFormat);
	fTranslatorMenu = new BMenu("Please select");
	BTranslationUtils::AddTranslationItems(fTranslatorMenu, B_TRANSLATOR_BITMAP,
		&message, NULL, NULL, NULL);

	fTranslatorMenu->SetLabelFromMarked(true);

	if (fTranslatorMenu->ItemAt(0))
		fTranslatorMenu->ItemAt(0)->SetMarked(true);

	int32 imageFileType;
	for (int32 i = 0; i < fTranslatorMenu->CountItems(); ++i) {
		BMenuItem* item = fTranslatorMenu->ItemAt(i);
		if (item && item->Message()) {
			item->Message()->FindInt32("be:type", &imageFileType);
			if (fImageFileType == imageFileType) {
				item->SetMarked(true);
				MessageReceived(item->Message());
				break;
			}
		}
	}
}


void
ScreenshotWindow::_SetupOutputPathMenu(BMenu* outputPathMenu,
	const BMessage& settings)
{
	fOutputPathMenu = outputPathMenu;
	fOutputPathMenu->SetLabelFromMarked(true);

	BString lastSelectedPath;
	settings.FindString("lastSelectedPath", &lastSelectedPath);

	BPath path;
	find_directory(B_USER_DIRECTORY, &path);

	BString label(B_TRANSLATE("Home folder"));
	_AddItemToPathMenu(path.Path(), label, 0,
		(path.Path() == lastSelectedPath));

	path.Append("Desktop");
	label.SetTo(B_TRANSLATE("Desktop"));
	_AddItemToPathMenu(path.Path(), label, 0, (
		path.Path() == lastSelectedPath));

	find_directory(B_BEOS_ETC_DIRECTORY, &path);
	path.Append("artwork");

	label.SetTo(B_TRANSLATE("Artwork folder"));
	_AddItemToPathMenu(path.Path(), label, 2,
		(path.Path() == lastSelectedPath));

	int32 i = 0;
	BString userPath;
	while (settings.FindString("path", i++, &userPath) == B_OK) {
		_AddItemToPathMenu(userPath.String(), userPath, 3,
			(userPath == lastSelectedPath));
	}

	if (!fLastSelectedPath) {
		if (settings.IsEmpty() || lastSelectedPath.Length() == 0) {
			fOutputPathMenu->ItemAt(1)->SetMarked(true);
			fLastSelectedPath = fOutputPathMenu->ItemAt(1);
		} else
			_AddItemToPathMenu(lastSelectedPath.String(), lastSelectedPath, 3,
				true);
	}

	fOutputPathMenu->AddItem(new BSeparatorItem());
	fOutputPathMenu->AddItem(new BMenuItem(B_TRANSLATE("Choose folder..."),
		new BMessage(kChooseLocation)));
}


void
ScreenshotWindow::_AddItemToPathMenu(const char* path, BString& label,
	int32 index, bool markItem)
{
	BMessage* message = new BMessage(kLocationChanged);
	message->AddString("path", path);

	fOutputPathMenu->TruncateString(&label, B_TRUNCATE_MIDDLE,
		fOutputPathMenu->StringWidth("SomethingLongHere"));

	fOutputPathMenu->AddItem(new BMenuItem(label.String(), message), index);

	if (markItem) {
		fOutputPathMenu->ItemAt(index)->SetMarked(true);
		fLastSelectedPath = fOutputPathMenu->ItemAt(index);
	}
}


void
ScreenshotWindow::_UpdatePreviewPanel()
{
	float height = 150.0f;

	float width = (fScreenshot->Bounds().Width()
		/ fScreenshot->Bounds().Height()) * height;

	// to prevent a preview way too wide
	if (width > 400.0f) {
		width = 400.0f;
		height = (fScreenshot->Bounds().Height()
			/ fScreenshot->Bounds().Width()) * width;
	}

	fPreview->SetExplicitMinSize(BSize(width, height));
	fPreview->SetExplicitMaxSize(BSize(width, height));

	fPreview->ClearViewBitmap();
	fPreview->SetViewBitmap(fScreenshot, fScreenshot->Bounds(),
		fPreview->Bounds(), B_FOLLOW_ALL, 0);

	BCardLayout* layout = dynamic_cast<BCardLayout*> (GetLayout());
	if (layout)
		layout->SetVisibleItem(1L);

	_UpdateFilenameSelection();

	SetTitle(B_TRANSLATE("Save screenshot"));
}


void
ScreenshotWindow::_UpdateFilenameSelection()
{
	fNameControl->MakeFocus(true);
	fNameControl->TextView()->Select(0,
		fNameControl->TextView()->TextLength()
			- fExtension.Length());
	fNameControl->TextView()->ScrollToSelection();
}


BString
ScreenshotWindow::_FindValidFileName(const char* name)
{
	BString baseName(name);

	if (fExtension.Compare(""))
		baseName.RemoveLast(fExtension);

	if (!fSaveScreenshotSilent && !fLastSelectedPath)
		return baseName;

	BPath orgPath(_GetDirectory());
	if (orgPath == NULL)
		return baseName;

	BTranslatorRoster* roster = BTranslatorRoster::Default();

	translator_id id = 0;
	if (_FindTranslator(fImageFileType, &id) == B_OK) {

		const translation_format* formats = NULL;

		int32 numFormats;
		if (roster->GetOutputFormats(id, &formats, &numFormats) == B_OK) {
			for (int32 i = 0; i < numFormats; ++i) {
				if (formats[i].type == uint32(fImageFileType)) {
					BMimeType mimeType(formats[i].MIME);
					BMessage msgExtensions;
					if (mimeType.GetFileExtensions(&msgExtensions) == B_OK) {
						const char* extension;
						if (msgExtensions.FindString("extensions", 0,
							&extension) == B_OK) {
							fExtension.SetTo(extension);
							fExtension.Prepend(".");
						} else
							fExtension.SetTo("");
					}
					break;
				}
			}
		}
	}

	BPath outputPath = orgPath;
	BString fileName;
	fileName << baseName << fExtension;
	outputPath.Append(fileName);

	if (!BEntry(outputPath.Path()).Exists())
		return fileName;

	if (baseName.FindFirst(B_TRANSLATE_COMMENT("screenshot",
		"!! Basename of screenshot files. !!")) == 0)
		baseName.SetTo(B_TRANSLATE_COMMENT("screenshot",
			"!! Basename of screenshot files. !!"));

	BEntry entry;
	int32 index = 1;
	char filename[B_FILE_NAME_LENGTH];
	do {
		sprintf(filename, "%s%ld%s", baseName.String(), index++,
			fExtension.String());
		outputPath.SetTo(orgPath.Path());
		outputPath.Append(filename);
		entry.SetTo(outputPath.Path());
	} while (entry.Exists());

	return BString(filename);
}


int32
ScreenshotWindow::_PathIndexInMenu(const BString& path) const
{
	BString userPath;
	for (int32 i = 0; i < fOutputPathMenu->CountItems(); ++i) {
		BMenuItem* item = fOutputPathMenu->ItemAt(i);
		if (item && item->Message()
			&& item->Message()->FindString("path", &userPath) == B_OK) {
			if (userPath == path)
				return i;
		}
	}
	return -1;
}


BMessage
ScreenshotWindow::_ReadSettings() const
{
	BMessage settings;

	BPath settingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath) != B_OK)
		return settings;
	settingsPath.Append("Screenshot_settings");

	BFile file(settingsPath.Path(), B_READ_ONLY);
	if (file.InitCheck() == B_OK)
		settings.Unflatten(&file);

	return settings;
}


void
ScreenshotWindow::_WriteSettings() const
{
	BMessage settings;

	settings.AddInt32("type", fImageFileType);
	settings.AddBool("includeBorder", fIncludeBorder);
	settings.AddBool("includeMouse", fIncludeMouse);
	settings.AddBool("grabActiveWindow", fGrabActiveWindow);
	settings.AddInt64("delay", fDelay);
	settings.AddString("outputFilename", fOutputFilename);

	BString path;
	int32 count = fOutputPathMenu->CountItems();
	if (count > 5) {
		for (int32 i = count - 3; i > count - 8 && i > 2; --i) {
			BMenuItem* item = fOutputPathMenu->ItemAt(i);
			if (item) {
				BMessage* msg = item->Message();
				if (msg && msg->FindString("path", &path) == B_OK)
					settings.AddString("path", path.String());
			}
		}
	}

	if (fLastSelectedPath) {
		BMessage* msg = fLastSelectedPath->Message();
		if (msg && msg->FindString("path", &path) == B_OK)
			settings.AddString("lastSelectedPath", path.String());
	}

	BPath settingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath) != B_OK)
		return;
	settingsPath.Append("Screenshot_settings");

	BFile file(settingsPath.Path(), B_CREATE_FILE | B_ERASE_FILE
		| B_WRITE_ONLY);
	if (file.InitCheck() == B_OK) {
		ssize_t size;
		settings.Flatten(&file, &size);
	}
}


void
ScreenshotWindow::_TakeScreenshot(bigtime_t delay)
{
	if (delay > 0)
		snooze(delay);

	BRect frame;
	delete fScreenshot;
	if (_GetActiveWindowFrame(&frame) == B_OK) {
		fScreenshot = new BBitmap(frame.OffsetToCopy(B_ORIGIN), B_RGBA32, true);
		BScreen(this).ReadBitmap(fScreenshot, fIncludeMouse, &frame);
		if (fIncludeBorder)
			_MakeTabSpaceTransparent(&frame);
	} else
		BScreen(this).GetBitmap(&fScreenshot, fIncludeMouse);
}


status_t
ScreenshotWindow::_GetActiveWindowFrame(BRect* frame)
{
	if (!fGrabActiveWindow || !frame)
		return B_ERROR;

	int32* tokens;
	int32 tokenCount;
	status_t status = BPrivate::get_window_order(current_workspace(), &tokens,
		&tokenCount);
	if (status != B_OK || !tokens || tokenCount < 1)
		return B_ERROR;

	status = B_ERROR;
	for (int32 i = 0; i < tokenCount; ++i) {
		client_window_info* windowInfo = get_window_info(tokens[i]);
		if (!windowInfo->is_mini && !windowInfo->show_hide_level > 0) {
			frame->left = windowInfo->window_left;
			frame->top = windowInfo->window_top;
			frame->right = windowInfo->window_right;
			frame->bottom = windowInfo->window_bottom;

			status = B_OK;
			if (fIncludeBorder) {
				float border = (windowInfo->border_size);
				frame->InsetBy(-(border), -(border));
				frame->top -= windowInfo->tab_height;
				fTabHeight = windowInfo->tab_height;
			}
			free(windowInfo);

			BRect screenFrame(BScreen(this).Frame());
			if (frame->left < screenFrame.left)
				frame->left = screenFrame.left;
			if (frame->top < screenFrame.top)
				frame->top = screenFrame.top;
			if (frame->right > screenFrame.right)
				frame->right = screenFrame.right;
			if (frame->bottom > screenFrame.bottom)
				frame->bottom = screenFrame.bottom;

			break;
		}
		free(windowInfo);
	}
	free(tokens);
	return status;
}


status_t
ScreenshotWindow::_SaveScreenshot()
{
	if (!fScreenshot || (!fSaveScreenshotSilent && !fLastSelectedPath))
		return B_ERROR;

	BPath path(_GetDirectory());

	if (path == NULL)
		return B_ERROR;

	if (fSaveScreenshotSilent) {
		if (!fOutputFilename.Compare("")) {
			path.Append(_FindValidFileName(B_TRANSLATE_COMMENT("screenshot1",
					"!! Filename of first screenshot !!")).String());
		} else
			path.SetTo(fOutputFilename);
	} else
		path.Append(fNameControl->Text());

	BEntry entry;
	entry.SetTo(path.Path());

	if (!fSaveScreenshotSilent) {
		if (entry.Exists()) {
			BAlert* overwriteAlert = new BAlert(
				B_TRANSLATE("overwrite"),
				B_TRANSLATE("This file already exists.\n Are you sure would "
					"you like to overwrite it?"),
				B_TRANSLATE("Cancel"),
				B_TRANSLATE("Overwrite"),
				NULL, B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);

				overwriteAlert->SetShortcut(0, B_ESCAPE);

				if (overwriteAlert->Go() == 0)
					return B_CANCELED;
		}
	}

	BFile file(&entry, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (file.InitCheck() != B_OK)
		return B_ERROR;

	BBitmapStream bitmapStream(fScreenshot);
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	roster->Translate(&bitmapStream, NULL, NULL, &file, fImageFileType,
		B_TRANSLATOR_BITMAP);
	fScreenshot = NULL;

	BNodeInfo nodeInfo(&file);
	if (nodeInfo.InitCheck() != B_OK)
		return B_ERROR;

	translator_id id = 0;
	if (_FindTranslator(fImageFileType, &id) != B_OK)
		return B_ERROR;

	int32 numFormats;
	const translation_format* formats = NULL;

	if (roster->GetOutputFormats(id, &formats, &numFormats) != B_OK)
		return B_ERROR;

	for (int32 i = 0; i < numFormats; ++i) {
		if (formats[i].type == uint32(fImageFileType)) {
			nodeInfo.SetType(formats[i].MIME);
			break;
		}
	}

	return B_OK;
}


void
ScreenshotWindow::_MakeTabSpaceTransparent(BRect* frame)
{
	if (!frame)
		return;

	if (fScreenshot->ColorSpace() != B_RGBA32)
		return;

	BRect fullFrame = *frame;

	BMessage message;
	BMessage reply;

	app_info appInfo;
	if (be_roster->GetActiveAppInfo(&appInfo) != B_OK)
		return;

	BMessenger messenger(appInfo.signature, appInfo.team);
	if (!messenger.IsValid())
		return;

	bool foundActiveWindow = false;
	int32 index = 0;

	while (true) {
		message.MakeEmpty();
		message.what = B_GET_PROPERTY;
		message.AddSpecifier("Active");
		message.AddSpecifier("Window", index);

		reply.MakeEmpty();
		messenger.SendMessage(&message, &reply);

		if (reply.what == B_MESSAGE_NOT_UNDERSTOOD)
			break;

		bool result;
		if (reply.FindBool("result", &result) == B_OK) {
			foundActiveWindow = result;

			if (foundActiveWindow)
				break;
		}
		index++;
	}

	if (!foundActiveWindow)
		return;

	message.MakeEmpty();
	message.what = B_GET_PROPERTY;
	message.AddSpecifier("TabFrame");
	message.AddSpecifier("Window", index);
	reply.MakeEmpty();
	messenger.SendMessage(&message, &reply);

	BRect tabFrame;
	if (reply.FindRect("result", &tabFrame) != B_OK)
		return;

	if (!fullFrame.Contains(tabFrame))
		return;

	BRegion tabSpace(fullFrame);
	fullFrame.OffsetBy(0, fTabHeight);
	tabSpace.Exclude(fullFrame);
	tabSpace.Exclude(tabFrame);
	fullFrame.OffsetBy(0, -fTabHeight);
	tabSpace.OffsetBy(-fullFrame.left, -fullFrame.top);
	BScreen screen;
	BRect screenFrame = screen.Frame();
	tabSpace.OffsetBy(-screenFrame.left, -screenFrame.top);

	BView view(fScreenshot->Bounds(), "bitmap", B_FOLLOW_ALL_SIDES, 0);
	fScreenshot->AddChild(&view);
	if (view.Looper() && view.Looper()->Lock()) {
		view.SetDrawingMode(B_OP_COPY);
		view.SetHighColor(B_TRANSPARENT_32_BIT);

		for (int i = 0; i < tabSpace.CountRects(); i++)
			view.FillRect(tabSpace.RectAt(i));

		view.Sync();
		view.Looper()->Unlock();
	}
	fScreenshot->RemoveChild(&view);
}


status_t
ScreenshotWindow::_FindTranslator(uint32 imageType, translator_id* id)
{
	translator_id* translators = NULL;
	int32 numTranslators = 0;
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	status_t status = roster->GetAllTranslators(&translators, &numTranslators);
	if (status != B_OK)
		return status;

	status = B_ERROR;
	for (int32 x = 0; x < numTranslators && status != B_OK; x++) {
		int32 numFormats;
		const translation_format* formats = NULL;

		if (roster->GetOutputFormats(x, &formats, &numFormats) == B_OK) {
			for (int32 i = 0; i < numFormats; ++i) {
				if (formats[i].type == imageType) {
					*id = x;
					status = B_OK;
					break;
				}
			}
		}
	}

	delete [] translators;

	return status;
}
