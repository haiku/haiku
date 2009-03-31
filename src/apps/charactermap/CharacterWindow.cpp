/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "CharacterWindow.h"

#include <stdio.h>

#include <Application.h>
#include <File.h>
#include <FindDirectory.h>
#include <GroupLayoutBuilder.h>
#include <ListView.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Roster.h>
#include <ScrollView.h>
#include <Slider.h>
#include <SplitLayoutBuilder.h>

#include "CharacterView.h"
#include "UnicodeBlocks.h"


static const uint32 kMsgUnicodeBlockSelected = 'unbs';
static const uint32 kMsgFontSelected = 'fnts';
static const uint32 kMsgFontSizeChanged = 'fsch';
static const uint32 kMsgPrivateBlocks = 'prbl';
static const uint32 kMsgContainedBlocks = 'cnbl';

static const int32 kMinFontSize = 10;
static const int32 kMaxFontSize = 72;


class FontSizeSlider : public BSlider {
public:
	FontSizeSlider(const char* name, const char* label, BMessage* message,
			int32 min, int32 max)
		: BSlider(name, label, NULL, min, max, B_HORIZONTAL)
	{
		SetModificationMessage(message);
	}
	
protected:
	const char* UpdateText() const
	{
		snprintf(fText, sizeof(fText), "%ldpt", Value());
		return fText;
	}

private:
	mutable char	fText[32];
};


CharacterWindow::CharacterWindow()
	: BWindow(BRect(100, 100, 500, 350), "CharacterMap", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	BMessage settings;
	_LoadSettings(settings);

	BRect frame;
	if (settings.FindRect("window frame", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}

	// create GUI

	SetLayout(new BGroupLayout(B_VERTICAL));

	BMenuBar* menuBar = new BMenuBar("menu");

	fUnicodeBlockView = new BListView("unicodeBlocks");
	fUnicodeBlockView->SetSelectionMessage(
		new BMessage(kMsgUnicodeBlockSelected));

	BScrollView* unicodeScroller = new BScrollView("unicodeScroller",
		fUnicodeBlockView, 0, false, true);

	fCharacterView = new CharacterView("characters");

	bool show;
	if (settings.FindBool("show private blocks", &show) == B_OK)
		fCharacterView->ShowPrivateBlocks(show);
	if (settings.FindBool("show contained blocks only", &show) == B_OK)
		fCharacterView->ShowContainedBlocksOnly(show);

	const char* family;
	const char* style;
	if (settings.FindString("font family", &family) == B_OK
		&& settings.FindString("font style", &style) == B_OK) {
		_SetFont(family, style);
	}

	int32 fontSize;
	if (settings.FindInt32("font size", &fontSize) == B_OK) {
		BFont font = fCharacterView->CharacterFont();
		if (fontSize < kMinFontSize)
			fontSize = kMinFontSize;
		else if (fontSize > kMaxFontSize)
			fontSize = kMaxFontSize;
		font.SetSize(fontSize);

		fCharacterView->SetCharacterFont(font);
	} else
		fontSize = (int32)fCharacterView->CharacterFont().Size();

	BScrollView* characterScroller = new BScrollView("characterScroller",
		fCharacterView, 0, false, true);

	fFontSizeSlider = new FontSizeSlider("fontSizeSlider", "Font Size:",
		new BMessage(kMsgFontSizeChanged), kMinFontSize, kMaxFontSize);
	fFontSizeSlider->SetValue(fontSize);

	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(menuBar)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)//BSplitLayoutBuilder()
			.Add(unicodeScroller)
			.Add(BGroupLayoutBuilder(B_VERTICAL, 10)
				.Add(characterScroller)
				.Add(fFontSizeSlider))
			.SetInsets(10, 10, 10, 10)));

	// Add menu

	// "File" menu
	BMenu* menu = new BMenu("File");
	BMenuItem* item;

	menu->AddItem(item = new BMenuItem("About CharacterMap" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));

	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	menu->SetTargetForItems(this);
	item->SetTarget(be_app);
	menuBar->AddItem(menu);

	menu = new BMenu("View");
	menu->AddItem(item = new BMenuItem("Show Private Blocks",
		new BMessage(kMsgPrivateBlocks)));
	item->SetMarked(fCharacterView->IsShowingPrivateBlocks());
// TODO: this feature is not yet supported by Haiku!
#if 0
	menu->AddItem(item = new BMenuItem("Only Show Blocks Contained in Font",
		new BMessage(kMsgContainedBlocks)));
	item->SetMarked(fCharacterView->IsShowingContainedBlocksOnly());
#endif
	menuBar->AddItem(menu);

	menuBar->AddItem(_CreateFontMenu());

	_CreateUnicodeBlocks();
	InvalidateLayout();
}


CharacterWindow::~CharacterWindow()
{
}


