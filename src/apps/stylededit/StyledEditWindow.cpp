/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
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
#include <Debug.h>
#include <Clipboard.h>
#include <File.h>
#include <Menu.h>
#include <MenuItem.h>
#include <PrintJob.h>
#include <Roster.h>
#include <ScrollView.h>
#include <String.h>
#include <TextControl.h>
#include <TranslationUtils.h>
#include <Window.h>

#include <CharacterSet.h>
#include <CharacterSetRoster.h>

#include <stdlib.h>

using namespace BPrivate;


StyledEditWindow::StyledEditWindow(BRect frame, int32 id, uint32 encoding)
	: BWindow(frame, "untitled", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	InitWindow(encoding);
	BString unTitled;
	unTitled.SetTo("Untitled ");
	unTitled << id;
	SetTitle(unTitled.String());
	fSaveItem->SetEnabled(true);
		// allow saving empty files
	Show();
}


StyledEditWindow::StyledEditWindow(BRect frame, entry_ref *ref, uint32 encoding)
	: BWindow(frame, "untitled", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	InitWindow(encoding);
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
StyledEditWindow::InitWindow(uint32 encoding)
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
	fCaseSens = false;
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

	BMenu* menu = new BMenu("File");
	fMenuBar->AddItem(menu);

	BMenuItem* menuItem;
	menu->AddItem(menuItem = new BMenuItem("New", new BMessage(MENU_NEW), 'N'));
	menuItem->SetTarget(be_app);
	
	menu->AddItem(menuItem = new BMenuItem(fRecentMenu = new BMenu("Open..."),
		new BMessage(MENU_OPEN)));
	menuItem->SetShortcut('O', 0);
	menuItem->SetTarget(be_app);
	menu->AddSeparatorItem();

	menu->AddItem(fSaveItem = new BMenuItem("Save", new BMessage(MENU_SAVE), 'S')); 
	fSaveItem->SetEnabled(false); 
	menu->AddItem(menuItem = new BMenuItem("Save As...", new BMessage(MENU_SAVEAS)));
	menuItem->SetShortcut('S',B_SHIFT_KEY);
	menuItem->SetEnabled(true);

	menu->AddItem(fRevertItem = new BMenuItem("Revert to Saved...",
		new BMessage(MENU_REVERT))); 
	fRevertItem->SetEnabled(false);
	menu->AddItem(menuItem = new BMenuItem("Close", new BMessage(MENU_CLOSE), 'W'));

	menu->AddSeparatorItem();
	menu->AddItem(menuItem = new BMenuItem("Page Setup...", new BMessage(MENU_PAGESETUP)));
	menu->AddItem(menuItem = new BMenuItem("Print...", new BMessage(MENU_PRINT), 'P'));

	menu->AddSeparatorItem();
	menu->AddItem(menuItem = new BMenuItem("Quit", new BMessage(MENU_QUIT), 'Q'));

	// Add the "Edit"-menu:
	menu = new BMenu("Edit");
	fMenuBar->AddItem(menu);

	menu->AddItem(fUndoItem = new BMenuItem("Can't Undo", new BMessage(B_UNDO), 'Z'));
	fUndoItem->SetEnabled(false);

	menu->AddSeparatorItem();
	menu->AddItem(fCutItem = new BMenuItem("Cut", new BMessage(B_CUT), 'X'));
	fCutItem->SetEnabled(false);
	fCutItem->SetTarget(fTextView);

	menu->AddItem(fCopyItem = new BMenuItem("Copy", new BMessage(B_COPY), 'C'));
	fCopyItem->SetEnabled(false);
	fCopyItem->SetTarget(fTextView);

	menu->AddItem(menuItem = new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));
	menuItem->SetTarget(fTextView);
	menu->AddItem(fClearItem = new BMenuItem("Clear", new BMessage(MENU_CLEAR)));
	fClearItem->SetEnabled(false); 
	fClearItem->SetTarget(fTextView);

	menu->AddSeparatorItem();
	menu->AddItem(menuItem = new BMenuItem("Select All", new BMessage(B_SELECT_ALL), 'A'));
	menuItem->SetTarget(fTextView);

	menu->AddSeparatorItem();
	menu->AddItem(menuItem = new BMenuItem("Find...", new BMessage(MENU_FIND),'F'));
	menu->AddItem(fFindAgainItem= new BMenuItem("Find Again",new BMessage(MENU_FIND_AGAIN),'G'));
	fFindAgainItem->SetEnabled(false);

	menu->AddItem(menuItem = new BMenuItem("Find Selection", new BMessage(MENU_FIND_SELECTION),'H'));
	menu->AddItem(menuItem = new BMenuItem("Replace...", new BMessage(MENU_REPLACE),'R'));
	menu->AddItem(fReplaceSameItem = new BMenuItem("Replace Same", new BMessage(MENU_REPLACE_SAME),'T'));
	fReplaceSameItem->SetEnabled(false);

	// Add the "Font"-menu:
	fFontMenu = new BMenu("Font");
	fMenuBar->AddItem(fFontMenu);

	//"Size"-subMenu
	fFontSizeMenu = new BMenu("Size");
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
	fFontColorMenu = new BMenu("Color");
	fFontColorMenu->SetRadioMode(true);
	fFontMenu->AddItem(fFontColorMenu);
	
	fFontColorMenu->AddItem(fBlackItem = new BMenuItem("Black", new BMessage(FONT_COLOR)));
	fBlackItem->SetMarked(true); 
	fFontColorMenu->AddItem(fRedItem = new ColorMenuItem("Red", RED, new BMessage(FONT_COLOR)));
	fFontColorMenu->AddItem(fGreenItem = new ColorMenuItem("Green", GREEN, new BMessage(FONT_COLOR)));
	fFontColorMenu->AddItem(fBlueItem = new ColorMenuItem("Blue", BLUE, new BMessage(FONT_COLOR)));
	fFontColorMenu->AddItem(fCyanItem = new ColorMenuItem("Cyan", CYAN, new BMessage(FONT_COLOR)));
	fFontColorMenu->AddItem(fMagentaItem = new ColorMenuItem("Magenta", MAGENTA, new BMessage(FONT_COLOR)));
	fFontColorMenu->AddItem(fYellowItem = new ColorMenuItem("Yellow", YELLOW, new BMessage(FONT_COLOR)));
	fFontMenu->AddSeparatorItem();

	// Available fonts, mark "be_plain_font" item

	font_family plainFamily;
	font_style plainStyle;
	be_plain_font->GetFamilyAndStyle(&plainFamily, &plainStyle);
	fCurrentFontItem = 0;

	BMenu* subMenu;
	int32 numFamilies = count_font_families();
	for (int32 i = 0; i < numFamilies; i++) {
		font_family family;
		if (get_font_family(i, &family) == B_OK) {
			subMenu = new BMenu(family);
			subMenu->SetRadioMode(true);
			fFontMenu->AddItem(menuItem = new BMenuItem(subMenu, new BMessage(FONT_FAMILY)));

			if (!strcmp(plainFamily, family)) {
				menuItem->SetMarked(true);
				fCurrentFontItem = menuItem;
			}

			int32 numStyles = count_font_styles(family);
			for (int32 j = 0; j < numStyles; j++) {
				font_style style;
				uint32 flags;
				if (get_font_style(family, j, &style, &flags) == B_OK) {
					subMenu->AddItem(menuItem = new BMenuItem(style,
						new BMessage(FONT_STYLE)));

					if (!strcmp(plainStyle, style))
						menuItem->SetMarked(true);
				}
			}
		}
	}

	// Add the "Document"-menu:
	menu = new BMenu("Document");
	fMenuBar->AddItem(menu);

	// "Align"-subMenu:
	subMenu = new BMenu("Align");
	subMenu->SetRadioMode(true); 

	subMenu->AddItem(fAlignLeft = new BMenuItem("Left", new BMessage(ALIGN_LEFT)));
	menuItem->SetMarked(true); 

	subMenu->AddItem(fAlignCenter = new BMenuItem("Center", new BMessage(ALIGN_CENTER)));
	subMenu->AddItem(fAlignRight = new BMenuItem("Right", new BMessage(ALIGN_RIGHT)));
	menu->AddItem(subMenu);
	menu->AddItem(fWrapItem = new BMenuItem("Wrap Lines", new BMessage(WRAP_LINES)));
	fWrapItem->SetMarked(true);

	fSavePanel = NULL;
	fSavePanelEncodingMenu = NULL;
		// build lazily
}


