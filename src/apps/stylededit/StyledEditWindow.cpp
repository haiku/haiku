// Be-defined headers

#ifndef _ALERT_H
#include <Alert.h>
#endif

#ifndef _CLIPBOARD_H
#include <Clipboard.h>
#endif

#ifndef _FILE_H
#include <File.h>
#endif

#ifndef _MENU_H
#include <Menu.h>
#endif

#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif

#ifndef _PRINTSESSION_H
#include <PrintJob.h>
#endif

#ifndef _SCROLL_VIEW_H
#include <ScrollView.h>
#endif

#ifndef _STDLIB_H
#include <stdlib.h>
#endif

#ifndef _STRING_H
#include <String.h>
#endif

#ifndef _TRANSLATION_UTILS_H 
#include <TranslationUtils.h>
#endif

#ifndef _WINDOW_H
#include <Window.h>
#endif

//****** Application defined header files************/.
#ifndef CONSTANTS_H
#include "Constants.h"
#endif

#ifndef COLOR_MENU_ITEM_H
#include "ColorMenuItem.h"
#endif

#ifndef FIND_WINDOW_H
#include "FindWindow.h"
#endif

#ifndef REPLACE_WINDOW_H
#include "ReplaceWindow.h"
#endif

#ifndef STYLED_EDIT_APP
#include "StyledEditApp.h"
#endif

#ifndef STYLED_EDIT_VIEW_H
#include "StyledEditView.h"
#endif

#ifndef STYLED_EDIT_WINDOW_H
#include "StyledEditWindow.h"
#endif

StyledEditWindow::StyledEditWindow(BRect frame)
	: BWindow(frame,"untitled",B_DOCUMENT_WINDOW,0){
		InitWindow(); 
		Show();
	}/***StyledEditWindow()***/

StyledEditWindow::StyledEditWindow(BRect frame, entry_ref *ref)
	: BWindow(frame,"untitled",B_DOCUMENT_WINDOW,0)
	{
		InitWindow();
		OpenFile(ref); 
		Show();
	} /***StyledEditWindow()***/
	
StyledEditWindow::~StyledEditWindow(){
	Unregister();
	
	if(fSaveMessage)
		delete fSaveMessage;
	if(fPrintSettings)
		delete fPrintSettings;
	
	delete fSavePanel;
}/***~StyledEditWindow()***/
	
