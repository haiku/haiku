
#ifndef CONSTANTS_H
#include "Constants.h"
#endif

#ifndef STYLED_EDIT_APP
#include "StyledEditApp.h"
#endif


BRect windowRect(50,50,599,399);

StyledEditApp::StyledEditApp()
	: BApplication(APP_SIGNATURE)
	{
	
	new StyledEditWindow(windowRect);
	fOpenPanel= new BFilePanel;
	
	fWindowCount= 0;
	fNext_Untitled_Window= 1;
	} /***StyledEditApp::StyledEditApp()***/


void StyledEditApp::MessageReceived(BMessage *message){
	switch(message->what){
		case MENU_OPEN:
			fOpenPanel->Show(); //
		break;
		case WINDOW_REGISTRY_ADD:
			{
			bool need_id= false;
			BMessage reply(WINDOW_REGISTRY_ADDED);
			
			if(message->FindBool("need_id", &need_id)== B_OK)
				{
				if(need_id)
					{
					reply.AddInt32("new_window_number", fNext_Untitled_Window);
					fNext_Untitled_Window++;
					}
				fWindowCount++;
				}
			reply.AddRect("rect",windowRect);
			windowRect.OffsetBy(20,20);
			message->SendReply(&reply);
			break;
			}
		case WINDOW_REGISTRY_SUB:
			fWindowCount--;
			if (!fWindowCount) 
			{
				Quit();
			}
			break;	 	
		default:
			BApplication::MessageReceived(message);
		break;
	} 
}

void StyledEditApp::RefsReceived(BMessage *message){
	
	int32		refNum;
	entry_ref	ref;
	status_t	err;
	
	refNum=0;
	do{
		if((err= message->FindRef("refs", refNum, &ref)) != B_OK)
			return;
		new StyledEditWindow(windowRect, &ref);
		refNum++;
	} while(1);		
} /***StyledEditApp::RefsReceived();***/

int32 StyledEditApp::NumberOfWindows(){
 	
 	return fWindowCount;

}/***StyledEditApp::NumberOfWindows()***/

int main(){
	StyledEditApp	StyledEdit;
	StyledEdit.Run();
	return 0;
	}
	
	
	
	