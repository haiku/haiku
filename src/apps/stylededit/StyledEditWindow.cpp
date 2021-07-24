/*
 * Copyright 2002-2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 *		Philippe Saint-Pierre
 *		Jonas Sundström
 *		Ryan Leavengood
 *		Vlad Slepukhin
 *		Sarzhuk Zharski
 *		Pascal R. G. Abresch
 */


#include "ColorMenuItem.h"
#include "Constants.h"
#include "FindWindow.h"
#include "ReplaceWindow.h"
#include "StatusView.h"
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
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PrintJob.h>
#include <RecentItems.h>
#include <Rect.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <UnicodeChar.h>
#include <UTF8.h>
#include <Volume.h>


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
	:
	BWindow(frame, "untitled", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS),
	fFindWindow(NULL),
	fReplaceWindow(NULL)
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
	:
	BWindow(frame, "untitled", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS),
	fFindWindow(NULL),
	fReplaceWindow(NULL)
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
	_SwitchNodeMonitor(false);

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

	if (fTextView->TextLength() == 0 && fSaveMessage == NULL)
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
		case MENU_NEW:
			 // this is because of the layout menu change,
			 // it is too early to connect to a different looper there
			 // so either have to redirect it here, or change the invocation
			be_app->PostMessage(message);
			break;
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

		case MENU_RELOAD:
			_ReloadDocument(message);
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
		case B_COPY:
		case B_PASTE:
		case B_SELECT_ALL:
			fTextView->MessageReceived(message);
			break;
		case MENU_CLEAR:
			fTextView->Clear();
			break;
		case MENU_FIND:
		{
			if (fFindWindow == NULL) {
				BRect findWindowFrame(Frame());
				findWindowFrame.InsetBy(
					(findWindowFrame.Width() - 400) / 2,
					(findWindowFrame.Height() - 235) / 2);

				fFindWindow = new FindWindow(findWindowFrame, this,
					&fStringToFind, fCaseSensitive, fWrapAround, fBackSearch);
				fFindWindow->Show();

			} else if (fFindWindow->IsHidden())
				fFindWindow->Show();
			else
				fFindWindow->Activate();
			break;
		}
		case MSG_FIND_WINDOW_QUIT:
		{
			fFindWindow = NULL;
			break;
		}
		case MSG_REPLACE_WINDOW_QUIT:
		{
			fReplaceWindow = NULL;
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
			if (fReplaceWindow == NULL) {
				BRect replaceWindowFrame(Frame());
				replaceWindowFrame.InsetBy(
					(replaceWindowFrame.Width() - 400) / 2,
					(replaceWindowFrame.Height() - 284) / 2);

				fReplaceWindow = new ReplaceWindow(replaceWindowFrame, this,
					&fStringToFind, &fReplaceString, fCaseSensitive,
					fWrapAround, fBackSearch);
				fReplaceWindow->Show();

			} else if (fReplaceWindow->IsHidden())
				fReplaceWindow->Show();
			else
				fReplaceWindow->Activate();
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
			message->FindString("FindText", &fStringToFind);
			message->FindString("ReplaceText", &fReplaceString);

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

		case B_NODE_MONITOR:
			_HandleNodeMonitorEvent(message);
			break;

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
			fUnderlineItem->SetMarked((font.Face() & B_UNDERSCORE_FACE) != 0);

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
			fUnderlineItem->SetMarked((font.Face() & B_UNDERSCORE_FACE) != 0);

			_SetFontStyle(fontFamily, fontStyle);
			break;
		}
		case kMsgSetFontUp:
		{
			uint32 sameProperties;
			BFont font;

			fTextView->GetFontAndColor(&font, &sameProperties);
			//GetFont seems to return a constant size for font.Size(),
			//thus not used here (maybe that is a bug)
			int32 cur_size = (int32)font.Size();

			for (unsigned int a = 0;
				a < sizeof(fontSizes)/sizeof(fontSizes[0]); a++) {
				if (fontSizes[a] > cur_size) {
					_SetFontSize(fontSizes[a]);
					break;
				}
			}
			break;
		}
		case kMsgSetFontDown:
		{
			uint32 sameProperties;
			BFont font;

			fTextView->GetFontAndColor(&font, &sameProperties);
			int32 cur_size = (int32)font.Size();

			for (unsigned int a = 1;
				a < sizeof(fontSizes)/sizeof(fontSizes[0]); a++) {
				if (fontSizes[a] >= cur_size) {
					_SetFontSize(fontSizes[a-1]);
					break;
				}
			}
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
		case kMsgSetUnderline:
		{
			uint32 sameProperties;
			BFont font;
			fTextView->GetFontAndColor(&font, &sameProperties);

			if (fUnderlineItem->IsMarked())
				font.SetFace(B_REGULAR_FACE);
			fUnderlineItem->SetMarked(!fUnderlineItem->IsMarked());

			font_family family;
			font_style style;
			font.GetFamilyAndStyle(&family, &style);

			_SetFontStyle(family, style);
			break;
		}
		case FONT_COLOR:
		{
			ssize_t colorLength;
			rgb_color* color;
			if (message->FindData("color", B_RGB_COLOR_TYPE,
					(const void**)&color, &colorLength) == B_OK
				&& colorLength == sizeof(rgb_color)) {
				/*
				 * TODO: Ideally, when selecting the default color,
				 * you wouldn't naively apply it; it shouldn't lose its nature.
				 * When reloaded with a different default color, it should
				 * reflect that different choice.
				 */
				_SetFontColor(color);
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
			// update wrap setting
			if (fTextView->DoesWordWrap()) {
				fTextView->SetWordWrap(false);
				fWrapItem->SetMarked(false);
			} else {
				fTextView->SetWordWrap(true);
				fWrapItem->SetMarked(true);
			}

			// update buttons
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
				fSaveItem->SetEnabled(fSaveMessage == NULL);
			} else {
				fSaveItem->SetEnabled(true);
			}
			fReloadItem->SetEnabled(fSaveMessage != NULL);
			fEncodingItem->SetEnabled(fSaveMessage != NULL);
			break;

		case SAVE_AS_ENCODING:
			void* ptr;
			if (message->FindPointer("source", &ptr) == B_OK
				&& fSavePanelEncodingMenu != NULL) {
				fTextView->SetEncoding(
					(uint32)fSavePanelEncodingMenu->IndexOf((BMenuItem*)ptr));
			}
			break;

		case UPDATE_STATUS:
		{
			message->AddBool("modified", !fClean);
			bool readOnly = !fTextView->IsEditable();
			message->AddBool("readOnly", readOnly);
			if (readOnly) {
				BVolume volume(fNodeRef.device);
				message->AddBool("canUnlock", !volume.IsReadOnly());
			}
			fStatusView->SetStatus(message);
			break;
		}

		case UPDATE_STATUS_REF:
		{
			entry_ref ref;
			const char* name;

			if (fSaveMessage != NULL
				&& fSaveMessage->FindRef("directory", &ref) == B_OK
				&& fSaveMessage->FindString("name", &name) == B_OK) {

				BDirectory dir(&ref);
				status_t status = dir.InitCheck();
				BEntry entry;
				if (status == B_OK)
					status = entry.SetTo(&dir, name);
				if (status == B_OK)
					status = entry.GetRef(&ref);
			}
			fStatusView->SetRef(ref);
			break;
		}

		case UNLOCK_FILE:
		{
			status_t status = _UnlockFile();
			if (status != B_OK) {
				BString text;
				bs_printf(&text,
					B_TRANSLATE("Unable to unlock file\n\t%s"),
					strerror(status));
				_ShowAlert(text, B_TRANSLATE("OK"), "", "", B_STOP_ALERT);
			}
			PostMessage(UPDATE_STATUS);
			break;
		}

		case UPDATE_LINE_SELECTION:
		{
			int32 line;
			if (message->FindInt32("be:line", &line) == B_OK) {
				fTextView->GoToLine(line);
				fTextView->ScrollToSelection();
			}

			int32 start, length;
			if (message->FindInt32("be:selection_offset", &start) == B_OK) {
				if (message->FindInt32("be:selection_length", &length) != B_OK)
					length = 0;

				fTextView->Select(start, start + length);
				fTextView->ScrollToOffset(start);
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
StyledEditWindow::MenusBeginning()
{
	// update the font menu
	// unselect the old values
	if (fCurrentFontItem != NULL) {
		fCurrentFontItem->SetMarked(false);
		BMenu* menu = fCurrentFontItem->Submenu();
		if (menu != NULL) {
			BMenuItem* item = menu->FindMarked();
			if (item != NULL)
				item->SetMarked(false);
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
	rgb_color color = ui_color(B_DOCUMENT_TEXT_COLOR);
	bool sameColor;
	fTextView->GetFontAndColor(&font, &sameProperties, &color, &sameColor);
	color.alpha = 255;

	if (sameColor) {
		if (fDefaultFontColorItem->Color() == color)
			fDefaultFontColorItem->SetMarked(true);
		else {
			for (int i = 0; i < fFontColorMenu->CountItems(); i++) {
				ColorMenuItem* item = dynamic_cast<ColorMenuItem*>
					(fFontColorMenu->ItemAt(i));
				if (item != NULL && item->Color() == color) {
					item->SetMarked(true);
					break;
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
	fUnderlineItem->SetMarked((font.Face() & B_UNDERSCORE_FACE) != 0);

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

	// text encoding
	const BCharacterSet* charset
		= BCharacterSetRoster::GetCharacterSetByFontID(fTextView->GetEncoding());
	BMenu* encodingMenu = fEncodingItem->Submenu();
	if (charset != NULL && encodingMenu != NULL) {
		const char* mime = charset->GetMIMEName();
		BString name(charset->GetPrintName());
		if (mime)
			name << " (" << mime << ")";

		BMenuItem* item = encodingMenu->FindItem(name);
		if (item != NULL)
			item->SetMarked(true);
	}
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SaveAlert"


status_t
StyledEditWindow::Save(BMessage* message)
{
	_NodeMonitorSuspender nodeMonitorSuspender(this);

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
					"read-only. Save changes to the document \"%s\"? "), name);
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

	// clear clean modes
	fSaveItem->SetEnabled(false);
	fUndoCleans = false;
	fRedoCleans = false;
	fClean = true;
	fNagOnNodeChange = true;

	PostMessage(UPDATE_STATUS);
	PostMessage(UPDATE_STATUS_REF);

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

	fSaveMessage = new(std::nothrow) BMessage(B_SAVE_REQUESTED);
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

	_SwitchNodeMonitor(true, ref);

	PostMessage(UPDATE_STATUS_REF);

	fReloadItem->SetEnabled(fSaveMessage != NULL);
	fEncodingItem->SetEnabled(fSaveMessage != NULL);
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
			while (currentHeight < printableRect.Height()
				&& printLine <= lastLine)
			{
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

	fNagOnNodeChange = true;


	// add textview and scrollview

	BRect viewFrame = Bounds();
	BRect textBounds = viewFrame;
	textBounds.OffsetTo(B_ORIGIN);

	fTextView = new StyledEditView(viewFrame, textBounds, this);
	fTextView->SetInsets(TEXT_INSET, TEXT_INSET, TEXT_INSET, TEXT_INSET);
	fTextView->SetDoesUndo(true);
	fTextView->SetStylable(true);
	fTextView->SetEncoding(encoding);

	fScrollView = new BScrollView("scrollview", fTextView, B_FOLLOW_ALL, 0,
		true, true, B_PLAIN_BORDER);
	fTextView->MakeFocus(true);

	fStatusView = new StatusView(fScrollView);
	fScrollView->AddChild(fStatusView);

	BMenuItem* openItem = new BMenuItem(BRecentFilesList::NewFileListMenu(
		B_TRANSLATE("Open" B_UTF8_ELLIPSIS), NULL, NULL, be_app, 9, true,
		NULL, APP_SIGNATURE), new BMessage(MENU_OPEN));
	openItem->SetShortcut('O', 0);
	openItem->SetTarget(be_app);

	fSaveItem = new BMenuItem(B_TRANSLATE("Save"),new BMessage(MENU_SAVE), 'S');
	fSaveItem->SetEnabled(false);

	fReloadItem = new BMenuItem(B_TRANSLATE("Reload" B_UTF8_ELLIPSIS),
	new BMessage(MENU_RELOAD), 'L');
	fReloadItem->SetEnabled(false);

	fUndoItem = new BMenuItem(B_TRANSLATE("Can't undo"),
		new BMessage(B_UNDO), 'Z');
	fUndoItem->SetEnabled(false);

	fCutItem = new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X');
	fCutItem->SetEnabled(false);

	fCopyItem = new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C');
	fCopyItem->SetEnabled(false);

	fFindAgainItem = new BMenuItem(B_TRANSLATE("Find again"),
		new BMessage(MENU_FIND_AGAIN), 'G');
	fFindAgainItem->SetEnabled(false);

	fReplaceItem = new BMenuItem(B_TRANSLATE("Replace" B_UTF8_ELLIPSIS),
		new BMessage(MENU_REPLACE), 'R');

	fReplaceSameItem = new BMenuItem(B_TRANSLATE("Replace next"),
		new BMessage(MENU_REPLACE_SAME), 'T');
	fReplaceSameItem->SetEnabled(false);

	fFontSizeMenu = new BMenu(B_TRANSLATE("Size"));
	fFontSizeMenu->SetRadioMode(true);

	BMenuItem* menuItem;
	for (uint32 i = 0; i < sizeof(fontSizes) / sizeof(fontSizes[0]); i++) {
		BMessage* fontMessage = new BMessage(FONT_SIZE);
		fontMessage->AddFloat("size", fontSizes[i]);

		char label[64];
		snprintf(label, sizeof(label), "%" B_PRId32, fontSizes[i]);
		fFontSizeMenu->AddItem(menuItem = new BMenuItem(label, fontMessage));

		if (fontSizes[i] == (int32)be_plain_font->Size())
			menuItem->SetMarked(true);
	}

	fFontColorMenu = new BMenu(B_TRANSLATE("Color"), 0, 0);
	fFontColorMenu->SetRadioMode(true);

	_BuildFontColorMenu(fFontColorMenu);

	fBoldItem = new BMenuItem(B_TRANSLATE("Bold"),
		new BMessage(kMsgSetBold));
	fBoldItem->SetShortcut('B', 0);

	fItalicItem = new BMenuItem(B_TRANSLATE("Italic"),
		new BMessage(kMsgSetItalic));
	fItalicItem->SetShortcut('I', 0);

	fUnderlineItem = new BMenuItem(B_TRANSLATE("Underline"),
		new BMessage(kMsgSetUnderline));
	fUnderlineItem->SetShortcut('U', 0);

	fFontMenu = new BMenu(B_TRANSLATE("Font"));
	fCurrentFontItem = 0;
	fCurrentStyleItem = 0;

	// premake font menu since we cant add members dynamically later
	BLayoutBuilder::Menu<>(fFontMenu)
		.AddItem(fFontSizeMenu)
		.AddItem(fFontColorMenu)
		.AddSeparator()
		.AddItem(B_TRANSLATE("Increase size"), kMsgSetFontUp, '+')
		.AddItem(B_TRANSLATE("Decrease size"), kMsgSetFontDown, '-')
		.AddItem(fBoldItem)
		.AddItem(fItalicItem)
		.AddItem(fUnderlineItem)
		.AddSeparator()
	.End();

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

	// "Align"-subMenu:
	BMenu* alignMenu = new BMenu(B_TRANSLATE("Align"));
	alignMenu->SetRadioMode(true);

	alignMenu->AddItem(fAlignLeft = new BMenuItem(B_TRANSLATE("Left"),
		new BMessage(ALIGN_LEFT)));
	fAlignLeft->SetMarked(true);
	fAlignLeft->SetShortcut('L', B_OPTION_KEY);

	alignMenu->AddItem(fAlignCenter = new BMenuItem(B_TRANSLATE("Center"),
		new BMessage(ALIGN_CENTER)));
	fAlignCenter->SetShortcut('C', B_OPTION_KEY);

	alignMenu->AddItem(fAlignRight = new BMenuItem(B_TRANSLATE("Right"),
		new BMessage(ALIGN_RIGHT)));
	fAlignRight->SetShortcut('R', B_OPTION_KEY);

	fWrapItem = new BMenuItem(B_TRANSLATE("Wrap lines"),
		new BMessage(WRAP_LINES));
	fWrapItem->SetMarked(true);
	fWrapItem->SetShortcut('W', B_OPTION_KEY);

	BMessage *message = new BMessage(MENU_RELOAD);
	message->AddString("encoding", "auto");
	fEncodingItem = new BMenuItem(_PopulateEncodingMenu(
		new BMenu(B_TRANSLATE("Text encoding")), "UTF-8"),
		message);
	fEncodingItem->SetEnabled(false);

	BMenuBar* mainMenu = new BMenuBar("mainMenu");

	BLayoutBuilder::Menu<>(mainMenu)
		.AddMenu(B_TRANSLATE("File"))
			.AddItem(B_TRANSLATE("New"), MENU_NEW, 'N')
			.AddItem(openItem)
			.AddSeparator()
			.AddItem(fSaveItem)
			.AddItem(B_TRANSLATE("Save as" B_UTF8_ELLIPSIS),
				MENU_SAVEAS, 'S', B_SHIFT_KEY)
			.AddItem(fReloadItem)
			.AddItem(B_TRANSLATE("Close"), MENU_CLOSE, 'W')
			.AddSeparator()
			.AddItem(B_TRANSLATE("Page setup" B_UTF8_ELLIPSIS), MENU_PAGESETUP)
			.AddItem(B_TRANSLATE("Print" B_UTF8_ELLIPSIS), MENU_PRINT, 'P')
			.AddSeparator()
			.AddItem(B_TRANSLATE("Quit"), MENU_QUIT, 'Q')
		.End()
		.AddMenu(B_TRANSLATE("Edit"))
			.AddItem(fUndoItem)
			.AddSeparator()
			.AddItem(fCutItem)
			.AddItem(fCopyItem)
			.AddItem(B_TRANSLATE("Paste"), B_PASTE, 'V')
			.AddSeparator()
			.AddItem(B_TRANSLATE("Select all"), B_SELECT_ALL, 'A')
			.AddSeparator()
			.AddItem(B_TRANSLATE("Find" B_UTF8_ELLIPSIS), MENU_FIND, 'F')
			.AddItem(fFindAgainItem)
			.AddItem(B_TRANSLATE("Find selection"), MENU_FIND_SELECTION, 'H')
			.AddItem(fReplaceItem)
			.AddItem(fReplaceSameItem)
		.End()
		.AddItem(fFontMenu)
		.AddMenu(B_TRANSLATE("Document"))
			.AddItem(alignMenu)
			.AddItem(fWrapItem)
			.AddItem(fEncodingItem)
			.AddSeparator()
			.AddItem(B_TRANSLATE("Statistics" B_UTF8_ELLIPSIS), SHOW_STATISTICS)
		.End();


	fSavePanel = NULL;
	fSavePanelEncodingMenu = NULL;

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(mainMenu)
		.AddGroup(B_VERTICAL, 0)
			.SetInsets(-1)
			.Add(fScrollView)
		.End()
	.End();

	SetKeyMenuBar(mainMenu);

}


void
StyledEditWindow::_BuildFontColorMenu(BMenu* menu)
{
	if (menu == NULL)
		return;

	BFont font;
	menu->GetFont(&font);
	font_height fh;
	font.GetHeight(&fh);

	const float itemHeight = ceilf(fh.ascent + fh.descent + 2 * fh.leading);
	const float margin = 8.0;
	const int nbColumns = 5;

	BMessage msgTemplate(FONT_COLOR);
	BRect matrixArea(0, 0, 0, 0);

	// we place the color palette, reserving room at the top
	for (uint i = 0; i < sizeof(palette) / sizeof(rgb_color); i++) {
		BPoint topLeft((i % nbColumns) * (itemHeight + margin),
			(i / nbColumns) * (itemHeight + margin));
		BRect buttonArea(topLeft.x, topLeft.y, topLeft.x + itemHeight,
			topLeft.y + itemHeight);
		buttonArea.OffsetBy(margin, itemHeight + margin + margin);
		menu->AddItem(
			new ColorMenuItem("", palette[i], new BMessage(msgTemplate)),
			buttonArea);
		buttonArea.OffsetBy(margin, margin);
		matrixArea = matrixArea | buttonArea;
	}

	// separator at the bottom to add spacing in the matrix menu
	matrixArea.top = matrixArea.bottom;
	menu->AddItem(new BSeparatorItem(), matrixArea);

	matrixArea.top = 0;
	matrixArea.bottom = itemHeight + 4;

	BMessage* msg = new BMessage(msgTemplate);
	msg->AddBool("default", true);
	fDefaultFontColorItem = new ColorMenuItem(B_TRANSLATE("Default"),
		ui_color(B_DOCUMENT_TEXT_COLOR), msg);
	menu->AddItem(fDefaultFontColorItem, matrixArea);

	matrixArea.top = matrixArea.bottom;
	matrixArea.bottom = matrixArea.top + margin;
	menu->AddItem(new BSeparatorItem(), matrixArea);
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

	// info about position of caret may live in the file attributes
	int32 position = 0;
	if (documentNode.ReadAttr("be:caret_position", B_INT32_TYPE, 0,
			&position, sizeof(position)) != sizeof(position))
		position = 0;

	fTextView->Select(position, position);
	fTextView->ScrollToOffset(position);

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

	// preserve caret line and position
	int32 start, end;
	fTextView->GetSelection(&start, &end);
	documentNode.WriteAttr("be:caret_position",
			B_INT32_TYPE, 0, &start, sizeof(start));
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LoadAlert"


status_t
StyledEditWindow::_LoadFile(entry_ref* ref, const char* forceEncoding)
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
		status = fTextView->GetStyledText(&file, forceEncoding);

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

	struct stat st;
	if (file.InitCheck() == B_OK && file.GetStat(&st) == B_OK) {
		bool editable = (getuid() == st.st_uid && S_IWUSR & st.st_mode)
					|| (getgid() == st.st_gid && S_IWGRP & st.st_mode)
					|| (S_IWOTH & st.st_mode);
		BVolume volume(ref->device);
		editable = editable && !volume.IsReadOnly();
		_SetReadOnly(!editable);
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
StyledEditWindow::_ReloadDocument(BMessage* message)
{
	entry_ref ref;
	const char* name;

	if (fSaveMessage == NULL || message == NULL
		|| fSaveMessage->FindRef("directory", &ref) != B_OK
		|| fSaveMessage->FindString("name", &name) != B_OK)
		return;

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

	if (!fClean) {
		BString alertText;
		bs_printf(&alertText,
			B_TRANSLATE("\"%s\" has unsaved changes.\n"
				"Revert it to the last saved version? "), Title());
		if (_ShowAlert(alertText, B_TRANSLATE("Cancel"), B_TRANSLATE("Revert"),
			"", B_WARNING_ALERT) != 1)
			return;
	}

	const BCharacterSet* charset
		= BCharacterSetRoster::GetCharacterSetByFontID(
			fTextView->GetEncoding());
	const char* forceEncoding = NULL;
	if (message->FindString("encoding", &forceEncoding) != B_OK) {
		if (charset != NULL)
			forceEncoding = charset->GetName();
	} else {
		if (charset != NULL) {
			// UTF8 id assumed equal to -1
			const uint32 idUTF8 = (uint32)-1;
			uint32 id = charset->GetConversionID();
			if (strcmp(forceEncoding, "next") == 0)
				id = id == B_MS_WINDOWS_1250_CONVERSION	? idUTF8 : id + 1;
			else if (strcmp(forceEncoding, "previous") == 0)
				id = id == idUTF8 ? B_MS_WINDOWS_1250_CONVERSION : id - 1;
			const BCharacterSet* newCharset
				= BCharacterSetRoster::GetCharacterSetByConversionID(id);
			if (newCharset != NULL)
				forceEncoding = newCharset->GetName();
		}
	}

	BScrollBar* vertBar = fScrollView->ScrollBar(B_VERTICAL);
	float vertPos = vertBar != NULL ? vertBar->Value() : 0.f;

	DisableUpdates();

	fTextView->Reset();

	status = _LoadFile(&ref, forceEncoding);

	if (vertBar != NULL)
		vertBar->SetValue(vertPos);

	EnableUpdates();

	if (status != B_OK)
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

	fUndoCleans = false;
	fRedoCleans = false;
	fClean = true;

	fNagOnNodeChange = true;
}


status_t
StyledEditWindow::_UnlockFile()
{
	_NodeMonitorSuspender nodeMonitorSuspender(this);

	if (!fSaveMessage)
		return B_ERROR;

	entry_ref dirRef;
	const char* name;
	if (fSaveMessage->FindRef("directory", &dirRef) != B_OK
		|| fSaveMessage->FindString("name", &name) != B_OK)
		return B_BAD_VALUE;

	BDirectory dir(&dirRef);
	BEntry entry(&dir, name);

	status_t status = dir.InitCheck();
	if (status != B_OK)
		return status;

	status = entry.InitCheck();
	if (status != B_OK)
		return status;

	struct stat st;
	BFile file(&entry, B_READ_WRITE);
	status = file.InitCheck();
	if (status != B_OK)
		return status;

	status = file.GetStat(&st);
	if (status != B_OK)
		return status;

	st.st_mode |= S_IWUSR;
	status = file.SetPermissions(st.st_mode);
	if (status == B_OK)
		_SetReadOnly(false);

	return status;
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

	if (fUnderlineItem->IsMarked())
		face |= B_UNDERSCORE_FACE;

	font.SetFace(face);

	fTextView->SetFontAndColor(&font, B_FONT_FAMILY_AND_STYLE | B_FONT_FACE);

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
		if (BUnicodeChar::IsWhitespace(fTextView->Text()[i])) {
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


void
StyledEditWindow::_SetReadOnly(bool readOnly)
{
	fReplaceItem->SetEnabled(!readOnly);
	fReplaceSameItem->SetEnabled(!readOnly);
	fFontMenu->SetEnabled(!readOnly);
	fAlignLeft->Menu()->SetEnabled(!readOnly);
	fWrapItem->SetEnabled(!readOnly);
	fTextView->MakeEditable(!readOnly);
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Menus"


void
StyledEditWindow::_UpdateCleanUndoRedoSaveRevert()
{
	fClean = false;
	fUndoCleans = false;
	fRedoCleans = false;
	fReloadItem->SetEnabled(fSaveMessage != NULL);
	fEncodingItem->SetEnabled(fSaveMessage != NULL);
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


BMenu*
StyledEditWindow::_PopulateEncodingMenu(BMenu* menu, const char* currentEncoding)
{
	menu->SetRadioMode(true);
	BString encoding(currentEncoding);
	if (encoding.Length() == 0)
		encoding.SetTo("UTF-8");

	BCharacterSetRoster roster;
	BCharacterSet charset;
	while (roster.GetNextCharacterSet(&charset) == B_OK) {
		const char* mime = charset.GetMIMEName();
		BString name(charset.GetPrintName());

		if (mime)
			name << " (" << mime << ")";

		BMessage *message = new BMessage(MENU_RELOAD);
		if (message != NULL) {
			message->AddString("encoding", charset.GetName());
			BMenuItem* item = new BMenuItem(name, message);
			if (encoding.Compare(charset.GetName()) == 0)
				item->SetMarked(true);
			menu->AddItem(item);
		}
	}

	menu->AddSeparatorItem();
	BMessage *message = new BMessage(MENU_RELOAD);
	message->AddString("encoding", "auto");
	menu->AddItem(new BMenuItem(B_TRANSLATE("Autodetect"), message));

	message = new BMessage(MENU_RELOAD);
	message->AddString("encoding", "next");
	AddShortcut(B_PAGE_DOWN, B_OPTION_KEY, message);
	message = new BMessage(MENU_RELOAD);
	message->AddString("encoding", "previous");
	AddShortcut(B_PAGE_UP, B_OPTION_KEY, message);

	return menu;
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NodeMonitorAlerts"


void
StyledEditWindow::_ShowNodeChangeAlert(const char* name, bool removed)
{
	if (!fNagOnNodeChange)
		return;

	BString alertText(removed ? B_TRANSLATE("File \"%file%\" was removed by "
		"another application, recover it?")
		: B_TRANSLATE("File \"%file%\" was modified by "
		"another application, reload it?"));
	alertText.ReplaceAll("%file%", name);

	if (_ShowAlert(alertText, removed ? B_TRANSLATE("Recover")
			: B_TRANSLATE("Reload"), B_TRANSLATE("Ignore"), "",
			B_WARNING_ALERT) == 0)
	{
		if (!removed) {
			// supress the warning - user has already agreed
			fClean = true;
			BMessage msg(MENU_RELOAD);
			_ReloadDocument(&msg);
		} else
			Save();
	} else
		fNagOnNodeChange = false;

	fSaveItem->SetEnabled(!fClean);
}


void
StyledEditWindow::_HandleNodeMonitorEvent(BMessage *message)
{
	int32 opcode = 0;
	if (message->FindInt32("opcode", &opcode) != B_OK)
		return;

	if (opcode != B_ENTRY_CREATED
		&& message->FindInt64("node") != fNodeRef.node)
		// bypass foreign nodes' event
		return;

	switch (opcode) {
		case B_STAT_CHANGED:
			{
				int32 fields = 0;
				if (message->FindInt32("fields", &fields) == B_OK
					&& (fields & (B_STAT_SIZE | B_STAT_MODIFICATION_TIME
							| B_STAT_MODE)) == 0)
					break;

				const char* name = NULL;
				if (fSaveMessage->FindString("name", &name) != B_OK)
					break;

				_ShowNodeChangeAlert(name, false);
			}
			break;

		case B_ENTRY_MOVED:
			{
				int32 device = 0;
				int64 srcFolder = 0;
				int64 dstFolder = 0;
				const char* name = NULL;
				if (message->FindInt32("device", &device) != B_OK
					|| message->FindInt64("to directory", &dstFolder) != B_OK
					|| message->FindInt64("from directory", &srcFolder) != B_OK
					|| message->FindString("name", &name) != B_OK)
						break;

				entry_ref newRef(device, dstFolder, name);
				BEntry entry(&newRef);

				BEntry dirEntry;
				entry.GetParent(&dirEntry);

				entry_ref ref;
				dirEntry.GetRef(&ref);
				fSaveMessage->ReplaceRef("directory", &ref);
				fSaveMessage->ReplaceString("name", name);

				// store previous name - it may be useful in case
				// we have just moved to temporary copy of file (vim case)
				const char* sourceName = NULL;
				if (message->FindString("from name", &sourceName) == B_OK) {
					fSaveMessage->RemoveName("org.name");
					fSaveMessage->AddString("org.name", sourceName);
					fSaveMessage->RemoveName("move time");
					fSaveMessage->AddInt64("move time", system_time());
				}

				SetTitle(name);

				if (srcFolder != dstFolder) {
					_SwitchNodeMonitor(false);
					_SwitchNodeMonitor(true);
				}
				PostMessage(UPDATE_STATUS_REF);
			}
			break;

		case B_ENTRY_REMOVED:
			{
				_SwitchNodeMonitor(false);

				fClean = false;

				// some editors like vim save files in following way:
				// 1) move t.txt -> t.txt~
				// 2) re-create t.txt and write data to it
				// 3) remove t.txt~
				// go to catch this case
				int32 device = 0;
				int64 directory = 0;
				BString orgName;
				if (fSaveMessage->FindString("org.name", &orgName) == B_OK
					&& message->FindInt32("device", &device) == B_OK
					&& message->FindInt64("directory", &directory) == B_OK)
				{
					// reuse the source name if it is not too old
					bigtime_t time = fSaveMessage->FindInt64("move time");
					if ((system_time() - time) < 1000000) {
						entry_ref ref(device, directory, orgName);
						BEntry entry(&ref);
						if (entry.InitCheck() == B_OK) {
							_SwitchNodeMonitor(true, &ref);
						}

						fSaveMessage->ReplaceString("name", orgName);
						fSaveMessage->RemoveName("org.name");
						fSaveMessage->RemoveName("move time");

						SetTitle(orgName);
						_ShowNodeChangeAlert(orgName, false);
						break;
					}
				}

				const char* name = NULL;
				if (message->FindString("name", &name) != B_OK
					&& fSaveMessage->FindString("name", &name) != B_OK)
					name = "Unknown";

				_ShowNodeChangeAlert(name, true);
				PostMessage(UPDATE_STATUS_REF);
			}
			break;

		default:
			break;
	}
}


void
StyledEditWindow::_SwitchNodeMonitor(bool on, entry_ref* ref)
{
	if (!on) {
		watch_node(&fNodeRef, B_STOP_WATCHING, this);
		watch_node(&fFolderNodeRef, B_STOP_WATCHING, this);
		fNodeRef = node_ref();
		fFolderNodeRef = node_ref();
		return;
	}

	BEntry entry, folderEntry;

	if (ref != NULL) {
		entry.SetTo(ref, true);
		entry.GetParent(&folderEntry);

	} else if (fSaveMessage != NULL) {
		entry_ref ref;
		const char* name = NULL;
		if (fSaveMessage->FindRef("directory", &ref) != B_OK
			|| fSaveMessage->FindString("name", &name) != B_OK)
			return;

		BDirectory dir(&ref);
		entry.SetTo(&dir, name);
		folderEntry.SetTo(&ref);

	} else
		return;

	if (entry.InitCheck() != B_OK || folderEntry.InitCheck() != B_OK)
		return;

	entry.GetNodeRef(&fNodeRef);
	folderEntry.GetNodeRef(&fFolderNodeRef);

	watch_node(&fNodeRef, B_WATCH_STAT, this);
	watch_node(&fFolderNodeRef, B_WATCH_DIRECTORY, this);
}
