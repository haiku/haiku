/*
 * Copyright Karsten Heimrich, host.haiku@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ScreenshotWindow.h"

#include "PNGDump.h"


#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <BitmapStream.h>
#include <Button.h>
#include <CardLayout.h>
#include <CheckBox.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <FilePanel.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <LayoutItem.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <Path.h>
#include <RadioButton.h>
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


#include <stdio.h>
#include <stdlib.h>


enum {
	kScreenshotType,
	kIncludeBorder,
	kShowCursor,
	kBackToSave,
	kTakeScreenshot,
	kImageOutputFormat,
	kLocationChanged,
	kChooseLocation,
	kFinishScreenshot,
	kShowOptions
};


const int32 kPNGImageFileType = 1347307296;


// #pragma mark - DirectoryRefFilter


class DirectoryRefFilter : public BRefFilter {
public:
	virtual			~DirectoryRefFilter() {}
			bool	Filter(const entry_ref* ref, BNode* node, struct stat* stat,
						const char* filetype)
					{
						return node->IsDirectory();
					}
};


// #pragma mark - ScreenshotWindow


ScreenshotWindow::ScreenshotWindow(bigtime_t delay, bool includeBorder,
	bool includeCursor, bool grabActiveWindow, bool showConfigWindow,
	bool saveScreenshotSilent)
	: BWindow(BRect(0, 0, 200.0, 100.0), "Take Screenshot", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_QUIT_ON_WINDOW_CLOSE |
		B_AVOID_FRONT | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fScreenshot(NULL),
	fOutputPathPanel(NULL),
	fLastSelectedPath(NULL),
	fDelay(delay),
	fIncludeBorder(includeBorder),
	fIncludeCursor(includeCursor),
	fGrabActiveWindow(grabActiveWindow),
	fShowConfigWindow(showConfigWindow),
	fSaveScreenshotSilent(saveScreenshotSilent)
{
	_InitWindow();
	_CenterAndShow();
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
	switch (message->what) {
		case kScreenshotType: {
			fGrabActiveWindow = false;
			if (fActiveWindow->Value() == B_CONTROL_ON)
				fGrabActiveWindow = true;
			fWindowBorder->SetEnabled(fGrabActiveWindow);
		}	break;

		case kIncludeBorder: {
				fIncludeBorder = (fWindowBorder->Value() == B_CONTROL_ON);
		}	break;

		case kShowCursor: {
			fIncludeCursor = (fShowCursor->Value() == B_CONTROL_ON);
		}	break;

		case kBackToSave: {
			BCardLayout* layout = dynamic_cast<BCardLayout*> (GetLayout());
			if (layout)
				layout->SetVisibleItem(1L);
			SetTitle("Save Screenshot");
		}	break;

		case kTakeScreenshot: {
			Hide();
			_TakeScreenshot();
			Show();
		}	break;

		case kImageOutputFormat: {
			message->FindInt32("be:type", &fImageFileType);
			message->FindInt32("be:translator", &fTranslator);
		}	break;

		case kLocationChanged: {
			void* source = NULL;
			if (message->FindPointer("source", &source) == B_OK)
				fLastSelectedPath = static_cast<BMenuItem*> (source);

			const char* text = fNameControl->Text();
			fNameControl->SetText(_FindValidFileName(text).String());
		}	break;

		case kChooseLocation: {
			if (!fOutputPathPanel) {
				BMessenger target(this);
				fOutputPathPanel = new BFilePanel(B_OPEN_PANEL, &target,
					NULL, B_DIRECTORY_NODE, false, NULL, new DirectoryRefFilter());
				fOutputPathPanel->Window()->SetTitle("Choose directory");
				fOutputPathPanel->SetButtonLabel(B_DEFAULT_BUTTON, "Select");
			}
			fOutputPathPanel->Show();
		}	break;

		case B_CANCEL: {
			fLastSelectedPath->SetMarked(true);
		}	break;

		case B_REFS_RECEIVED: {
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BPath path(&ref);
				BMessage* message = new BMessage(kLocationChanged);
				message->AddString("path", path.Path());

				fLastSelectedPath = new BMenuItem(path.Path(), message);
				fLastSelectedPath->SetMarked(true);

				fOutputPathMenu->AddItem(fLastSelectedPath,
					fOutputPathMenu->CountItems() - 2);
			}
		}	break;

		case kFinishScreenshot: {
			_SaveScreenshot();
			be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		}	break;

		case kShowOptions: {
			BCardLayout* layout = dynamic_cast<BCardLayout*> (GetLayout());
			if (layout)
				layout->SetVisibleItem(0L);
			SetTitle("Take Screenshot");
			fBackToSave->SetEnabled(true);
		}	break;

		default: {
			BWindow::MessageReceived(message);
		}	break;
	};

}


void
ScreenshotWindow::_InitWindow()
{
	BCardLayout* layout = new BCardLayout();
	SetLayout(layout);

	_SetupFirstLayoutItem(layout);
	_SetupSecondLayoutItem(layout);

	if (!fShowConfigWindow) {
		_TakeScreenshot();
		layout->SetVisibleItem(1L);
	} else {
		layout->SetVisibleItem(0L);
	}
}


void
ScreenshotWindow::_SetupFirstLayoutItem(BCardLayout* layout)
{
	BStringView* stringView = new BStringView("", "Options");
	stringView->SetFont(be_bold_font);
	stringView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fActiveWindow = new BRadioButton("Take active window",
		 new BMessage(kScreenshotType));
	fWholeDesktop = new BRadioButton("Take whole Desktop",
		 new BMessage(kScreenshotType));
	fWholeDesktop->SetValue(B_CONTROL_ON);

	BString delay;
	delay << fDelay / 1000000;
	fDelayControl = new BTextControl("", "Take screenshot after a delay of",
		delay.String(), NULL);
	_DisallowChar(fDelayControl->TextView());
	fDelayControl->TextView()->SetAlignment(B_ALIGN_RIGHT);

	BStringView* stringView2 = new BStringView("", "seconds");
	stringView2->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fWindowBorder = new BCheckBox("Include window border",
		new BMessage(kIncludeBorder));
	fWindowBorder->SetEnabled(false);

	fShowCursor = new BCheckBox("Include cursor in screenshot",
		new BMessage(kShowCursor));
	fShowCursor->SetValue(fIncludeCursor);

	BBox* divider = new BBox(B_FANCY_BORDER, NULL);
	divider->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));

	fBackToSave = new BButton("", "Back to save", new BMessage(kBackToSave));
	fBackToSave->SetEnabled(false);

	fTakeScreenshot = new BButton("", "Take Screenshot",
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
			.Add(fShowCursor)
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
			.Add(new BButton("", "Cancel", new BMessage(B_QUIT_REQUESTED)))
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
ScreenshotWindow::_SetupSecondLayoutItem(BCardLayout* layout)
{
	fPreviewBox = new BBox(BRect(0.0, 0.0, 200.0, 150.0));
	fPreviewBox->SetExplicitMinSize(BSize(200.0, B_SIZE_UNSET));
	fPreviewBox->SetFlags(fPreviewBox->Flags() | B_FULL_UPDATE_ON_RESIZE);

	fNameControl = new BTextControl("", "Name:", "screenshot", NULL);

	_SetupTranslatorMenu(new BMenu("Please select"));
	BMenuField* menuField = new BMenuField("Save as:", fTranslatorMenu);

	_SetupOutputPathMenu(new BMenu("Please select"));
	BMenuField* menuField2 = new BMenuField("Save in:", fOutputPathMenu);

	fNameControl->SetText(_FindValidFileName("screenshot").String());

	BBox* divider = new BBox(B_FANCY_BORDER, NULL);
	divider->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));

	BGridLayout* gridLayout = BGridLayoutBuilder(0.0, 5.0)
		.Add(fNameControl->CreateLabelLayoutItem(), 0, 0)
		.Add(fNameControl->CreateTextViewLayoutItem(), 1, 0)
		.Add(menuField->CreateLabelLayoutItem(), 0, 1)
		.Add(menuField->CreateMenuBarLayoutItem(), 1, 1)
		.Add(menuField2->CreateLabelLayoutItem(), 0, 2)
		.Add(menuField2->CreateMenuBarLayoutItem(), 1, 2);
	gridLayout->SetMinColumnWidth(1, menuField->StringWidth("SomethingLongHere"));

	layout->AddView(1, BGroupLayoutBuilder(B_VERTICAL)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10.0)
			.Add(fPreviewBox)
			.AddGroup(B_VERTICAL)
				.Add(gridLayout->View())
				.AddGlue()
				.End())
		.AddStrut(10)
		.Add(divider)
		.AddStrut(10)
		.AddGroup(B_HORIZONTAL, 10.0)
			.Add(new BButton("", "Options", new BMessage(kShowOptions)))
			.AddGlue()
			.Add(new BButton("", "Cancel", new BMessage(B_QUIT_REQUESTED)))
			.Add(new BButton("", "Save", new BMessage(kFinishScreenshot)))
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

	for (int32 i = 0; i < fTranslatorMenu->CountItems(); ++i) {
		BMenuItem* item = fTranslatorMenu->ItemAt(i);
		if (item && item->Message()) {
			item->Message()->FindInt32("be:type", &fImageFileType);
			item->Message()->FindInt32("be:translator", &fTranslator);
			if (fImageFileType == kPNGImageFileType) {
				item->SetMarked(true);
				break;
			}
		}
	}
}


void
ScreenshotWindow::_SetupOutputPathMenu(BMenu* outputPathMenu)
{
	fOutputPathMenu = outputPathMenu;

	BPath path;
	find_directory(B_USER_DIRECTORY, &path);

	BMessage* message = new BMessage(kLocationChanged);
	message->AddString("path", path.Path());
	fOutputPathMenu->AddItem(new BMenuItem("Home directory", message));

	path.Append("Desktop");
	message = new BMessage(kLocationChanged);
	message->AddString("path", path.Path());
	fOutputPathMenu->AddItem(new BMenuItem("Desktop", message), 0);

	find_directory(B_BEOS_ETC_DIRECTORY, &path);
	path.Append("artwork");

	message = new BMessage(kLocationChanged);
	message->AddString("path", path.Path());
	fOutputPathMenu->AddItem(new BMenuItem("Artwork directory", message));

	fOutputPathMenu->AddItem(new BSeparatorItem());

	fOutputPathMenu->AddItem(new BMenuItem("Choose directory...",
		new BMessage(kChooseLocation)));

	fOutputPathMenu->SetLabelFromMarked(true);
	fOutputPathMenu->ItemAt(1)->SetMarked(true);
	fLastSelectedPath = fOutputPathMenu->ItemAt(1);
}


BString
ScreenshotWindow::_FindValidFileName(const char* name) const
{
	BString fileName(name);
	if (!fLastSelectedPath)
		return fileName;

	const char* path;
	BMessage* message = fLastSelectedPath->Message();
	if (!message || message->FindString("path", &path) != B_OK)
		return fileName;

	BPath outputPath(path);
	outputPath.Append(name);
	if (!BEntry(outputPath.Path()).Exists())
		return fileName;

	BEntry entry;
	int32 index = 1;
	char filename[32];
	do {
		sprintf(filename, "%s%ld", name, index++);
		outputPath.SetTo(path);
		outputPath.Append(filename);
		entry.SetTo(outputPath.Path());
	} while (entry.Exists());

	return BString(filename);
}


void
ScreenshotWindow::_CenterAndShow()
{
	if (fSaveScreenshotSilent) {
		_SaveScreenshotSilent();
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		return;
	}

	BSize size = GetLayout()->PreferredSize();
	ResizeTo(size.Width(), size.Height());

	BRect frame(BScreen(this).Frame());
	MoveTo((frame.Width() - size.Width()) / 2.0,
		(frame.Height() - size.Height()) / 2.0);

	Show();
}


void
ScreenshotWindow::_TakeScreenshot()
{
	snooze((atoi(fDelayControl->Text()) * 1000000) + 50000);

	BRect frame;
	delete fScreenshot;
	if (_GetActiveWindowFrame(&frame) == B_OK) {
		fScreenshot = new BBitmap(frame.OffsetToCopy(B_ORIGIN), B_RGBA32);
		BScreen(this).ReadBitmap(fScreenshot, fIncludeCursor, &frame);
	} else {
		BScreen(this).GetBitmap(&fScreenshot, fIncludeCursor);
	}

	fPreviewBox->ClearViewBitmap();
	fPreviewBox->SetViewBitmap(fScreenshot, fScreenshot->Bounds(),
		fPreviewBox->Bounds(), B_FOLLOW_ALL, 0);

	BCardLayout* layout = dynamic_cast<BCardLayout*> (GetLayout());
	if (layout)
		layout->SetVisibleItem(1L);

	SetTitle("Save Screenshot");
}


status_t
ScreenshotWindow::_GetActiveWindowFrame(BRect* frame)
{
	if (!fGrabActiveWindow || !frame)
		return B_ERROR;

	int32* tokens;
	int32 tokenCount;
	status_t status = BPrivate::get_window_order(B_CURRENT_WORKSPACE, &tokens,
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
			free(windowInfo);

			if (fIncludeBorder) {
				// TODO: that's wrong for windows without titlebar, change once
				//		 we can access the decorator or get it via window info
				frame->InsetBy(-5.0, -5.0);
				frame->top -= 22.0;
			}

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


void
ScreenshotWindow::_SaveScreenshot()
{
	if (!fScreenshot || !fLastSelectedPath)
		return;

	const char* path;
	BMessage* message = fLastSelectedPath->Message();
	if (!message || message->FindString("path", &path) != B_OK)
		return;

	BDirectory dir(path);
	BFile file(&dir, fNameControl->Text(), B_CREATE_FILE |
		B_ERASE_FILE | B_WRITE_ONLY);
	if (file.InitCheck() != B_OK)
		return;

	BBitmapStream bitmapStream(fScreenshot);
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	roster->Translate(&bitmapStream, NULL, NULL, &file, fImageFileType,
		B_TRANSLATOR_BITMAP);
	fScreenshot = NULL;

	BNodeInfo nodeInfo(&file);
	if (nodeInfo.InitCheck() != B_OK)
		return;

	int32 numFormats;
	const translation_format *formats = NULL;
	if (roster->GetOutputFormats(fTranslator, &formats, &numFormats) != B_OK)
		return;

	for (int32 i = 0; i < numFormats; ++i) {
		if (formats[i].type == uint32(fImageFileType)) {
			nodeInfo.SetType(formats[i].MIME);
			break;
		}
	}
}


void
ScreenshotWindow::_SaveScreenshotSilent() const
{
	if (!fScreenshot)
		return;

	BPath homePath;
	if (find_directory(B_USER_DIRECTORY, &homePath) != B_OK) {
		fprintf(stderr, "failed to find user home directory\n");
		return;
	}

	BPath path;
	BEntry entry;
	int32 index = 1;
	do {
		char filename[32];
		sprintf(filename, "screenshot%ld.png", index++);
		path = homePath;
		path.Append(filename);
		entry.SetTo(path.Path());
	} while (entry.Exists());

	// Dump to PNG
	SaveToPNG(path.Path(), fScreenshot->Bounds(), fScreenshot->ColorSpace(),
		fScreenshot->Bits(), fScreenshot->BitsLength(),
		fScreenshot->BytesPerRow());
}
