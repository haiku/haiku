/*
 * Copyright 2002-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 *		Philippe Saint-Pierre
 *		Jonas Sundström
 *		Ryan Leavengood
 */


#include "Constants.h"
#include "ColorMenuItem.h"
#include "FindWindow.h"
#include "ReplaceWindow.h"
#include "StyledEditApp.h"
#include "StyledEditView.h"
#include "StyledEditWindow.h"

#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <Clipboard.h>
#include <Debug.h>
#include <File.h>
#include <FilePanel.h>
#include <fs_attr.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <PrintJob.h>
#include <Rect.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <UnicodeChar.h>


using namespace BPrivate;


const float kLineViewWidth = 30.0;
const char* kInfoAttributeName = "StyledEdit-info";


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StyledEditWindow"


// This is a temporary solution for building BString with printf like format.
// will be removed in the future.
static void
bs_printf(BString* string, const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	char* buf;
	vasprintf(&buf, format, ap);
	string->SetTo(buf);
	free(buf);
	va_end(ap);
}


// #pragma mark -


StyledEditWindow::StyledEditWindow(BRect frame, int32 id, uint32 encoding)
	: BWindow(frame, "untitled", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	_InitWindow(encoding);
	BString unTitled(B_TRANSLATE("Untitled "));
	unTitled << id;
	SetTitle(unTitled.String());
	fSaveItem->SetEnabled(true);
		// allow saving empty files
	Show();
}


StyledEditWindow::StyledEditWindow(BRect frame, entry_ref* ref, uint32 encoding)
	: BWindow(frame, "untitled", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	_InitWindow(encoding);
	OpenFile(ref);
	Show();
}


StyledEditWindow::~StyledEditWindow()
{
	delete fSaveMessage;
	delete fPrintSettings;
	delete fSavePanel;
}


