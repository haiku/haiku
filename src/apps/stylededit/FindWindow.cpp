#include <Button.h>
#include <CheckBox.h>
#include <String.h>
#include <TextControl.h>
#include <Window.h>
#include <Box.h>
#include "Constants.h"
#include "FindWindow.h"


// FindWindow::FindWindow()
FindWindow::FindWindow(BRect frame, BHandler *_handler, BString *searchString,
	bool *caseState, bool *wrapState, bool *backState)
	: BWindow(frame, "FindWindow", B_MODAL_WINDOW,
		B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS,
		B_CURRENT_WORKSPACE)
{
	fFindView = new BBox(Bounds(), "FindView", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	fFindView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fFindView);

	font_height height;
	fFindView->GetFontHeight(&height);
	float lineHeight = height.ascent+height.descent+height.leading;
	
	float findWidth = fFindView->StringWidth("Find:")+6;
															
	float searchBottom = 12 + 2 + lineHeight + 2 + 1;
	float buttonTop = frame.Height()-19-lineHeight;
	float wrapBoxTop = (buttonTop+searchBottom-lineHeight)/2;
	float wrapBoxBottom = (buttonTop+searchBottom+lineHeight)/2;
	float caseBoxTop = (searchBottom+wrapBoxTop-lineHeight)/2;
	float caseBoxBottom = (searchBottom+wrapBoxTop+lineHeight)/2;
	float backBoxTop = (buttonTop+wrapBoxBottom-lineHeight)/2;
	float backBoxBottom = (buttonTop+wrapBoxBottom+lineHeight)/2;
	
	fFindView->AddChild(fSearchString= new BTextControl(BRect(14,12,frame.Width()-10,searchBottom), "", "Find:", NULL, NULL));
		fSearchString->SetDivider(findWidth);

	fFindView->AddChild(fCaseSensBox= new BCheckBox(BRect(16+findWidth,caseBoxTop,frame.Width()-12,caseBoxBottom),"","Case-sensitive", NULL));
	fFindView->AddChild(fWrapBox= new BCheckBox(BRect(16+findWidth,wrapBoxTop,frame.Width()-12,wrapBoxBottom),"","Wrap-around search", NULL));
	fFindView->AddChild(fBackSearchBox= new BCheckBox(BRect(16+findWidth,backBoxTop,frame.Width()-12,backBoxBottom),"","Search backwards", NULL));
	
	fFindView->AddChild(fCancelButton= new BButton(BRect(142,buttonTop,212,frame.Height()-7),"","Cancel",new BMessage(B_QUIT_REQUESTED)));
	fFindView->AddChild(fSearchButton= new BButton(BRect(221,buttonTop,291,frame.Height()-7),"","Find",new BMessage(MSG_SEARCH)));
	
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