void
StyledEditWindow::MessageReceived(BMessage *message)
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
			RevertToSaved();
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
				&fStringToFind, &fCaseSens, &fWrapAround, &fBackSearch);
			window->Show();
			break;
		}
		case MSG_SEARCH:
			message->FindString("findtext", &fStringToFind);
			fFindAgainItem->SetEnabled(true);
			message->FindBool("casesens", &fCaseSens);
			message->FindBool("wrap", &fWrapAround);
			message->FindBool("backsearch", &fBackSearch);

			Search(fStringToFind, fCaseSens, fWrapAround, fBackSearch);
			break;
		case MENU_FIND_AGAIN:
			Search(fStringToFind, fCaseSens, fWrapAround, fBackSearch);
			break;
		case MENU_FIND_SELECTION:
			FindSelection();
			break;
		case MENU_REPLACE:
		{
			BRect replaceWindowFrame(100, 100, 400, 284);
			BWindow* window = new ReplaceWindow(replaceWindowFrame, this,
				&fStringToFind, &fReplaceString, &fCaseSens, &fWrapAround, &fBackSearch);
			window->Show();
			break;
		}
		case MSG_REPLACE:
		{
			BString findIt; 
			BString replaceWith;
			bool caseSens, wrap, backSearch;

			message->FindBool("casesens", &caseSens);
			message->FindBool("wrap", &wrap);
			message->FindBool("backsearch", &backSearch);

			message->FindString("FindText", &findIt);
			message->FindString("ReplaceText", &replaceWith);
			fStringToFind = findIt;
			fFindAgainItem->SetEnabled(true);
			fReplaceString = replaceWith;
			fReplaceSameItem->SetEnabled(true);
			fCaseSens = caseSens;
			fWrapAround = wrap;
			fBackSearch = backSearch;

			Replace(findIt, replaceWith, caseSens, wrap, backSearch);
			break;
		}
		case MENU_REPLACE_SAME:
			Replace(fStringToFind,fReplaceString,fCaseSens,fWrapAround,fBackSearch);
			break;

		case MSG_REPLACE_ALL:
		{
			BString findIt; 
			BString replaceWith;
			bool caseSens, allWindows;

			message->FindBool("casesens", &caseSens);
			message->FindString("FindText",&findIt);
			message->FindString("ReplaceText",&replaceWith);				
			message->FindBool("allwindows", &allWindows);

			fStringToFind = findIt;
			fFindAgainItem->SetEnabled(true);
			fReplaceString = replaceWith;
			fReplaceSameItem->SetEnabled(true);
			fCaseSens = caseSens;

			if (allWindows)
				SearchAllWindows(findIt, replaceWith, caseSens);
			else	
				ReplaceAll(findIt, replaceWith,caseSens);
			break;	
		}

		// Font menu

		case FONT_SIZE:
		{
			float fontSize;
			if (message->FindFloat("size", &fontSize) == B_OK)
				SetFontSize(fontSize);
			break;
		}
		case FONT_FAMILY:
		{
			const char* fontFamily = NULL;
			const char* fontStyle = NULL;
			void* ptr;
			if (message->FindPointer("source", &ptr) == B_OK) {
				fCurrentFontItem = static_cast<BMenuItem*>(ptr);
				fontFamily = fCurrentFontItem->Label();
			}
			SetFontStyle(fontFamily, fontStyle);
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
					fCurrentFontItem = menu->Superitem();
					if (fCurrentFontItem != NULL)
						fontFamily = fCurrentFontItem->Label();
				}
			}
			SetFontStyle(fontFamily, fontStyle);
			break;
		}
		case FONT_COLOR:
		{
			void* ptr;
			if (message->FindPointer("source", &ptr) == B_OK) {
				if (ptr == fBlackItem)
					SetFontColor(&BLACK);
				else if (ptr == fRedItem)
					SetFontColor(&RED);
				else if (ptr == fGreenItem)
					SetFontColor(&GREEN);
				else if (ptr == fBlueItem)
					SetFontColor(&BLUE);
				else if (ptr == fCyanItem)
					SetFontColor(&CYAN);
				else if (ptr == fMagentaItem)
					SetFontColor(&MAGENTA);
				else if (ptr == fYellowItem)
					SetFontColor(&YELLOW);
			}
			break;
		}

		// Document menu

		case ALIGN_LEFT:
			fTextView->SetAlignment(B_ALIGN_LEFT);
			fClean = false;
			fUndoCleans = false;
			fRedoCleans = false;	
			fRevertItem->SetEnabled(fSaveMessage != NULL);
			fSaveItem->SetEnabled(true);
			fUndoItem->SetLabel("Can't Undo");	
			fUndoItem->SetEnabled(false);
			fCanUndo = false;
			fCanRedo = false;
			break;
		case ALIGN_CENTER:
			fTextView->SetAlignment(B_ALIGN_CENTER);
			fClean = false;
			fUndoCleans = false;
			fRedoCleans = false;	
			fRevertItem->SetEnabled(fSaveMessage != NULL);
			fSaveItem->SetEnabled(true);
			fUndoItem->SetLabel("Can't Undo");	
			fUndoItem->SetEnabled(false);
			fCanUndo = false;
			fCanRedo = false;
			break;
		case ALIGN_RIGHT:
			fTextView->SetAlignment(B_ALIGN_RIGHT);
			fClean = false;
			fUndoCleans = false;
			fRedoCleans = false;	
			fRevertItem->SetEnabled(fSaveMessage != NULL);
			fSaveItem->SetEnabled(true);
			fUndoItem->SetLabel("Can't Undo");	
			fUndoItem->SetEnabled(false);
			fCanUndo = false;
			fCanRedo = false;
			break;
		case WRAP_LINES:
			if (fTextView->DoesWordWrap()) {
				fTextView->SetWordWrap(false);
				fWrapItem->SetMarked(false);
				BRect textRect;
				textRect = fTextView->Bounds();
				textRect.OffsetTo(B_ORIGIN);
				textRect.InsetBy(TEXT_INSET,TEXT_INSET);
				// the width comes from stylededit R5. TODO: find a better way
				textRect.SetRightBottom(BPoint(1500.0,textRect.RightBottom().y));
				fTextView->SetTextRect(textRect);
			} else {
				fTextView->SetWordWrap(true);
				fWrapItem->SetMarked(true);
				BRect textRect;
				textRect = fTextView->Bounds();
				textRect.OffsetTo(B_ORIGIN);
				textRect.InsetBy(TEXT_INSET,TEXT_INSET);
				fTextView->SetTextRect(textRect);
			}	
			fClean = false;
			fUndoCleans = false;
			fRedoCleans = false;	
			fRevertItem->SetEnabled(fSaveMessage != NULL);
			fSaveItem->SetEnabled(true);
			fUndoItem->SetLabel("Can't Undo");	
			fUndoItem->SetEnabled(false);
			fCanUndo = false;
			fCanRedo = false;
			break;
		case ENABLE_ITEMS:
			fCutItem->SetEnabled(true);
			fCopyItem->SetEnabled(true);
			fClearItem->SetEnabled(true);
			break;
		case DISABLE_ITEMS:
			fCutItem->SetEnabled(false);
			fCopyItem->SetEnabled(false);
			fClearItem->SetEnabled(false);
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
				fUndoItem->SetLabel("Redo Typing");
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
				fUndoItem->SetLabel("Undo Typing");
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
				fTextView->SetEncoding((uint32)fSavePanelEncodingMenu->IndexOf((BMenuItem*)ptr));
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
	if (fCurrentFontItem != NULL)
		fCurrentFontItem->SetMarked(false);

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

	if (sameProperties & B_FONT_FAMILY_AND_STYLE) {
		font_family family;
		font_style style;
		font.GetFamilyAndStyle(&family, &style);
		fCurrentFontItem = fFontMenu->FindItem(family);
		if (fCurrentFontItem != NULL) {
			fCurrentFontItem->SetMarked(true);
			BMenu* menu = fCurrentFontItem->Submenu();
			if (menu != NULL) {
				BMenuItem* item = menu->FindItem(style);
				if (item != NULL)
					item->SetMarked(true);
			}
		}
	}

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