void
CharacterWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUnicodeBlockSelected:
		{
			int32 index;
			if (message->FindInt32("index", &index) != B_OK
				|| index < 0)
				break;

			fCharacterView->ScrollTo(index);
			break;
		}

		case kMsgFontSelected:
		{
			BMenuItem* item;
			if (message->FindPointer("source", (void**)&item) != B_OK)
				break;

			fSelectedFontItem->SetMarked(false);

			if (item != NULL) {
				item->SetMarked(true);
				fSelectedFontItem = item;

				_SetFont(item->Menu()->Name(), item->Label());
			}	
			break;
		}

		case kMsgFontSizeChanged:
		{
			int32 size = fFontSizeSlider->Value();
			if (size < kMinFontSize)
				size = kMinFontSize;
			else if (size > kMaxFontSize)
				size = kMaxFontSize;

			BFont font = fCharacterView->CharacterFont();
			font.SetSize(size);
			fCharacterView->SetCharacterFont(font);
			break;
		}

		case kMsgPrivateBlocks:
		{
			BMenuItem* item;
			if (message->FindPointer("source", (void**)&item) != B_OK
				|| item == NULL)
				break;
	
			item->SetMarked(!item->IsMarked());

			fCharacterView->ShowPrivateBlocks(item->IsMarked());
			_UpdateUnicodeBlocks();
			break;
		}

		case kMsgContainedBlocks:
		{
			BMenuItem* item;
			if (message->FindPointer("source", (void**)&item) != B_OK
				|| item == NULL)
				break;
	
			item->SetMarked(!item->IsMarked());

			fCharacterView->ShowContainedBlocksOnly(item->IsMarked());
			_UpdateUnicodeBlocks();
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
CharacterWindow::QuitRequested()
{
	_SaveSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


status_t
CharacterWindow::_OpenSettings(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("CharacterMap settings");

	return file.SetTo(path.Path(), mode);
}


status_t
CharacterWindow::_LoadSettings(BMessage& settings)
{
	BFile file;
	status_t status = _OpenSettings(file, B_READ_ONLY);
	if (status < B_OK)
		return status;

	return settings.Unflatten(&file);
}


status_t
CharacterWindow::_SaveSettings()
{
	BFile file;
	status_t status = _OpenSettings(file, B_WRITE_ONLY | B_CREATE_FILE
		| B_ERASE_FILE);
	if (status < B_OK)
		return status;

	BMessage settings('chrm');
	status = settings.AddRect("window frame", Frame());
	if (status != B_OK)
		return status;

	if (status == B_OK) {
		status = settings.AddBool("show private blocks",
			fCharacterView->IsShowingPrivateBlocks());
	}
	if (status == B_OK) {
		status = settings.AddBool("show contained blocks only",
			fCharacterView->IsShowingContainedBlocksOnly());
	}
	
	if (status == B_OK) {
		BFont font = fCharacterView->CharacterFont();
		status = settings.AddInt32("font size", font.Size());
		
		font_family family;
		font_style style;
		if (status == B_OK)
			font.GetFamilyAndStyle(&family, &style);
		if (status == B_OK)
			status = settings.AddString("font family", family);
		if (status == B_OK)
			status = settings.AddString("font style", style);
	}

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}


void
CharacterWindow::_SetFont(const char* family, const char* style)
{
	BFont font = fCharacterView->CharacterFont();
	font.SetFamilyAndStyle(family, style);

	fCharacterView->SetCharacterFont(font);
}


BMenu*
CharacterWindow::_CreateFontMenu()
{
	BMenu* menu = new BMenu("Font");

	font_family currentFamily;
	font_style currentStyle;
	fCharacterView->CharacterFont().GetFamilyAndStyle(&currentFamily,
		&currentStyle);

	int32 numFamilies = count_font_families();

	for (int32 i = 0; i < numFamilies; i++) {
		font_family family;
		if (get_font_family(i, &family) == B_OK) {
			BMenu* subMenu = new BMenu(family);
			menu->AddItem(new BMenuItem(subMenu,
				new BMessage(kMsgFontSelected)));

			int numStyles = count_font_styles(family);
			for (int32 j = 0; j < numStyles; j++) {
				font_style style;
				uint32 flags;
				if (get_font_style(family, j, &style, &flags) == B_OK) {
					BMenuItem* item = new BMenuItem(style,
						new BMessage(kMsgFontSelected));
					subMenu->AddItem(item);

					if (!strcmp(family, currentFamily)
						&& !strcmp(style, currentStyle)) {
						fSelectedFontItem = item;
						item->SetMarked(true);
					}
				}
			}
		}
	}

	return menu;
}


void
CharacterWindow::_UpdateUnicodeBlocks()
{
	for (int32 i = 0; i < fUnicodeBlockView->CountItems(); i++) {
		BStringItem* item
			= static_cast<BStringItem*>(fUnicodeBlockView->ItemAt(i));

		bool enabled = fCharacterView->IsShowingBlock(i);

		if (item->IsEnabled() != enabled) {
			item->SetEnabled(enabled);
			fUnicodeBlockView->InvalidateItem(i);
		}
	}
}


void
CharacterWindow::_CreateUnicodeBlocks()
{
	float minWidth = 0;
	for (uint32 i = 0; i < kNumUnicodeBlocks; i++) {
		BStringItem* item = new BStringItem(kUnicodeBlocks[i].name);
		fUnicodeBlockView->AddItem(item);

		float width = fUnicodeBlockView->StringWidth(item->Text());
		if (minWidth < width)
			minWidth = width;
	}

	fUnicodeBlockView->SetExplicitMinSize(BSize(minWidth / 2, 32));
	fUnicodeBlockView->SetExplicitMaxSize(BSize(minWidth, B_SIZE_UNSET));

	_UpdateUnicodeBlocks();
}