void StyledEditWindow::InitWindow(){
	
	fPrintSettings= NULL;
	fSaveMessage= NULL;
	fEnableItems= false;
	fFirstEdit= true;
	fUnDoneFlag= false;
	fTextSaved= true; //initial state, no text changed.
	
	//search- state
	fReplaceString= "";
	fStringToFind="";
	fCaseSens= false;
	fWrapAround= false;
	fBackSearch= false;
	
	//add menubar
	BRect			menuBarRect;
	
	menuBarRect= Bounds();
	fMenuBar= new BMenuBar(menuBarRect,"menubar");
	
	AddChild(fMenuBar);

	//add textview and scrollview
	BRect			viewFrame;
	BRect			textBounds;
	
	viewFrame= Bounds();
	
	viewFrame.top = MENU_BAR_HEIGHT+1;
	viewFrame.right -=  B_V_SCROLL_BAR_WIDTH;
	viewFrame.bottom -= B_H_SCROLL_BAR_HEIGHT; 
	
	
	textBounds= viewFrame;
	textBounds.OffsetTo(B_ORIGIN);
	textBounds.InsetBy(TEXT_INSET, TEXT_INSET);
	
	
	fTextView= new StyledEditView(viewFrame, textBounds, this);
	fTextView-> SetDoesUndo(true);
	fTextView-> SetStylable(true);
	fTextView-> MakeFocus(true);
	
	fScrollView= new BScrollView("scrollview", fTextView, B_FOLLOW_ALL, 0, true, true, B_NO_BORDER);
	AddChild(fScrollView);
	
	
		
	//Add "File"-menu:
	BMenu			*menu;
	BMenu			*subMenu;
	BMenuItem		*menuItem;
	
	menu= new BMenu("File");
	fMenuBar-> AddItem(menu);
	
	menu-> AddItem(menuItem= new BMenuItem("New", new BMessage(MENU_NEW), 'N'));
	menu-> AddItem(menuItem= new BMenuItem("Open..", new BMessage(MENU_OPEN)));
	menuItem->SetTarget(be_app);
	menu-> AddSeparatorItem(); 
	
	menu-> AddItem(fSaveItem= new BMenuItem("Save", new BMessage(MENU_SAVE), 'S')); 
	fSaveItem-> SetEnabled(false); 
	menu-> AddItem(menuItem= new BMenuItem("Save as", new BMessage(MENU_SAVEAS)));
	menuItem-> SetEnabled(true);				
	
	menu-> AddItem(menuItem= new BMenuItem("Revert to Saved", new BMessage(MENU_REVERT))); 
	menuItem-> SetEnabled(true); 									
	menu-> AddItem(menuItem= new BMenuItem("Close", new BMessage(MENU_CLOSE), 'W'));
	
	menu-> AddSeparatorItem();
	menu-> AddItem(menuItem= new BMenuItem("Page Setup" B_UTF8_ELLIPSIS, new BMessage(MENU_PAGESETUP)));
	menu-> AddItem(menuItem= new BMenuItem("Print" B_UTF8_ELLIPSIS, new BMessage(MENU_PRINT), 'P'));
	
	menu-> AddSeparatorItem();
	menu-> AddItem(menuItem= new BMenuItem("Quit", new BMessage(MENU_QUIT), 'Q'));
	
	//Add the "Edit"-menu:
	menu= new BMenu("Edit");
	fMenuBar-> AddItem(menu);
	
	menu-> AddItem(fUndoItem= new BMenuItem("Can't Undo", new BMessage(B_UNDO), 'Z'));
	fUndoItem-> SetEnabled(false);
	fUndoItem-> SetTarget(fTextView);
	
	menu-> AddSeparatorItem();
	menu-> AddItem(fCutItem= new BMenuItem("Cut", new BMessage(B_CUT), 'X'));
	fCutItem->SetEnabled(false);
	fCutItem-> SetTarget(fTextView);

	menu-> AddItem(fCopyItem= new BMenuItem("Copy", new BMessage(B_COPY), 'C'));
	fCopyItem-> SetEnabled(false);
	fCopyItem-> SetTarget(fTextView);

	menu-> AddItem(menuItem= new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));
	menuItem-> SetTarget(fTextView);
	menu-> AddItem(fClearItem= new BMenuItem("Clear", new BMessage(MENU_CLEAR)));
	fClearItem-> SetEnabled(false); 
	fClearItem-> SetTarget(fTextView);
	
	
	menu-> AddSeparatorItem();
	menu-> AddItem(menuItem=new BMenuItem("Select All", new BMessage(B_SELECT_ALL), 'A'));
	menuItem-> SetTarget(fTextView);
	
	menu-> AddSeparatorItem();
	menu-> AddItem(menuItem= new BMenuItem("Find", new BMessage(MENU_FIND),'F'));
	menu-> AddItem(menuItem= new BMenuItem("Find Again",new BMessage(MENU_FIND_AGAIN),'G'));
	menu-> AddItem(menuItem= new BMenuItem("Find Selection", new BMessage(MENU_FIND_SELECTION),'H'));
	menu-> AddItem(menuItem= new BMenuItem("Replace", new BMessage(MENU_REPLACE),'R'));
	menu-> AddItem(menuItem= new BMenuItem("Replace Same", new BMessage(MENU_REPLACE_SAME),'T'));
	
	//Add the "Font"-menu:
	BMessage		*fontMessage;
	menu= new BMenu("Font");
	fMenuBar-> AddItem(menu);
	
	//"Size"-subMenu
	subMenu=new BMenu("Size");
	subMenu-> SetRadioMode(true);
	menu-> AddItem(subMenu);
	
	subMenu-> AddItem(menuItem= new BMenuItem("9", fontMessage= new BMessage(FONT_SIZE)));
	fontMessage-> AddFloat("size", 9.0);
	
	subMenu-> AddItem(menuItem= new BMenuItem("10", fontMessage= new BMessage(FONT_SIZE)));
	fontMessage-> AddFloat("size",10.0);
	menuItem-> SetMarked(true);
	
	subMenu-> AddItem(menuItem= new BMenuItem("12", fontMessage= new BMessage(FONT_SIZE)));
	fontMessage-> AddFloat("size",12.0);
	subMenu-> AddItem(menuItem= new BMenuItem("14", fontMessage= new BMessage(FONT_SIZE)));
	fontMessage-> AddFloat("size",14.0);
	subMenu-> AddItem(menuItem= new BMenuItem("18", fontMessage= new BMessage(FONT_SIZE)));
	fontMessage-> AddFloat("size",18.0);
	
	subMenu-> AddItem(menuItem= new BMenuItem("24", fontMessage= new BMessage(FONT_SIZE)));
	fontMessage-> AddFloat("size",24.0);
	subMenu-> AddItem(menuItem= new BMenuItem("36", fontMessage= new BMessage(FONT_SIZE)));
	fontMessage-> AddFloat("size",36.0);
	subMenu-> AddItem(menuItem= new BMenuItem("48", fontMessage= new BMessage(FONT_SIZE)));
	fontMessage-> AddFloat("size",48.0);
	subMenu-> AddItem(menuItem= new BMenuItem("72", fontMessage= new BMessage(FONT_SIZE)));
	fontMessage-> AddFloat("size",72.0);
	
	//"Color"-subMenu
	subMenu= new BMenu("Color");
	subMenu->SetRadioMode(true);
	menu-> AddItem(subMenu);
	
	ColorMenuItem *ColorItem; 
	
	subMenu-> AddItem(menuItem= new BMenuItem("Black", fontMessage= new BMessage(FONT_COLOR)));
	fontMessage-> AddPointer("colour",&BLACK);
	menuItem-> SetMarked(true); 
	subMenu-> AddItem(ColorItem= new ColorMenuItem("Red", RED, fontMessage= new BMessage(FONT_COLOR)));
	fontMessage-> AddPointer("colour", &RED);
	subMenu-> AddItem(ColorItem= new ColorMenuItem("Green", GREEN, fontMessage= new BMessage(FONT_COLOR)));
	fontMessage-> AddPointer("colour", &GREEN);
	subMenu-> AddItem(ColorItem= new ColorMenuItem("Blue", BLUE, fontMessage= new BMessage(FONT_COLOR)));
	fontMessage-> AddPointer("colour", &BLUE);
	subMenu-> AddItem(ColorItem= new ColorMenuItem("Cyan", CYAN, fontMessage= new BMessage(FONT_COLOR)));
	fontMessage-> AddPointer("colour",&CYAN);
	subMenu-> AddItem(ColorItem= new ColorMenuItem("Magenta", MAGENTA, fontMessage= new BMessage(FONT_COLOR)));
	fontMessage-> AddPointer("colour", &MAGENTA);
	subMenu-> AddItem(ColorItem= new ColorMenuItem("Yellow", YELLOW, fontMessage= new BMessage(FONT_COLOR)));
	fontMessage-> AddPointer("colour", &YELLOW);
	menu->AddSeparatorItem();

	//Available fonts

	int32 numFamilies = count_font_families();
	for ( int32 i = 0; i < numFamilies; i++ ) {
		font_family localfamily;
		if ( get_font_family ( i, &localfamily ) == B_OK ) {
			subMenu=new BMenu(localfamily);
			subMenu-> SetRadioMode(true);
			menu->AddItem(subMenu);
			menu->SetRadioMode(true);
			int32 numStyles=count_font_styles(localfamily);
			for(int32 j = 0;j<numStyles;j++){
				font_style style;
				uint32 flags;
				if( get_font_style(localfamily,j,&style,&flags)==B_OK){
					subMenu-> AddItem(menuItem= new BMenuItem(style, fontMessage= new BMessage(FONT_STYLE)));
					fontMessage-> AddString("FontFamily", localfamily);
					fontMessage-> AddString("FontStyle", style);
				}
			}
		}
	}	
				
	//Add the "Document"-menu:
	menu= new BMenu("Document");
	fMenuBar-> AddItem(menu);
	
	//"Align"-subMenu:
	subMenu= new BMenu("Align");
	subMenu-> SetRadioMode(true); 
	subMenu-> AddItem(menuItem= new BMenuItem("Left", new BMessage(ALIGN_LEFT)));
	menuItem-> SetMarked(true); 
	
	subMenu-> AddItem(menuItem= new BMenuItem("Center", new BMessage(ALIGN_CENTER)));
	subMenu-> AddItem(menuItem= new BMenuItem("Right", new BMessage(ALIGN_RIGHT)));
	menu-> AddItem(subMenu);
	menu-> AddItem(fWrapItem= new BMenuItem("Wrap Lines", new BMessage(WRAP_LINES)));
	fWrapItem-> SetMarked(true);
	/***************************MENUS ADDED***********************/
	
	
	fSavePanel= new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false);
	Register(true);
	Minimize(true);	
	}  /***StyledEditWindow::Initwindow()***/
	
	void StyledEditWindow::MessageReceived(BMessage *message){
		switch(message->what){
			case WINDOW_REGISTRY_ADDED:
			{
				
				BRect rect;
				if (message->FindInt32("new_window_number", &fWindow_Id) == B_OK) 
				{
					if (!fSaveMessage)
					{		
						BString unTitled;
						unTitled.SetTo("Untitled ");
						unTitled << fWindow_Id;
						const char *title= unTitled.String();
						SetTitle(title);  
					}
				}
				if (message->FindRect("rect", &rect) == B_OK) 
				{
					MoveTo(rect.LeftTop());
					ResizeTo(rect.Width(), rect.Height());
				}
				Minimize(false);
			}
			break;		  
/************file menu:***************/
			case MENU_NEW:   
				{
				BRect newViewFrame;
				newViewFrame= Frame();
				new StyledEditWindow(newViewFrame);
				}
			break;
			case MENU_SAVE:
				{
				if(!fSaveMessage)
					fSavePanel->Show();
				else
					Save(fSaveMessage);
				}
			break;		
			case MENU_SAVEAS:
				fSavePanel->Show();
			break;
			case B_SAVE_REQUESTED:
				Save(message);
			break;
			case MENU_REVERT:
				RevertToSaved();
			break;
			case MENU_CLOSE:{
				if(this->QuitRequested())
					Quit();
			}		
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
/*********commands from the "Edit"-menu:**************/
			case B_UNDO:
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
				BRect findWindowFrame(100,100,400,235);
				FindWindow *find;
				find= new FindWindow(findWindowFrame, this, &fStringToFind, &fCaseSens,
					&fWrapAround, &fBackSearch);
				}
			break;
			case MSG_SEARCH:
				{
				message->FindString("findtext", &fStringToFind);
				message->FindBool("casesens", &fCaseSens);
				message->FindBool("wrap", &fWrapAround);
				message->FindBool("backsearch", &fBackSearch);
				
				Search(fStringToFind, fCaseSens, fWrapAround, fBackSearch);
				} 
				break;
			case MENU_FIND_AGAIN:
				Search(fStringToFind, fCaseSens, fWrapAround, fBackSearch);
			break;
			case MENU_FIND_SELECTION:
				FindSelection();
			break;
			case MENU_REPLACE:
				{
				BRect replaceWindowFrame(100,100,400,284);
				ReplaceWindow *replace;
				replace= new ReplaceWindow(replaceWindowFrame, this, &fStringToFind, &fReplaceString, &fCaseSens,
					&fWrapAround, &fBackSearch);
				}
			break;
			case MSG_REPLACE:
			{
				BString findIt; 
				BString replaceWith;
				bool caseSens, wrap, backSearch;
				
				message->FindBool("casesens", &caseSens);
				message->FindBool("wrap", &wrap);
				message->FindBool("backsearch", &backSearch);
				
				
				message->FindString("FindText",&findIt);
				message->FindString("ReplaceText",&replaceWith);
				fStringToFind= findIt;
				fReplaceString= replaceWith;
				fCaseSens= caseSens;
				fWrapAround= wrap;
				fBackSearch= backSearch;
				
				Replace(findIt, replaceWith, caseSens, wrap, backSearch);
			}
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
				
				fStringToFind= findIt;
				fReplaceString= replaceWith;
				fCaseSens= caseSens;
				
				
				if(allWindows)
					SearchAllWindows(findIt, replaceWith, caseSens);
				else	
					ReplaceAll(findIt, replaceWith,caseSens);
			
			}	
			break;	
	/*********"Font"-menu*****************/
			case FONT_SIZE:
			{
				float fontSize;
				message->FindFloat("size",&fontSize);
				SetFontSize(fontSize);
			}
			break;
			case FONT_STYLE:
				{
				const char *fontFamily,*fontStyle;
				message-> FindString("FontFamily",&fontFamily);
				message-> FindString("FontStyle",&fontStyle);
				SetFontStyle(fontFamily, fontStyle);
				}
			break;
			case FONT_COLOR:
			{
			void		*v; 
			rgb_color 	*color;
			
			message-> FindPointer("colour",&v); 
			color= (rgb_color *) v; 
			SetFontColor(color);
			}
			break;
	/*********"Document"-menu*************/
			case ALIGN_LEFT:
			fTextView->SetAlignment(B_ALIGN_LEFT);
			break;
			case ALIGN_CENTER:
			fTextView->SetAlignment(B_ALIGN_CENTER);
			break;
			case ALIGN_RIGHT:
			fTextView->SetAlignment(B_ALIGN_RIGHT);
			break;
			case WRAP_LINES:
				if(fTextView->DoesWordWrap()){
					fTextView->SetWordWrap(false);
					fWrapItem->SetMarked(false);
					}
				else{
					fTextView->SetWordWrap(true);
					fWrapItem->SetMarked(true);
				}	
			break;
			case ENABLE_ITEMS:
				{
				fCutItem->SetEnabled(true);
				fCopyItem->SetEnabled(true);
				fClearItem->SetEnabled(true);
				}
			break;
			case DISABLE_ITEMS:
				{
				fCutItem->SetEnabled(false);
				fCopyItem->SetEnabled(false);
				fClearItem->SetEnabled(false);
				}
			break;
			case TEXT_CHANGED:
				{
				fSaveItem-> SetEnabled(true);
				fTextSaved= false;
				
				if(fFirstEdit){
					fUndoItem-> SetLabel("Undo Typing");	
					fUndoItem-> SetEnabled(true);
					}
				
				if (fUnDoneFlag){
					fUndoItem->SetLabel("Redo Typing");
					fUnDoneFlag= false;
					}
				else{
					fUndoItem->SetLabel("Undo Typing");
					fUnDoneFlag= true;
					}	
					
				}
			break;
			default:
			BWindow::MessageReceived(message);
			break;
		}
	}/***StyledEditWindow::MessageReceived() ***/

	
