#ifndef _BUTTON_H
#include <Button.h>
#endif

#ifndef _CHECK_BOX_H
#include <CheckBox.h>
#endif

#ifndef _STRING_H
#include <String.h>
#endif

#ifndef _TEXT_CONTROL_H
#include <TextControl.h>
#endif

#ifndef _WINDOW_H
#include <Window.h>
#endif

#ifndef CONSTANTS_H
#include "Constants.h"
#endif

#ifndef FIND_WINDOW_H
#include "FindWindow.h"
#endif



// FindWindow::FindWindow()
FindWindow::FindWindow(BRect frame, BHandler *_handler, BString *searchString, bool *caseState, bool *wrapState, bool *backState)
			: BWindow(frame," ", B_MODAL_WINDOW, B_NOT_RESIZABLE|B_ASYNCHRONOUS_CONTROLS, B_CURRENT_WORKSPACE) {
	AddChild(fFindView=new BView(BRect(1.0,1.0,Bounds().Width(),Bounds().Height()),"",B_FOLLOW_ALL_SIDES,B_WILL_DRAW));
	fFindView->SetViewColor(216,216,216);
	
	font_height height;
	fFindView->GetFontHeight(&height);
	float lineHeight = height.ascent+height.descent+height.leading;
	
	float findWidth = fFindView->StringWidth("Find:")+6;
															
	float searchBottom = 10 + 2 + lineHeight + 2 + 1;
	fFindView->AddChild(fSearchString= new BTextControl(BRect(14,10,frame.Width()-12,searchBottom), "", "Find:", NULL, NULL));
		fSearchString->SetDivider(findWidth);
	float buttonTop = frame.Height()-21-lineHeight;
	float wrapBoxTop = (buttonTop+searchBottom-lineHeight)/2;
	float wrapBoxBottom = (buttonTop+searchBottom+lineHeight)/2;
	float caseBoxTop = (searchBottom+wrapBoxTop-lineHeight)/2;
	float caseBoxBottom = (searchBottom+wrapBoxTop+lineHeight)/2;
	float backBoxTop = (buttonTop+wrapBoxBottom-lineHeight)/2;
	float backBoxBottom = (buttonTop+wrapBoxBottom+lineHeight)/2;
	
	fFindView->AddChild(fCaseSensBox= new BCheckBox(BRect(16+findWidth,caseBoxTop,frame.Width()-14,caseBoxBottom),"","Case-sensitive", NULL));
	fFindView->AddChild(fWrapBox= new BCheckBox(BRect(16+findWidth,wrapBoxTop,frame.Width()-14,wrapBoxBottom),"","Wrap-around search", NULL));
	fFindView->AddChild(fBackSearchBox= new BCheckBox(BRect(16+findWidth,backBoxTop,frame.Width()-14,backBoxBottom),"","Search backwards", NULL));
	
	fFindView->AddChild(fCancelButton= new BButton(BRect(140,buttonTop,210,frame.Height()-9),"","Cancel",new BMessage(B_QUIT_REQUESTED)));
	fFindView->AddChild(fSearchButton= new BButton(BRect(289-70,buttonTop,289,frame.Height()-9),"","Find",new BMessage(MSG_SEARCH)));
	
	fSearchButton->MakeDefault(true);
	fHandler=_handler;
	
	const char *text= searchString->String();
	
    fSearchString->SetText(text);
	fSearchString-> MakeFocus(true); //021021
	
	if(*caseState== true)
		fCaseSensBox->SetValue(B_CONTROL_ON);
	else
		fCaseSensBox->SetValue(B_CONTROL_OFF);
	
	if(*wrapState== true)
		fWrapBox->SetValue(B_CONTROL_ON);
	else
		fWrapBox->SetValue(B_CONTROL_OFF);
	
	if(*backState== true)
		fBackSearchBox->SetValue(B_CONTROL_ON);
	else
		fBackSearchBox->SetValue(B_CONTROL_OFF);	
	
	Show();
}

void FindWindow::MessageReceived(BMessage *msg){
	switch(msg->what){
		case B_QUIT_REQUESTED:
			Quit();
		break;
		case MSG_SEARCH:
			ExtractToMsg(new BMessage(MSG_SEARCH));
		break;
	default:
		BWindow::MessageReceived(msg);
		break;	
	}
}	
	
void FindWindow::DispatchMessage(BMessage *message, BHandler *handler)

{
	int8	key;
	
	if ( message->what == B_KEY_DOWN ) {
		status_t result;
		result= message->FindInt8("byte", 0, &key);
		
		if (result== B_OK){
			if (key== B_ESCAPE){
				message-> MakeEmpty();
				message-> what= B_QUIT_REQUESTED;
			}
		}
	}
	BWindow::DispatchMessage(message,handler);	
}

void FindWindow::ExtractToMsg(BMessage *message){
	
	int32	caseBoxState;
	int32 	wrapBoxState;
	int32 	backBoxState;
	
	caseBoxState= fCaseSensBox-> Value();
	wrapBoxState= fWrapBox-> Value();
	backBoxState= fBackSearchBox-> Value();
	
	bool 	caseSens;
	bool 	wrapIt;
	bool 	backSearch;
	
	if(caseBoxState== B_CONTROL_ON)	
		caseSens=true;
	else
		caseSens= false;
	
	if(wrapBoxState== B_CONTROL_ON)		 
		wrapIt= true;
	else
		wrapIt= false;
	
	if(backBoxState== B_CONTROL_ON)		 
		backSearch= true;
	else
		backSearch= false;	 
	
	//Add the string
	message->AddString("findtext",fSearchString->Text());
	
	//Add searchparameters from checkboxes
	message->AddBool("casesens", caseSens);
	message->AddBool("wrap", wrapIt);
	message->AddBool("backsearch", backSearch);

	fHandler->Looper()->PostMessage(message,fHandler);
	delete(message);
	PostMessage(B_QUIT_REQUESTED);
}