void
StyledEditWindow::Quit()
{
	_SaveAttrs();
	if (StyledEditApp* app = dynamic_cast<StyledEditApp*>(be_app))
		app->CloseDocument();
	BWindow::Quit();
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "QuitAlert"


bool
StyledEditWindow::QuitRequested()
{
	if (fClean)
		return true;

	BString alertText;
	bs_printf(&alertText,
		B_TRANSLATE("Save changes to the document \"%s\"? "), Title());

	int32 index = _ShowAlert(alertText, B_TRANSLATE("Cancel"),
		B_TRANSLATE("Don't save"), B_TRANSLATE("Save"),	B_WARNING_ALERT);

	if (index == 0)
		return false;	// "cancel": dont save, dont close the window

	if (index == 1)
		return true;	// "don't save": just close the window

	if (!fSaveMessage) {
		SaveAs(new BMessage(SAVE_THEN_QUIT));
		return false;
	}

	return Save() == B_OK;
}


void
StyledEditWindow::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		entry_ref ref;
		if (message->FindRef("refs", 0, &ref)==B_OK) {
			message->what = B_REFS_RECEIVED;
			be_app->PostMessage(message);
		}
	}

	switch (message->what) {
		// File menu
		case MENU_SAVE:
			if (!fSaveMessage)
				SaveAs();
			else
				Save(fSaveMessage);
			break;

		case MENU_SAVEAS:
			SaveAs();
			break;

		case B_SAVE_REQUESTED:
			Save(message);
			break;

		case SAVE_THEN_QUIT:
			if (Save(message) == B_OK)
				Quit();
			break;

		case MENU_REVERT:
			_RevertToSaved();
			break;

		case MENU_CLOSE:
			if (QuitRequested())
				Quit();
			break;

		case MENU_PAGESETUP:
			PageSetup(fTextView->Window()->Title());
			break;
		case MENU_PRINT:
			Print(fTextView->Window()->Title());
			break;
		case MENU_QUIT:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		// Edit menu

		case B_UNDO:
			ASSERT(fCanUndo || fCanRedo);
			ASSERT(!(fCanUndo && fCanRedo));
			if (fCanUndo)
				fUndoFlag = true;
			if (fCanRedo)
				fRedoFlag = true;

			fTextView->Undo(be_clipboard);
			break;
		case B_CUT:
			fTextView->Cut(be_clipboard);
			break;
		case B_COPY:
			fTextView->Copy(be_clipboard);
			break;
		case B_PASTE:
			fTextView->Paste(be_clipboard);
			break;
		case MENU_CLEAR:
			fTextView->Clear();
			break;
		case MENU_FIND:
		{
			BRect findWindowFrame(100, 100, 400, 235);
			BWindow* window = new FindWindow(findWindowFrame, this,
				&fStringToFind, fCaseSensitive, fWrapAround, fBackSearch);
			window->Show();
			break;
		}
		case MSG_SEARCH:
			message->FindString("findtext", &fStringToFind);
			fFindAgainItem->SetEnabled(true);
			message->FindBool("casesens", &fCaseSensitive);
			message->FindBool("wrap", &fWrapAround);
			message->FindBool("backsearch", &fBackSearch);

			_Search(fStringToFind, fCaseSensitive, fWrapAround, fBackSearch);
			break;
		case MENU_FIND_AGAIN:
			_Search(fStringToFind, fCaseSensitive, fWrapAround, fBackSearch);
			break;
		case MENU_FIND_SELECTION:
			_FindSelection();
			break;
		case MENU_REPLACE:
		{
			BRect replaceWindowFrame(100, 100, 400, 284);
			BWindow* window = new ReplaceWindow(replaceWindowFrame, this,
				&fStringToFind, &fReplaceString, fCaseSensitive, fWrapAround,
				fBackSearch);
			window->Show();
			break;
		}
		case MSG_REPLACE:
		{
			message->FindBool("casesens", &fCaseSensitive);
			message->FindBool("wrap", &fWrapAround);
			message->FindBool("backsearch", &fBackSearch);

			message->FindString("FindText", &fStringToFind);
			message->FindString("ReplaceText", &fReplaceString);

			fFindAgainItem->SetEnabled(true);
			fReplaceSameItem->SetEnabled(true);

			_Replace(fStringToFind, fReplaceString, fCaseSensitive, fWrapAround,
				fBackSearch);
			break;
		}
		case MENU_REPLACE_SAME:
			_Replace(fStringToFind, fReplaceString, fCaseSensitive, fWrapAround,
				fBackSearch);
			break;

		case MSG_REPLACE_ALL:
		{
			message->FindBool("casesens", &fCaseSensitive);
			message->FindString("FindText",&fStringToFind);
			message->FindString("ReplaceText",&fReplaceString);

			bool allWindows;
			message->FindBool("allwindows", &allWindows);

			fFindAgainItem->SetEnabled(true);
			fReplaceSameItem->SetEnabled(true);

			if (allWindows)
				SearchAllWindows(fStringToFind, fReplaceString, fCaseSensitive);
			else
				_ReplaceAll(fStringToFind, fReplaceString, fCaseSensitive);
			break;
		}

		// Font menu

		case FONT_SIZE:
		{
			float fontSize;
			if (message->FindFloat("size", &fontSize) == B_OK)
				_SetFontSize(fontSize);
			break;
		}
		case FONT_FAMILY:
		{
			const char* fontFamily = NULL;
			const char* fontStyle = NULL;
			void* ptr;
			if (message->FindPointer("source", &ptr) == B_OK) {
				BMenuItem* item = static_cast<BMenuItem*>(ptr);
				fontFamily = item->Label();
			}

			BFont font;
			font.SetFamilyAndStyle(fontFamily, fontStyle);
			fItalicItem->SetMarked((font.Face() & B_ITALIC_FACE) != 0);
			fBoldItem->SetMarked((font.Face() & B_BOLD_FACE) != 0);

			_SetFontStyle(fontFamily, fontStyle);
			break;
		}
		case FONT_STYLE:
		{
			const char* fontFamily = NULL;
			const char* fontStyle = NULL;
			void* ptr;
			if (message->FindPointer("source", &ptr) == B_OK) {
				BMenuItem* item = static_cast<BMenuItem*>(ptr);
				fontStyle = item->Label();
				BMenu* menu = item->Menu();
				if (menu != NULL) {
					BMenuItem* super_item = menu->Superitem();
					if (super_item != NULL)
						fontFamily = super_item->Label();
				}
			}

			BFont font;
			font.SetFamilyAndStyle(fontFamily, fontStyle);
			fItalicItem->SetMarked((font.Face() & B_ITALIC_FACE) != 0);
			fBoldItem->SetMarked((font.Face() & B_BOLD_FACE) != 0);

			_SetFontStyle(fontFamily, fontStyle);
			break;
		}
		case kMsgSetItalic:
		{
			uint32 sameProperties;
			BFont font;
			fTextView->GetFontAndColor(&font, &sameProperties);

			if (fItalicItem->IsMarked())
				font.SetFace(B_REGULAR_FACE);
			fItalicItem->SetMarked(!fItalicItem->IsMarked());

			font_family family;
			font_style style;
			font.GetFamilyAndStyle(&family, &style);

			_SetFontStyle(family, style);
			break;
		}
		case kMsgSetBold:
		{
			uint32 sameProperties;
			BFont font;
			fTextView->GetFontAndColor(&font, &sameProperties);

			if (fBoldItem->IsMarked())
				font.SetFace(B_REGULAR_FACE);
			fBoldItem->SetMarked(!fBoldItem->IsMarked());

			font_family family;
			font_style style;
			font.GetFamilyAndStyle(&family, &style);

			_SetFontStyle(family, style);
			break;
		}
		case FONT_COLOR:
		{
			void* ptr;
			if (message->FindPointer("source", &ptr) == B_OK) {
				if (ptr == fBlackItem)
					_SetFontColor(&BLACK);
				else if (ptr == fRedItem)
					_SetFontColor(&RED);
				else if (ptr == fGreenItem)
					_SetFontColor(&GREEN);
				else if (ptr == fBlueItem)
					_SetFontColor(&BLUE);
				else if (ptr == fCyanItem)
					_SetFontColor(&CYAN);
				else if (ptr == fMagentaItem)
					_SetFontColor(&MAGENTA);
				else if (ptr == fYellowItem)
					_SetFontColor(&YELLOW);
			}
			break;
		}

		// Document menu

		case ALIGN_LEFT:
			fTextView->SetAlignment(B_ALIGN_LEFT);
			_UpdateCleanUndoRedoSaveRevert();
			break;
		case ALIGN_CENTER:
			fTextView->SetAlignment(B_ALIGN_CENTER);
			_UpdateCleanUndoRedoSaveRevert();
			break;
		case ALIGN_RIGHT:
			fTextView->SetAlignment(B_ALIGN_RIGHT);
			_UpdateCleanUndoRedoSaveRevert();
			break;
		case WRAP_LINES:
		{
			BRect textRect(fTextView->Bounds());
			textRect.OffsetTo(B_ORIGIN);
			textRect.InsetBy(TEXT_INSET, TEXT_INSET);
			if (fTextView->DoesWordWrap()) {
				fTextView->SetWordWrap(false);
				fWrapItem->SetMarked(false);
				// the width comes from stylededit R5. TODO: find a better way
				textRect.SetRightBottom(BPoint(1500.0, textRect.RightBottom().y));
			} else {
				fTextView->SetWordWrap(true);
				fWrapItem->SetMarked(true);
			}
			fTextView->SetTextRect(textRect);

			_UpdateCleanUndoRedoSaveRevert();
			break;
		}
		case SHOW_STATISTICS:
			_ShowStatistics();
			break;
		case ENABLE_ITEMS:
			fCutItem->SetEnabled(true);
			fCopyItem->SetEnabled(true);
			break;
		case DISABLE_ITEMS:
			fCutItem->SetEnabled(false);
			fCopyItem->SetEnabled(false);
			break;
		case TEXT_CHANGED:
			if (fUndoFlag) {
				if (fUndoCleans) {
					// we cleaned!
					fClean = true;
					fUndoCleans = false;
				} else if (fClean) {
					// if we were clean
					// then a redo will make us clean again
					fRedoCleans = true;
					fClean = false;
				}
				// set mode
				fCanUndo = false;
				fCanRedo = true;
				fUndoItem->SetLabel(B_TRANSLATE("Redo typing"));
				fUndoItem->SetEnabled(true);
				fUndoFlag = false;
			} else {
				if (fRedoFlag && fRedoCleans) {
					// we cleaned!
					fClean = true;
					fRedoCleans = false;
				} else if (fClean) {
					// if we were clean
					// then an undo will make us clean again
					fUndoCleans = true;
					fClean = false;
				} else {
					// no more cleaning from undo now...
					fUndoCleans = false;
				}
				// set mode
				fCanUndo = true;
				fCanRedo = false;
				fUndoItem->SetLabel(B_TRANSLATE("Undo typing"));
				fUndoItem->SetEnabled(true);
				fRedoFlag = false;
			}
			if (fClean) {
				fRevertItem->SetEnabled(false);
				fSaveItem->SetEnabled(fSaveMessage == NULL);
			} else {
				fRevertItem->SetEnabled(fSaveMessage != NULL);
				fSaveItem->SetEnabled(true);
			}
			break;

		case SAVE_AS_ENCODING:
			void* ptr;
			if (message->FindPointer("source", &ptr) == B_OK
				&& fSavePanelEncodingMenu != NULL) {
				fTextView->SetEncoding(
					(uint32)fSavePanelEncodingMenu->IndexOf((BMenuItem*)ptr));
			}
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
StyledEditWindow::MenusBeginning()
{
	// set up the recent documents menu
	BMessage documents;
	be_roster->GetRecentDocuments(&documents, 9, NULL, APP_SIGNATURE);

	// delete old items..
	//    shatty: it would be preferable to keep the old
	//            menu around instead of continuously thrashing
	//            the menu, but unfortunately there does not
	//            seem to be a straightforward way to update it
	// going backwards may simplify memory management
	for (int i = fRecentMenu->CountItems(); i-- > 0;) {
		delete fRecentMenu->RemoveItem(i);
	}

	// add new items
	int count = 0;
	entry_ref ref;
	while (documents.FindRef("refs", count++, &ref) == B_OK) {
		if (ref.device != -1 && ref.directory != -1) {
			// sanity check passed
			BMessage* openRecent = new BMessage(B_REFS_RECEIVED);
			openRecent->AddRef("refs", &ref);
			BMenuItem* item = new BMenuItem(ref.name, openRecent);
			item->SetTarget(be_app);
			fRecentMenu->AddItem(item);
		}
	}

	// update the font menu
	// unselect the old values
	if (fCurrentFontItem != NULL) {
		fCurrentFontItem->SetMarked(false);
		BMenu* menu = fCurrentFontItem->Submenu();
		if (menu != NULL) {
			BMenuItem* item = menu->FindMarked();
			if (item != NULL) {
				item->SetMarked(false);
			}
		}
	}

	if (fCurrentStyleItem != NULL) {
		fCurrentStyleItem->SetMarked(false);
	}

	BMenuItem* oldColorItem = fFontColorMenu->FindMarked();
	if (oldColorItem != NULL)
		oldColorItem->SetMarked(false);

	BMenuItem* oldSizeItem = fFontSizeMenu->FindMarked();
	if (oldSizeItem != NULL)
		oldSizeItem->SetMarked(false);

	// find the current font, color, size
	BFont font;
	uint32 sameProperties;
	rgb_color color = BLACK;
	bool sameColor;
	fTextView->GetFontAndColor(&font, &sameProperties, &color, &sameColor);

	if (sameColor && color.alpha == 255) {
		// select the current color
		if (color.red == 0) {
			if (color.green == 0) {
				if (color.blue == 0) {
					fBlackItem->SetMarked(true);
				} else if (color.blue == 255) {
					fBlueItem->SetMarked(true);
				}
			} else if (color.green == 255) {
				if (color.blue == 0) {
					fGreenItem->SetMarked(true);
				} else if (color.blue == 255) {
					fCyanItem->SetMarked(true);
				}
			}
		} else if (color.red == 255) {
			if (color.green == 0) {
				if (color.blue == 0) {
					fRedItem->SetMarked(true);
				} else if (color.blue == 255) {
					fMagentaItem->SetMarked(true);
				}
			} else if (color.green == 255) {
				if (color.blue == 0) {
					fYellowItem->SetMarked(true);
				}
			}
		}
	}

	if (sameProperties & B_FONT_SIZE) {
		if ((int)font.Size() == font.Size()) {
			// select the current font size
			char fontSizeStr[16];
			snprintf(fontSizeStr, 15, "%i", (int)font.Size());
			BMenuItem* item = fFontSizeMenu->FindItem(fontSizeStr);
			if (item != NULL)
				item->SetMarked(true);
		}
	}

	font_family family;
	font_style style;
	font.GetFamilyAndStyle(&family, &style);

	fCurrentFontItem = fFontMenu->FindItem(family);

	if (fCurrentFontItem != NULL) {
		fCurrentFontItem->SetMarked(true);
		BMenu* menu = fCurrentFontItem->Submenu();
		if (menu != NULL) {
			BMenuItem* item = menu->FindItem(style);
			fCurrentStyleItem = item;
			if (fCurrentStyleItem != NULL)
				item->SetMarked(true);
		}
	}

	fBoldItem->SetMarked((font.Face() & B_BOLD_FACE) != 0);
	fItalicItem->SetMarked((font.Face() & B_ITALIC_FACE) != 0);

	switch (fTextView->Alignment()) {
		case B_ALIGN_LEFT:
		default:
			fAlignLeft->SetMarked(true);
			break;
		case B_ALIGN_CENTER:
			fAlignCenter->SetMarked(true);
			break;
		case B_ALIGN_RIGHT:
			fAlignRight->SetMarked(true);
			break;
	}
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SaveAlert"


status_t
StyledEditWindow::Save(BMessage* message)
{
	if (!message)
		message = fSaveMessage;

	if (!message)
		return B_ERROR;

	entry_ref dirRef;
	const char* name;
	if (message->FindRef("directory", &dirRef) != B_OK
		|| message->FindString("name", &name) != B_OK)
		return B_BAD_VALUE;

	BDirectory dir(&dirRef);
	BEntry entry(&dir, name);

	status_t status = B_ERROR;
	if (dir.InitCheck() == B_OK && entry.InitCheck() == B_OK) {
		struct stat st;
		BFile file(&entry, B_READ_WRITE | B_CREATE_FILE);
		if (file.InitCheck() == B_OK
			&& (status = file.GetStat(&st)) == B_OK) {
			// check the file permissions
			if (!((getuid() == st.st_uid && (S_IWUSR & st.st_mode))
				|| (getgid() == st.st_gid && (S_IWGRP & st.st_mode))
				|| (S_IWOTH & st.st_mode))) {
				BString alertText;
				bs_printf(&alertText, B_TRANSLATE("This file is marked "
					"Read-Only. Save changes to the document \"%s\"? "), name);
				switch (_ShowAlert(alertText, B_TRANSLATE("Cancel"),
						B_TRANSLATE("Don't save"),
						B_TRANSLATE("Save"), B_WARNING_ALERT)) {
					case 0:
						return B_CANCELED;
					case 1:
						return B_OK;
					default:
						break;
				}
			}

			status = fTextView->WriteStyledEditFile(&file);
		}
	}

	if (status != B_OK) {
		BString alertText;
		bs_printf(&alertText, B_TRANSLATE("Error saving \"%s\":\n%s"), name,
			strerror(status));

		_ShowAlert(alertText, B_TRANSLATE("OK"), "", "", B_STOP_ALERT);
		return status;
	}

	SetTitle(name);

	if (fSaveMessage != message) {
		delete fSaveMessage;
		fSaveMessage = new BMessage(*message);
	}

	entry_ref ref;
	if (entry.GetRef(&ref) == B_OK)
		be_roster->AddToRecentDocuments(&ref, APP_SIGNATURE);

	// clear clean modes
	fSaveItem->SetEnabled(false);
	fRevertItem->SetEnabled(false);
	fUndoCleans = false;
	fRedoCleans = false;
	fClean = true;
	return status;
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Open_and_SaveAsPanel"


status_t
StyledEditWindow::SaveAs(BMessage* message)
{
	if (fSavePanel == NULL) {
		entry_ref* directory = NULL;
		entry_ref dirRef;
		if (fSaveMessage != NULL) {
			if (fSaveMessage->FindRef("directory", &dirRef) == B_OK)
				directory = &dirRef;
		}

		BMessenger target(this);
		fSavePanel = new BFilePanel(B_SAVE_PANEL, &target,
			directory, B_FILE_NODE, false);

		BMenuBar* menuBar = dynamic_cast<BMenuBar*>(
			fSavePanel->Window()->FindView("MenuBar"));
		if (menuBar != NULL) {
			fSavePanelEncodingMenu = new BMenu(B_TRANSLATE("Encoding"));
			fSavePanelEncodingMenu->SetRadioMode(true);
			menuBar->AddItem(fSavePanelEncodingMenu);

			BCharacterSetRoster roster;
			BCharacterSet charset;
			while (roster.GetNextCharacterSet(&charset) == B_NO_ERROR) {
				BString name(charset.GetPrintName());
				const char* mime = charset.GetMIMEName();
				if (mime) {
					name.Append(" (");
					name.Append(mime);
					name.Append(")");
				}
				BMenuItem * item = new BMenuItem(name.String(),
					new BMessage(SAVE_AS_ENCODING));
				item->SetTarget(this);
				fSavePanelEncodingMenu->AddItem(item);
				if (charset.GetFontID() == fTextView->GetEncoding())
					item->SetMarked(true);
			}
		}
	}

	fSavePanel->SetSaveText(Title());
	if (message != NULL)
		fSavePanel->SetMessage(message);

	fSavePanel->Show();
	return B_OK;
}


void
StyledEditWindow::OpenFile(entry_ref* ref)
{
	if (_LoadFile(ref) != B_OK) {
		fSaveItem->SetEnabled(true);
			// allow saving new files
		return;
	}

	be_roster->AddToRecentDocuments(ref, APP_SIGNATURE);
	fSaveMessage = new BMessage(B_SAVE_REQUESTED);
	if (fSaveMessage) {
		BEntry entry(ref, true);
		BEntry parent;
		entry_ref parentRef;
		char name[B_FILE_NAME_LENGTH];

		entry.GetParent(&parent);
		entry.GetName(name);
		parent.GetRef(&parentRef);
		fSaveMessage->AddRef("directory", &parentRef);
		fSaveMessage->AddString("name", name);
		SetTitle(name);

		_LoadAttrs();
	}
	fTextView->Select(0, 0);
}


status_t
StyledEditWindow::PageSetup(const char* documentName)
{
	BPrintJob printJob(documentName);

	if (fPrintSettings != NULL)
		printJob.SetSettings(new BMessage(*fPrintSettings));

	status_t result = printJob.ConfigPage();
	if (result == B_OK) {
		delete fPrintSettings;
		fPrintSettings = printJob.Settings();
	}

	return result;
}


void
StyledEditWindow::Print(const char* documentName)
{
	BPrintJob printJob(documentName);
	if (fPrintSettings)
		printJob.SetSettings(new BMessage(*fPrintSettings));

	if (printJob.ConfigJob() != B_OK)
 		return;

	delete fPrintSettings;
	fPrintSettings = printJob.Settings();

	// information from printJob
	BRect printableRect = printJob.PrintableRect();
	int32 firstPage = printJob.FirstPage();
	int32 lastPage = printJob.LastPage();

	// lines eventually to be used to compute pages to print
	int32 firstLine = 0;
	int32 lastLine = fTextView->CountLines();

	// values to be computed
	int32 pagesInDocument = 1;
	int32 linesInDocument = fTextView->CountLines();

	int32 currentLine = 0;
	while (currentLine < linesInDocument) {
		float currentHeight = 0;
		while (currentHeight < printableRect.Height() && currentLine
				< linesInDocument) {
			currentHeight += fTextView->LineHeight(currentLine);
			if (currentHeight < printableRect.Height())
				currentLine++;
		}
		if (pagesInDocument == lastPage)
			lastLine = currentLine - 1;

		if (currentHeight >= printableRect.Height()) {
			pagesInDocument++;
			if (pagesInDocument == firstPage)
				firstLine = currentLine;
		}
	}

	if (lastPage > pagesInDocument - 1) {
		lastPage = pagesInDocument - 1;
		lastLine = currentLine - 1;
	}


	printJob.BeginJob();
	if (fTextView->CountLines() > 0 && fTextView->TextLength() > 0) {
		int32 printLine = firstLine;
		while (printLine <= lastLine) {
			float currentHeight = 0;
			int32 firstLineOnPage = printLine;
			while (currentHeight < printableRect.Height() && printLine <= lastLine) {
				currentHeight += fTextView->LineHeight(printLine);
				if (currentHeight < printableRect.Height())
					printLine++;
			}

			float top = 0;
			if (firstLineOnPage != 0)
				top = fTextView->TextHeight(0, firstLineOnPage - 1);

			float bottom = fTextView->TextHeight(0, printLine - 1);
			BRect textRect(0.0, top + TEXT_INSET,
				printableRect.Width(), bottom + TEXT_INSET);
			printJob.DrawView(fTextView, textRect, B_ORIGIN);
			printJob.SpoolPage();
		}
	}


	printJob.CommitJob();
}


void
StyledEditWindow::SearchAllWindows(BString find, BString replace,
	bool caseSensitive)
{
	int32 numWindows;
	numWindows = be_app->CountWindows();

	BMessage* message;
	message= new BMessage(MSG_REPLACE_ALL);
	message->AddString("FindText", find);
	message->AddString("ReplaceText", replace);
	message->AddBool("casesens", caseSensitive);

	while (numWindows >= 0) {
		StyledEditWindow* window = dynamic_cast<StyledEditWindow *>(
			be_app->WindowAt(numWindows));

		BMessenger messenger(window);
		messenger.SendMessage(message);

		numWindows--;
	}
}


bool
StyledEditWindow::IsDocumentEntryRef(const entry_ref* ref)
{
	if (ref == NULL)
		return false;

	if (fSaveMessage == NULL)
		return false;

	entry_ref dir;
	const char* name;
	if (fSaveMessage->FindRef("directory", &dir) != B_OK
		|| fSaveMessage->FindString("name", &name) != B_OK)
		return false;

	entry_ref documentRef;
	BPath documentPath(&dir);
	documentPath.Append(name);
	get_ref_for_path(documentPath.Path(), &documentRef);

	return *ref == documentRef;
}


// #pragma mark - private methods


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Menus"


void
StyledEditWindow::_InitWindow(uint32 encoding)
{
	fPrintSettings = NULL;
	fSaveMessage = NULL;

	// undo modes
	fUndoFlag = false;
	fCanUndo = false;
	fRedoFlag = false;
	fCanRedo = false;

	// clean modes
	fUndoCleans = false;
	fRedoCleans = false;
	fClean = true;

	// search- state
	fReplaceString = "";
	fStringToFind = "";
	fCaseSensitive = false;
	fWrapAround = false;
	fBackSearch = false;

	// add menubar
	fMenuBar = new BMenuBar(BRect(0, 0, 0, 0), "menubar");
	AddChild(fMenuBar);

	// add textview and scrollview

	BRect viewFrame = Bounds();
	viewFrame.top = fMenuBar->Bounds().Height() + 1;
	viewFrame.right -=  B_V_SCROLL_BAR_WIDTH;
	viewFrame.left = 0;
	viewFrame.bottom -= B_H_SCROLL_BAR_HEIGHT;

	BRect textBounds = viewFrame;
	textBounds.OffsetTo(B_ORIGIN);
	textBounds.InsetBy(TEXT_INSET, TEXT_INSET);

	fTextView= new StyledEditView(viewFrame, textBounds, this);
	fTextView->SetDoesUndo(true);
	fTextView->SetStylable(true);
	fTextView->SetEncoding(encoding);

	fScrollView = new BScrollView("scrollview", fTextView, B_FOLLOW_ALL, 0,
		true, true, B_PLAIN_BORDER);
	AddChild(fScrollView);
	fTextView->MakeFocus(true);

	// Add "File"-menu:
	BMenu* menu = new BMenu(B_TRANSLATE("File"));
	fMenuBar->AddItem(menu);

	BMenuItem* menuItem;
	menu->AddItem(menuItem = new BMenuItem(B_TRANSLATE("New"),
		new BMessage(MENU_NEW), 'N'));
	menuItem->SetTarget(be_app);

	menu->AddItem(menuItem = new BMenuItem(fRecentMenu
		= new BMenu(B_TRANSLATE("Open" B_UTF8_ELLIPSIS)),
			new BMessage(MENU_OPEN)));
	menuItem->SetShortcut('O', 0);
	menuItem->SetTarget(be_app);
	menu->AddSeparatorItem();

	menu->AddItem(fSaveItem = new BMenuItem(B_TRANSLATE("Save"),
		new BMessage(MENU_SAVE), 'S'));
	fSaveItem->SetEnabled(false);
	menu->AddItem(menuItem = new BMenuItem(
		B_TRANSLATE("Save as" B_UTF8_ELLIPSIS), new BMessage(MENU_SAVEAS)));
	menuItem->SetShortcut('S', B_SHIFT_KEY);
	menuItem->SetEnabled(true);

	menu->AddItem(fRevertItem
		= new BMenuItem(B_TRANSLATE("Revert to saved" B_UTF8_ELLIPSIS),
		new BMessage(MENU_REVERT)));
	fRevertItem->SetEnabled(false);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(MENU_CLOSE), 'W'));

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Page setup" B_UTF8_ELLIPSIS),
		new BMessage(MENU_PAGESETUP)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Print" B_UTF8_ELLIPSIS),
		new BMessage(MENU_PRINT), 'P'));

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(MENU_QUIT), 'Q'));

	// Add the "Edit"-menu:
	menu = new BMenu(B_TRANSLATE("Edit"));
	fMenuBar->AddItem(menu);

	menu->AddItem(fUndoItem = new BMenuItem(B_TRANSLATE("Can't undo"),
		new BMessage(B_UNDO), 'Z'));
	fUndoItem->SetEnabled(false);

	menu->AddSeparatorItem();
	menu->AddItem(fCutItem = new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X'));
	fCutItem->SetEnabled(false);
	fCutItem->SetTarget(fTextView);

	menu->AddItem(fCopyItem = new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C'));
	fCopyItem->SetEnabled(false);
	fCopyItem->SetTarget(fTextView);

	menu->AddItem(menuItem = new BMenuItem(B_TRANSLATE("Paste"),
		new BMessage(B_PASTE), 'V'));
	menuItem->SetTarget(fTextView);

	menu->AddSeparatorItem();
	menu->AddItem(menuItem = new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A'));
	menuItem->SetTarget(fTextView);

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Find" B_UTF8_ELLIPSIS),
		new BMessage(MENU_FIND), 'F'));
	menu->AddItem(fFindAgainItem= new BMenuItem(B_TRANSLATE("Find again"),
		new BMessage(MENU_FIND_AGAIN), 'G'));
	fFindAgainItem->SetEnabled(false);

	menu->AddItem(new BMenuItem(B_TRANSLATE("Find selection"),
		new BMessage(MENU_FIND_SELECTION), 'H'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Replace" B_UTF8_ELLIPSIS),
		new BMessage(MENU_REPLACE), 'R'));
	menu->AddItem(fReplaceSameItem = new BMenuItem(B_TRANSLATE("Replace next"),
		new BMessage(MENU_REPLACE_SAME), 'T'));
	fReplaceSameItem->SetEnabled(false);

	// Add the "Font"-menu:
	fFontMenu = new BMenu(B_TRANSLATE("Font"));
	fMenuBar->AddItem(fFontMenu);

	//"Size"-subMenu
	fFontSizeMenu = new BMenu(B_TRANSLATE("Size"));
	fFontSizeMenu->SetRadioMode(true);
	fFontMenu->AddItem(fFontSizeMenu);

	const int32 fontSizes[] = {9, 10, 11, 12, 14, 18, 24, 36, 48, 72};
	for (uint32 i = 0; i < sizeof(fontSizes) / sizeof(fontSizes[0]); i++) {
		BMessage* fontMessage = new BMessage(FONT_SIZE);
		fontMessage->AddFloat("size", fontSizes[i]);

		char label[64];
		snprintf(label, sizeof(label), "%ld", fontSizes[i]);
		fFontSizeMenu->AddItem(menuItem = new BMenuItem(label, fontMessage));

		if (fontSizes[i] == (int32)be_plain_font->Size())
			menuItem->SetMarked(true);
	}

	// "Color"-subMenu
	fFontColorMenu = new BMenu(B_TRANSLATE("Color"));
	fFontColorMenu->SetRadioMode(true);
	fFontMenu->AddItem(fFontColorMenu);

	fFontColorMenu->AddItem(fBlackItem = new BMenuItem(B_TRANSLATE("Black"),
		new BMessage(FONT_COLOR)));
	fBlackItem->SetMarked(true);
	fFontColorMenu->AddItem(fRedItem = new ColorMenuItem(B_TRANSLATE("Red"),
		RED, new BMessage(FONT_COLOR)));
	fFontColorMenu->AddItem(fGreenItem = new ColorMenuItem(B_TRANSLATE("Green"),
		GREEN, new BMessage(FONT_COLOR)));
	fFontColorMenu->AddItem(fBlueItem = new ColorMenuItem(B_TRANSLATE("Blue"),
		BLUE, new BMessage(FONT_COLOR)));
	fFontColorMenu->AddItem(fCyanItem = new ColorMenuItem(B_TRANSLATE("Cyan"),
		CYAN, new BMessage(FONT_COLOR)));
	fFontColorMenu->AddItem(fMagentaItem = new ColorMenuItem(B_TRANSLATE("Magenta"),
		MAGENTA, new BMessage(FONT_COLOR)));
	fFontColorMenu->AddItem(fYellowItem = new ColorMenuItem(B_TRANSLATE("Yellow"),
		YELLOW, new BMessage(FONT_COLOR)));
	fFontMenu->AddSeparatorItem();

	// "Bold" & "Italic" menu items
	fFontMenu->AddItem(fBoldItem = new BMenuItem(B_TRANSLATE("Bold"),
		new BMessage(kMsgSetBold)));
	fFontMenu->AddItem(fItalicItem = new BMenuItem(B_TRANSLATE("Italic"),
		new BMessage(kMsgSetItalic)));
	fBoldItem->SetShortcut('B', 0);
	fItalicItem->SetShortcut('I', 0);
	fFontMenu->AddSeparatorItem();

	// Available fonts

	fCurrentFontItem = 0;
	fCurrentStyleItem = 0;

	BMenu* subMenu;
	int32 numFamilies = count_font_families();
	for (int32 i = 0; i < numFamilies; i++) {
		font_family family;
		if (get_font_family(i, &family) == B_OK) {
			subMenu = new BMenu(family);
			subMenu->SetRadioMode(true);
			fFontMenu->AddItem(new BMenuItem(subMenu,
				new BMessage(FONT_FAMILY)));

			int32 numStyles = count_font_styles(family);
			for (int32 j = 0; j < numStyles; j++) {
				font_style style;
				uint32 flags;
				if (get_font_style(family, j, &style, &flags) == B_OK) {
					subMenu->AddItem(new BMenuItem(style,
						new BMessage(FONT_STYLE)));
				}
			}
		}
	}

	// Add the "Document"-menu:
	menu = new BMenu(B_TRANSLATE("Document"));
	fMenuBar->AddItem(menu);

	// "Align"-subMenu:
	subMenu = new BMenu(B_TRANSLATE("Align"));
	subMenu->SetRadioMode(true);

	subMenu->AddItem(fAlignLeft = new BMenuItem(B_TRANSLATE("Left"),
		new BMessage(ALIGN_LEFT)));
	fAlignLeft->SetMarked(true);
	fAlignLeft->SetShortcut('L', B_OPTION_KEY);

	subMenu->AddItem(fAlignCenter = new BMenuItem(B_TRANSLATE("Center"),
		new BMessage(ALIGN_CENTER)));
	fAlignCenter->SetShortcut('C', B_OPTION_KEY);

	subMenu->AddItem(fAlignRight = new BMenuItem(B_TRANSLATE("Right"),
		new BMessage(ALIGN_RIGHT)));
	fAlignRight->SetShortcut('R', B_OPTION_KEY);

	menu->AddItem(subMenu);
	menu->AddItem(fWrapItem = new BMenuItem(B_TRANSLATE("Wrap lines"),
		new BMessage(WRAP_LINES)));
	fWrapItem->SetMarked(true);
	fWrapItem->SetShortcut('W', B_OPTION_KEY);

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Statistics" B_UTF8_ELLIPSIS),
		new BMessage(SHOW_STATISTICS)));

	fSavePanel = NULL;
	fSavePanelEncodingMenu = NULL;
		// build lazily
}