void StyledEditWindow::Register(bool need_id) {
		BMessenger messenger(APP_SIGNATURE);
		BMessage message(WINDOW_REGISTRY_ADD);
	
		message.AddBool("need_id", need_id);
		messenger.SendMessage(&message, this);
}/***Register()***/
	

void StyledEditWindow::Unregister(void){
		BMessenger messenger(APP_SIGNATURE);
		messenger.SendMessage(new BMessage(WINDOW_REGISTRY_SUB));
}/***Unregister()***/

	
bool StyledEditWindow::QuitRequested(){
		
		int32 buttonIndex= 0; 
		
		if(!fTextSaved){
			BAlert *saveAlert;
			BString alertText;
			alertText.SetTo("Save changes to the document ");
			alertText<< Title();
			alertText<<"? ";
			saveAlert= new BAlert("savealert",alertText.String(), "Cancel", "Don't save","Save",
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
			saveAlert-> SetShortcut(0, B_ESCAPE);
			buttonIndex= saveAlert-> Go();
		
			
			if(buttonIndex==0) 		//"cancel": dont save, dont close the window
				return false;
			else if(buttonIndex==1) // "don't save": just close the window
				return true;
			else if(!fSaveMessage){ //save as	
					DispatchMessage(new BMessage(MENU_SAVEAS), this);
					return false;
				}		
				else{	
					DispatchMessage(new BMessage(B_SAVE_REQUESTED), this);
					return true;
					}
		}		
			else
				return true;
				
}/***QuitRequested()***/
	
status_t StyledEditWindow::Save(BMessage *message){
		entry_ref ref;
		const char *name;
		status_t err;
		
		if(!message){
			message=fSaveMessage;
			if(!message)
				return B_ERROR;
		}
		
		err= message->FindRef("directory", &ref); 
		if(err!= B_OK)
			return err;
		
		err=message->FindString("name", &name); 
		if (err!= B_OK)
			return err;
			
		BDirectory dir(&ref);
		err= dir.InitCheck();
		if(err!= B_OK)
			return err;
		
		BFile file(&dir, name, B_READ_WRITE | B_CREATE_FILE);
		
		err= file.InitCheck();
		if(err!= B_OK)
			return err;
		
		err= BTranslationUtils::WriteStyledEditFile(fTextView, &file);
		
		if(err>= 0)
		{
			SetTitle(name);
			fSaveItem->SetEnabled(true);
			if(fSaveMessage!= message)
			{
				delete fSaveMessage;
				fSaveMessage= new BMessage(*message);
			}
		}
		
		fSaveItem-> SetEnabled(false); 
		fTextSaved= true;
		return err;						
} /***Save()***/

void StyledEditWindow::OpenFile(entry_ref *ref){
	
	BFile file;
	status_t fileinit;
	
	fileinit= file.SetTo(ref, B_READ_ONLY);
	
	if(fileinit== B_OK){
		status_t result;
		result= BTranslationUtils::GetStyledText(&file, fTextView, NULL); //he he he :)
		
		if(result==B_OK){
			fSaveMessage= new BMessage(B_SAVE_REQUESTED);
			if(fSaveMessage){
				BEntry entry(ref, true);
				BEntry parent;
				entry_ref parentRef;
				char name[B_FILE_NAME_LENGTH];
					
				entry.GetParent(&parent);
				entry.GetName(name);
				parent.GetRef(&parentRef);
				fSaveMessage->AddRef("directory",&parentRef);
				fSaveMessage->AddString("name", name);
				SetTitle(name);
			}	
		}
	}				
}/*** StyledEditWindow::OpenFile() ***/

void StyledEditWindow::RevertToSaved(){
																	//translationutils here as well
		entry_ref ref;
		const char *name;
				
		fSaveMessage->FindRef("directory", &ref);
		fSaveMessage->FindString("name", &name);
			
		BDirectory dir(&ref);
				
	 	BFile file(&dir, name, B_READ_ONLY | B_CREATE_FILE);
		fTextView->SetText(""); 									//clear the textview...
		BTranslationUtils::GetStyledText(&file, fTextView, NULL); 	//and fill it from the file
}/***StyledEditWindow::RevertToSaved()***/
	
status_t StyledEditWindow::PageSetup(const char *documentname){
		
	status_t result= B_ERROR;
		
	BPrintJob printJob(documentname);
		
	if (fPrintSettings!= NULL)
		printJob.SetSettings(fPrintSettings);
	//else
		//; ///??
		
	result= printJob.ConfigPage();
		
	if (result== B_NO_ERROR){
		delete fPrintSettings;
		fPrintSettings= printJob.Settings();
	}	
				
	return result;
}/***StyledEditWindow::PageSetup()***/
	
void StyledEditWindow::Print(const char *documentname){
	
	BPrintJob printJob(documentname);
														
	if (fPrintSettings== NULL){
		status_t pageSetup;
		pageSetup= PageSetup(fTextView->Window()->Title());
		
		if (pageSetup!= B_NO_ERROR)
			return;
		}
	else
		printJob.SetSettings(fPrintSettings);
	
	status_t configCheck;
	configCheck= printJob.ConfigJob();	
	if (configCheck== B_NO_ERROR){
		int32 currentPage= 1;
		int32 firstPage;
		int32 lastPage;
		int32 pagesInDocument;
		BRect pageRect;
		BRect currentPageRect;
		
		pageRect= printJob.PrintableRect();
		currentPageRect= pageRect;
		//not sure about this....
		BRect docRect;
		docRect= fTextView->TextRect();
		pagesInDocument= (int32)(docRect.Height()+ (pageRect.Height()-1) / pageRect.Height());
		
		firstPage= printJob.FirstPage();
		lastPage=  printJob.LastPage();
		
		if(lastPage> pagesInDocument)
			lastPage= pagesInDocument;
			
		printJob.BeginJob();
		
		for(currentPage= firstPage;currentPage<= lastPage;currentPage++){
			currentPageRect.OffsetTo(0, currentPage* pageRect.Height());
			printJob.DrawView(fTextView, currentPageRect, BPoint(0.0, 0.0));
			printJob.SpoolPage();
		}
		printJob.CommitJob();
	}		
}/***StyledEditWindow::Print()***/

bool StyledEditWindow::Search(BString string, bool casesens, bool wrap, bool backsearch){ 
	
	int32	start;
	int32	finish;
	int32	strlen;
	
	start= B_ERROR; 
	
	strlen= string.Length();      
	if (strlen== 0)
		return false;
	
	BString viewText;
	viewText.SetTo(fTextView->Text());
	int32 textStart, textFinish;
	fTextView->GetSelection(&textStart, &textFinish);
	//case insensitive, non wrap seems to be default in SE...
        if(casesens== true)
        	start= viewText.FindFirst(string, textFinish);
		else if(wrap== true)
			start= viewText.IFindFirst(string); 
		else if(backsearch== true)
			start= viewText.IFindLast(string, textStart); 
		else
			start= viewText.IFindFirst(string, textFinish); //i.e this one...
		
				
	if(start!= B_ERROR){
		finish= start+ strlen;
		fTextView->Select(start, finish);
		fTextView->ScrollToSelection();
		return true;
	} else
		return false;	

}/***StyledEditWindow::Search***/

void StyledEditWindow::FindSelection(){
	
	int32 selectionStart, selectionFinish;
	fTextView->GetSelection(&selectionStart,&selectionFinish);
				
	int32 selectionLength;
	selectionLength= selectionFinish- selectionStart;
				
	BString viewText;
	viewText= fTextView->Text();
				
	viewText.CopyInto(fStringToFind, selectionStart, selectionLength);
	Search(fStringToFind, false, false, false);

}/***StyledEditWindow::FindSelection()***/
	
void StyledEditWindow::Replace(BString findthis, BString replaceWith,  bool casesens, bool wrap, bool backsearch){

	int32	start;
	int32	replaceLength;
	int32	findLength;
	
	start= B_ERROR; 
	
	findLength= findthis.Length();
	replaceLength= replaceWith.Length();      
	
	BString viewText;
	viewText.SetTo(fTextView->Text()); 
	
	int32 textStart, textFinish;
	fTextView->GetSelection(&textStart, &textFinish);
	
        if(casesens== true)
        	start= viewText.FindFirst(findthis, textFinish);
		else if(wrap== true)
			start= viewText.IFindFirst(findthis); 
		else if(backsearch== true)
			start= viewText.IFindLast(findthis, textStart);
		else
			start= viewText.IFindFirst(findthis, textFinish); 
	
	if (start!= B_ERROR) {
		fTextView->Delete(start, start+ findLength);
		fTextView->Insert(start, replaceWith.String(), replaceLength);
		fTextView->Select(start, start+ replaceLength);
	}	
		
}/***StyledEditWindow::Replace()***/

void StyledEditWindow::ReplaceAll(BString findIt, BString replaceWith, bool caseSens){
	
	while(Search(findIt, caseSens, true, false)) 
		Replace(findIt, replaceWith, caseSens, true, false);

}/***StyledEditWindow::ReplaceAll()***/

void StyledEditWindow::SearchAllWindows(BString find, BString replace, bool casesens){
		
		int32 numWindows;
		numWindows= be_app->CountWindows();
		
		BMessage *message;
		message= new BMessage(MSG_REPLACE_ALL);
		message->AddString("FindText",find);
		message->AddString("ReplaceText",replace);
		message->AddBool("casesens", casesens);
		
		BMessenger *messenger;
		
		while(numWindows>=0){
			StyledEditWindow *win= dynamic_cast<StyledEditWindow *>(be_app->WindowAt(numWindows));
			messenger= new BMessenger(win);
			messenger->SendMessage(message);
			
			numWindows--;
		}
		
}/***StyledEditWindow::SearchAllWindows***/


void StyledEditWindow::SetFontSize(float fontSize){
	
	uint32 sameProperties;
	BFont font;
		
	fTextView->GetFontAndColor(&font,&sameProperties);
	font.SetSize(fontSize);
	fTextView->SetFontAndColor(&font,B_FONT_SIZE);
}/***StyledEditWindow::SetFontSize()***/

void StyledEditWindow::SetFontColor(rgb_color *color){
	uint32 sameProperties;
	BFont font;
	
	fTextView-> GetFontAndColor(&font,&sameProperties,NULL,NULL);
	fTextView-> SetFontAndColor(&font, B_FONT_ALL,color);
	
}/***StyledEditWindow::SetFontColor()***/

void StyledEditWindow::SetFontStyle(const char *fontFamily, const char *fontStyle){
	BFont font;
	uint32 sameProperties;
	
	fTextView->GetFontAndColor(&font,&sameProperties);
	font.SetFamilyAndStyle(fontFamily,fontStyle);
	fTextView->SetFontAndColor(&font);
	
	BMenuItem *superItem;
	superItem= fMenuBar->FindItem(fontFamily);
	superItem-> SetMarked(true);

}/***StyledEditWindow::SetFontStyle()***/