void
StyledEditWindow::Quit()
{
	styled_edit_app->CloseDocument();	
	BWindow::Quit();
}


bool
StyledEditWindow::QuitRequested()
{
	int32 buttonIndex = 0;

	if (fClean)
		return true;

	BAlert *saveAlert;
	BString alertText;
	alertText.SetTo("Save changes to the document \"");
	alertText<< Title();
	alertText<<"\"? ";
	saveAlert = new BAlert("savealert",alertText.String(), "Cancel", "Don't Save","Save",
		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
	saveAlert->SetShortcut(0, B_ESCAPE);
	saveAlert->SetShortcut(1, 'd');
	saveAlert->SetShortcut(2, 's');
	buttonIndex = saveAlert->Go();

	if (buttonIndex == 0) {
		// "cancel": dont save, dont close the window
		return false;
	} else if (buttonIndex == 1) {
		// "don't save": just close the window
		return true;
	} else if (!fSaveMessage) {
		// save as
		BMessage* message = new BMessage(SAVE_THEN_QUIT);
		SaveAs(message);
		return false;
	}

	return Save() == B_OK;
}


status_t
StyledEditWindow::Save(BMessage *message)
{
	status_t err = B_OK;

	if (!message){
		message = fSaveMessage;
		if (!message)
			return B_ERROR;
	}

	entry_ref dirRef;
	err = message->FindRef("directory", &dirRef);
	if (err!= B_OK)
		return err;

	const char* name;
	err = message->FindString("name", &name);
	if (err!= B_OK)
		return err;

	BDirectory dir(&dirRef);
	err = dir.InitCheck();
	if (err != B_OK)
		return err;

	BEntry entry(&dir, name);
	err = entry.InitCheck();
	if (err != B_OK)
		return err;

	BFile file(&entry, B_READ_WRITE | B_CREATE_FILE);
	err = file.InitCheck();
	if (err != B_OK)
		return err;

	err = fTextView->WriteStyledEditFile(&file);
	if (err != B_OK) {
		BAlert *saveFailedAlert;
		BString alertText;
		if (err == B_TRANSLATION_ERROR_BASE)
			alertText.SetTo("Translation error saving \"");
		else
			alertText.SetTo("Unknown error saving \"");

		alertText << name;
		alertText << "\".";
		saveFailedAlert = new BAlert("saveFailedAlert", alertText.String(), "Bummer",
			0, 0, B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_STOP_ALERT);
		saveFailedAlert->SetShortcut(0, B_ESCAPE);
		saveFailedAlert->Go();
		return err;
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
	return err;
}


status_t
StyledEditWindow::SaveAs(BMessage *message)
{
	if (fSavePanel == NULL) {
		entry_ref* directory = NULL;
		if (fSaveMessage != NULL) {
			entry_ref dirRef;
			if (fSaveMessage->FindRef("directory", &dirRef))
				directory = new entry_ref(dirRef);
		}

		fSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this),
			directory, B_FILE_NODE, false);

		BMenuBar* menuBar = dynamic_cast<BMenuBar*>(
			fSavePanel->Window()->FindView("MenuBar"));

		fSavePanelEncodingMenu= new BMenu("Encoding");
		menuBar->AddItem(fSavePanelEncodingMenu);
		fSavePanelEncodingMenu->SetRadioMode(true);

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
			BMenuItem * item = new BMenuItem(name.String(), new BMessage(SAVE_AS_ENCODING));
			item->SetTarget(this);
			fSavePanelEncodingMenu->AddItem(item);
			if (charset.GetFontID() == fTextView->GetEncoding())
				item->SetMarked(true);
		}
	}

	fSavePanel->SetSaveText(Title()); 
	if (message != NULL)
		fSavePanel->SetMessage(message);

	fSavePanel->Show();
	return B_OK;
}


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
			strcpy(name, "???");

		char text[B_PATH_NAME_LENGTH + 100];
		snprintf(text, sizeof(text), "Error loading \"%s\":\n\t%s", name,
			strerror(status));

		BAlert* alert = new BAlert("StyledEdit Load Failed", text,
			"Bummer", 0, 0, B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_STOP_ALERT);
		alert->Go();
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