void
StyledEditWindow::_LoadAttrs()
{
	entry_ref dir;
	const char* name;
	if (fSaveMessage->FindRef("directory", &dir) != B_OK
		|| fSaveMessage->FindString("name", &name) != B_OK)
		return;

	BPath documentPath(&dir);
	documentPath.Append(name);

	BNode documentNode(documentPath.Path());
	if (documentNode.InitCheck() != B_OK)
		return;

	BRect newFrame;
	ssize_t bytesRead = documentNode.ReadAttr(kInfoAttributeName, B_RECT_TYPE,
		0, &newFrame, sizeof(BRect));
	if (bytesRead != sizeof(BRect))
		return;

	swap_data(B_RECT_TYPE, &newFrame, sizeof(BRect), B_SWAP_BENDIAN_TO_HOST);

	// Check if the frame in on screen, otherwise, ignore it
	BScreen screen(this);
	if (newFrame.Width() > 32 && newFrame.Height() > 32
		&& screen.Frame().Contains(newFrame)) {
		MoveTo(newFrame.left, newFrame.top);
		ResizeTo(newFrame.Width(), newFrame.Height());
	}
}


void
StyledEditWindow::_SaveAttrs()
{
	if (!fSaveMessage)
		return;

	entry_ref dir;
	const char* name;
	if (fSaveMessage->FindRef("directory", &dir) != B_OK
		|| fSaveMessage->FindString("name", &name) != B_OK)
		return;

	BPath documentPath(&dir);
	documentPath.Append(name);

	BNode documentNode(documentPath.Path());
	if (documentNode.InitCheck() != B_OK)
		return;

	BRect frame(Frame());
	swap_data(B_RECT_TYPE, &frame, sizeof(BRect), B_SWAP_HOST_TO_BENDIAN);

	documentNode.WriteAttr(kInfoAttributeName, B_RECT_TYPE, 0, &frame,
		sizeof(BRect));
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LoadAlert"


status_t
StyledEditWindow::_LoadFile(entry_ref* ref)
{
	BEntry entry(ref, true);
		// traverse an eventual link

	status_t status = entry.InitCheck();
	if (status == B_OK && entry.IsDirectory())
		status = B_IS_A_DIRECTORY;

	BFile file;
	if (status == B_OK)
		status = file.SetTo(&entry, B_READ_ONLY);
	if (status == B_OK)
		status = fTextView->GetStyledText(&file);

	if (status == B_ENTRY_NOT_FOUND) {
		// Treat non-existing files consideratley; we just want to get an
		// empty window for them - to create this new document
		status = B_OK;
	}

	if (status != B_OK) {
		// If an error occured, bail out and tell the user what happened
		BEntry entry(ref, true);
		char name[B_FILE_NAME_LENGTH];
		if (entry.GetName(name) != B_OK)
			strlcpy(name, B_TRANSLATE("???"), sizeof(name));

		BString text;
		if (status == B_BAD_TYPE)
			bs_printf(&text,
				B_TRANSLATE("Error loading \"%s\":\n\tUnsupported format"), name);
		else
			bs_printf(&text, B_TRANSLATE("Error loading \"%s\":\n\t%s"),
				name, strerror(status));

		_ShowAlert(text, B_TRANSLATE("OK"), "", "", B_STOP_ALERT);
		return status;
	}

	// update alignment
	switch (fTextView->Alignment()) {
		case B_ALIGN_LEFT:
		default:
			fAlignLeft->SetMarked(true);
			break;
		case B_ALIGN_CENTER:
			fAlignCenter->SetMarked(true);
			break;
		case B_ALIGN_RIGHT:
			fAlignRight->SetMarked(true);
			break;
	}

	// update word wrapping
	fWrapItem->SetMarked(fTextView->DoesWordWrap());
	return B_OK;
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RevertToSavedAlert"


void
StyledEditWindow::_RevertToSaved()
{
	entry_ref ref;
	const char* name;

	fSaveMessage->FindRef("directory", &ref);
	fSaveMessage->FindString("name", &name);

	BDirectory dir(&ref);
	status_t status = dir.InitCheck();
	BEntry entry;
	if (status == B_OK)
		status = entry.SetTo(&dir, name);

	if (status == B_OK)
		status = entry.GetRef(&ref);

	if (status != B_OK || !entry.Exists()) {
		BString alertText;
		bs_printf(&alertText,
			B_TRANSLATE("Cannot revert, file not found: \"%s\"."), name);
		_ShowAlert(alertText, B_TRANSLATE("OK"), "", "", B_STOP_ALERT);
		return;
	}

	BString alertText;
	bs_printf(&alertText,
		B_TRANSLATE("Revert to the last version of \"%s\"? "), Title());
	if (_ShowAlert(alertText, B_TRANSLATE("Cancel"), B_TRANSLATE("OK"),
		"", B_WARNING_ALERT) != 1)
		return;

	fTextView->Reset();
	if (_LoadFile(&ref) != B_OK)
		return;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Menus"

	// clear undo modes
	fUndoItem->SetLabel(B_TRANSLATE("Can't undo"));
	fUndoItem->SetEnabled(false);
	fUndoFlag = false;
	fCanUndo = false;
	fRedoFlag = false;
	fCanRedo = false;

	// clear clean modes
	fSaveItem->SetEnabled(false);
	fRevertItem->SetEnabled(false);
	fUndoCleans = false;
	fRedoCleans = false;
	fClean = true;
}


bool
StyledEditWindow::_Search(BString string, bool caseSensitive, bool wrap,
	bool backSearch, bool scrollToOccurence)
{
	int32 start;
	int32 finish;

	start = B_ERROR;

	int32 length = string.Length();
	if (length == 0)
		return false;

	BString viewText(fTextView->Text());
	int32 textStart, textFinish;
	fTextView->GetSelection(&textStart, &textFinish);
	if (backSearch) {
		if (caseSensitive)
			start = viewText.FindLast(string, textStart);
		else
			start = viewText.IFindLast(string, textStart);
	} else {
		if (caseSensitive)
			start = viewText.FindFirst(string, textFinish);
		else
			start = viewText.IFindFirst(string, textFinish);
	}
	if (start == B_ERROR && wrap) {
		if (backSearch) {
			if (caseSensitive)
				start = viewText.FindLast(string, viewText.Length());
			else
				start = viewText.IFindLast(string, viewText.Length());
		} else {
			if (caseSensitive)
				start = viewText.FindFirst(string, 0);
			else
				start = viewText.IFindFirst(string, 0);
		}
	}

	if (start != B_ERROR) {
		finish = start + length;
		fTextView->Select(start, finish);
		
		if (scrollToOccurence)
			fTextView->ScrollToSelection();
		return true;
	}

	return false;
}


void
StyledEditWindow::_FindSelection()
{
	int32 selectionStart, selectionFinish;
	fTextView->GetSelection(&selectionStart, &selectionFinish);

	int32 selectionLength = selectionFinish- selectionStart;

	BString viewText = fTextView->Text();
	viewText.CopyInto(fStringToFind, selectionStart, selectionLength);
	fFindAgainItem->SetEnabled(true);
	_Search(fStringToFind, fCaseSensitive, fWrapAround, fBackSearch);
}


bool
StyledEditWindow::_Replace(BString findThis, BString replaceWith,
	bool caseSensitive, bool wrap, bool backSearch)
{
	if (_Search(findThis, caseSensitive, wrap, backSearch)) {
		int32 start;
		int32 finish;
		fTextView->GetSelection(&start, &finish);

		_UpdateCleanUndoRedoSaveRevert();
		fTextView->SetSuppressChanges(true);
		fTextView->Delete(start, start + findThis.Length());
		fTextView->Insert(start, replaceWith.String(), replaceWith.Length());
		fTextView->SetSuppressChanges(false);
		fTextView->Select(start, start + replaceWith.Length());
		fTextView->ScrollToSelection();
		return true;
	}

	return false;
}


void
StyledEditWindow::_ReplaceAll(BString findThis, BString replaceWith,
	bool caseSensitive)
{
	bool first = true;
	fTextView->SetSuppressChanges(true);
	
	// start from the beginning of text
	fTextView->Select(0, 0);

	// iterate occurences of findThis without wrapping around
	while (_Search(findThis, caseSensitive, false, false, false)) {
		if (first) {
			_UpdateCleanUndoRedoSaveRevert();
			first = false;
		}
		int32 start;
		int32 finish;
				
		fTextView->GetSelection(&start, &finish);
		fTextView->Delete(start, start + findThis.Length());
		fTextView->Insert(start, replaceWith.String(), replaceWith.Length());
		
		// advance the caret behind the inserted text
		start += replaceWith.Length();
		fTextView->Select(start, start);
	}
	fTextView->ScrollToSelection();
	fTextView->SetSuppressChanges(false);
}


void
StyledEditWindow::_SetFontSize(float fontSize)
{
	uint32 sameProperties;
	BFont font;

	fTextView->GetFontAndColor(&font, &sameProperties);
	font.SetSize(fontSize);
	fTextView->SetFontAndColor(&font, B_FONT_SIZE);

	_UpdateCleanUndoRedoSaveRevert();
}


void
StyledEditWindow::_SetFontColor(const rgb_color* color)
{
	uint32 sameProperties;
	BFont font;

	fTextView->GetFontAndColor(&font, &sameProperties, NULL, NULL);
	fTextView->SetFontAndColor(&font, 0, color);

	_UpdateCleanUndoRedoSaveRevert();
}


void
StyledEditWindow::_SetFontStyle(const char* fontFamily, const char* fontStyle)
{
	BFont font;
	uint32 sameProperties;

	// find out what the old font was
	font_family oldFamily;
	font_style oldStyle;
	fTextView->GetFontAndColor(&font, &sameProperties);
	font.GetFamilyAndStyle(&oldFamily, &oldStyle);

	// clear that family's bit on the menu, if necessary
	if (strcmp(oldFamily, fontFamily)) {
		BMenuItem* oldItem = fFontMenu->FindItem(oldFamily);
		if (oldItem != NULL) {
			oldItem->SetMarked(false);
			BMenu* menu = oldItem->Submenu();
			if (menu != NULL) {
				oldItem = menu->FindItem(oldStyle);
				if (oldItem != NULL)
					oldItem->SetMarked(false);
			}
		}
	}

	font.SetFamilyAndStyle(fontFamily, fontStyle);

	uint16 face = 0;

	if (!(font.Face() & B_REGULAR_FACE))
		face = font.Face();

	if (fBoldItem->IsMarked())
		face |= B_BOLD_FACE;

	if (fItalicItem->IsMarked())
		face |= B_ITALIC_FACE;

	font.SetFace(face);

	fTextView->SetFontAndColor(&font);

	BMenuItem* superItem;
	superItem = fFontMenu->FindItem(fontFamily);
	if (superItem != NULL) {
		superItem->SetMarked(true);
		fCurrentFontItem = superItem;
	}

	_UpdateCleanUndoRedoSaveRevert();
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Statistics"


int32
StyledEditWindow::_ShowStatistics()
{
	size_t words = 0;
	bool inWord = false;
	size_t length = fTextView->TextLength();

	for (size_t i = 0; i < length; i++)	{
		if (BUnicodeChar::IsSpace(fTextView->Text()[i])) {
			inWord = false;
		} else if (!inWord)	{
			words++;
			inWord = true;
		}
	}

	BString result;
	result << B_TRANSLATE("Document statistics") << '\n' << '\n'
		<< B_TRANSLATE("Lines:") << ' ' << fTextView->CountLines() << '\n'
		<< B_TRANSLATE("Characters:") << ' ' << length << '\n'
		<< B_TRANSLATE("Words:") << ' ' << words;

	BAlert* alert = new BAlert("Statistics", result, B_TRANSLATE("OK"), NULL,
		NULL, B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_INFO_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	
	return alert->Go();
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Menus"


void
StyledEditWindow::_UpdateCleanUndoRedoSaveRevert()
{
	fClean = false;
	fUndoCleans = false;
	fRedoCleans = false;
	fRevertItem->SetEnabled(fSaveMessage != NULL);
	fSaveItem->SetEnabled(true);
	fUndoItem->SetLabel(B_TRANSLATE("Can't undo"));
	fUndoItem->SetEnabled(false);
	fCanUndo = false;
	fCanRedo = false;
}


int32
StyledEditWindow::_ShowAlert(const BString& text, const BString& label,
	const BString& label2, const BString& label3, alert_type type) const
{
	const char* button2 = NULL;
	if (label2.Length() > 0)
		button2 = label2.String();

	const char* button3 = NULL;
	button_spacing spacing = B_EVEN_SPACING;
	if (label3.Length() > 0) {
		button3 = label3.String();
		spacing = B_OFFSET_SPACING;
	}

	BAlert* alert = new BAlert("Alert", text.String(), label.String(), button2,
		button3, B_WIDTH_AS_USUAL, spacing, type);
	alert->SetShortcut(0, B_ESCAPE);

	return alert->Go();
}