void
StyledEditWindow::OpenFile(entry_ref* ref)
{
	if (_LoadFile(ref) != B_OK) {
		fSaveItem->SetEnabled(true);
			// allow saving new files
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
	}
	fTextView->Select(0, 0);
}


void
StyledEditWindow::RevertToSaved()
{
	entry_ref ref;
	const char *name;

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
 		BAlert *vanishedAlert;
		BString alertText;
		alertText.SetTo("Cannot revert, file not found: \"");
		alertText << name;
		alertText << "\".";
		vanishedAlert = new BAlert("vanishedAlert", alertText.String(), "Bummer", 0, 0, 
			B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_STOP_ALERT);
		vanishedAlert->SetShortcut(0, B_ESCAPE);
		vanishedAlert->Go();
		return;
	}

	int32 buttonIndex = 0; 
	BAlert* revertAlert;
	BString alertText;
	alertText.SetTo("Revert to the last version of \"");
	alertText << Title();
	alertText << "\"? ";
	revertAlert= new BAlert("revertAlert", alertText.String(), "Cancel", "OK", 0,
		B_WIDTH_AS_USUAL, B_EVEN_SPACING, B_WARNING_ALERT);
	revertAlert->SetShortcut(0, B_ESCAPE);
	revertAlert->SetShortcut(1, 'o');
	buttonIndex = revertAlert->Go();

	if (buttonIndex != 1) {
		// some sort of cancel, don't revert
		return;
	}

	fTextView->Reset();

	if (_LoadFile(&ref) != B_OK)
		return;

	// clear undo modes
	fUndoItem->SetLabel("Can't Undo");
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
	status_t result;

	if (fPrintSettings == NULL) {
		result = PageSetup(documentName);
		if (result != B_OK)
			return;
	} 

	BPrintJob printJob(documentName);
	printJob.SetSettings(new BMessage(*fPrintSettings));
	result = printJob.ConfigJob();
	if (result != B_OK)
		return;

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
		while (currentHeight < printableRect.Height() && currentLine < linesInDocument) {
			currentHeight += fTextView->LineHeight(currentLine);
			if (currentHeight < printableRect.Height())
				currentLine++;
		}
		if (pagesInDocument == lastPage)
			lastLine = currentLine;

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
	int32 printLine = firstLine;
	while (printLine < lastLine) {
		float currentHeight = 0;
		int32 firstLineOnPage = printLine;
		while (currentHeight < printableRect.Height() && printLine < lastLine) {
			currentHeight += fTextView->LineHeight(printLine);
			if (currentHeight < printableRect.Height())
				printLine++;
		}

		float top = 0;
		if (firstLineOnPage != 0)
			top = fTextView->TextHeight(0, firstLineOnPage - 1);

		float bottom = fTextView->TextHeight(0, printLine - 1);
		BRect textRect(0.0, top + TEXT_INSET, printableRect.Width(), bottom + TEXT_INSET);
		printJob.DrawView(fTextView, textRect, BPoint(0.0,0.0));
		printJob.SpoolPage();
	}

	printJob.CommitJob();
}


bool
StyledEditWindow::Search(BString string, bool caseSensitive, bool wrap, bool backsearch)
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
	if (backsearch) {
		if (caseSensitive) {
			start = viewText.FindLast(string, textStart);
		} else {
			start = viewText.IFindLast(string, textStart);
		}
	} else {
		if (caseSensitive == true) {
			start = viewText.FindFirst(string, textFinish);
		} else {
			start = viewText.IFindFirst(string, textFinish);
		}
	}
	if (start == B_ERROR && wrap) {
		if (backsearch) {
			if (caseSensitive) {
				start = viewText.FindLast(string, viewText.Length());
			} else {
				start = viewText.IFindLast(string, viewText.Length());
			}
		} else {
			if (caseSensitive) {
				start = viewText.FindFirst(string, 0);
			} else {
				start = viewText.IFindFirst(string, 0);
			}
		}
	}

	if (start != B_ERROR) {
		finish = start + length;
		fTextView->Select(start, finish);
		fTextView->ScrollToSelection();
		return true;
	}

	return false;	
}


void 
StyledEditWindow::FindSelection()
{
	int32 selectionStart, selectionFinish;
	fTextView->GetSelection(&selectionStart, &selectionFinish);

	int32 selectionLength = selectionFinish- selectionStart;

	BString viewText = fTextView->Text();
	viewText.CopyInto(fStringToFind, selectionStart, selectionLength);
	fFindAgainItem->SetEnabled(true);
	Search(fStringToFind, fCaseSens, fWrapAround, fBackSearch);
}


bool
StyledEditWindow::Replace(BString findthis, BString replaceWith, bool caseSensitive,
	bool wrap, bool backsearch)
{
	if (Search(findthis, caseSensitive, wrap, backsearch)) {
		int32 start, finish;
		fTextView->GetSelection(&start, &finish);

		fTextView->Delete(start, start + findthis.Length());
		fTextView->Insert(start, replaceWith.String(), replaceWith.Length());
		fTextView->Select(start, start + replaceWith.Length());
		fTextView->ScrollToSelection();
		return true;
	}

	return false;
}


void
StyledEditWindow::ReplaceAll(BString findIt, BString replaceWith, bool caseSensitive)
{
	BString viewText(fTextView->Text());
	if (caseSensitive)
		viewText.ReplaceAll(findIt.String(), replaceWith.String());
	else
		viewText.IReplaceAll(findIt.String(), replaceWith.String());

	if (viewText.Compare(fTextView->Text()) == 0) {
		// they are the same
		return;
	}

	int32 textStart, textFinish;
	fTextView->GetSelection(&textStart, &textFinish);

	fTextView->SetText(viewText.String());

	if (viewText.Length() < textStart) 
		textStart = viewText.Length();
	if (viewText.Length() < textFinish) 
		textFinish = viewText.Length();

	fTextView->Select(textStart,textFinish);
	fTextView->ScrollToSelection();

	fClean = false;
	fUndoCleans = false;
	fRedoCleans = false;	
	fRevertItem->SetEnabled(fSaveMessage != NULL);
	fSaveItem->SetEnabled(true);
	fUndoItem->SetLabel("Can't Undo");	
	fUndoItem->SetEnabled(false);
	fCanUndo = false;
	fCanRedo = false;
}


void
StyledEditWindow::SearchAllWindows(BString find, BString replace, bool caseSensitive)
{
	int32 numWindows;
	numWindows = be_app->CountWindows();

	BMessage *message;
	message= new BMessage(MSG_REPLACE_ALL);
	message->AddString("FindText", find);
	message->AddString("ReplaceText", replace);
	message->AddBool("casesens", caseSensitive);

	while (numWindows >= 0) {
		StyledEditWindow *window = dynamic_cast<StyledEditWindow *>(
			be_app->WindowAt(numWindows));

		BMessenger messenger(window);
		messenger.SendMessage(message);

		numWindows--;
	}
}


void
StyledEditWindow::SetFontSize(float fontSize)
{
	uint32 sameProperties;
	BFont font;

	fTextView->GetFontAndColor(&font, &sameProperties);
	font.SetSize(fontSize);
	fTextView->SetFontAndColor(&font, B_FONT_SIZE);
	fClean = false;
	fUndoCleans = false;
	fRedoCleans = false;
	fRevertItem->SetEnabled(fSaveMessage != NULL);
	fSaveItem->SetEnabled(true);
	fUndoItem->SetLabel("Can't Undo");
	fUndoItem->SetEnabled(false);
	fCanUndo = false;
	fCanRedo = false;
}


void
StyledEditWindow::SetFontColor(const rgb_color *color)
{
	uint32 sameProperties;
	BFont font;

	fTextView->GetFontAndColor(&font, &sameProperties, NULL, NULL);
	fTextView->SetFontAndColor(&font, 0, color);
	fClean = false;
	fUndoCleans = false;
	fRedoCleans = false;	
	fRevertItem->SetEnabled(fSaveMessage != NULL);
	fSaveItem->SetEnabled(true);
	fUndoItem->SetLabel("Can't Undo");	
	fUndoItem->SetEnabled(false);
	fCanUndo = false;
	fCanRedo = false;
}


void
StyledEditWindow::SetFontStyle(const char *fontFamily, const char *fontStyle)
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
		if (oldItem != NULL)
			oldItem->SetMarked(false);
	}

	font.SetFamilyAndStyle(fontFamily, fontStyle);
	fTextView->SetFontAndColor(&font);

	BMenuItem* superItem;
	superItem = fFontMenu->FindItem(fontFamily);
	if (superItem != NULL)
		superItem->SetMarked(true);

	fClean = false;
	fUndoCleans = false;
	fRedoCleans = false;	
	fRevertItem->SetEnabled(fSaveMessage != NULL);
	fSaveItem->SetEnabled(true);
	fUndoItem->SetLabel("Can't Undo");	
	fUndoItem->SetEnabled(false);
	fCanUndo = false;
	fCanRedo = false;
}
