#ifndef REPLACE_WINDOW_H 
#define REPLACE_WINDOW_H 

#include <Window.h>
#include <Rect.h>
#include <Handler.h>
#include <String.h>
#include <Message.h>
#include <View.h>
#include <TextControl.h>
#include <CheckBox.h>
#include <Button.h>

class ReplaceWindow : public BWindow {
	public:
				ReplaceWindow(BRect frame, BHandler *_handler,BString *searchString,
					BString *replaceString, bool *caseState, bool *wrapState, bool *backState);
	
	virtual void MessageReceived(BMessage* message);
	virtual void DispatchMessage(BMessage* message, BHandler *handler);
			
			
	private:
	void	ExtractToMsg(BMessage *message);
	void	ChangeUi();		
			BView 			*fReplaceView;
			BTextControl	*fSearchString;
			BTextControl	*fReplaceString;
			BCheckBox		*fCaseSensBox;
			BCheckBox		*fWrapBox;
			BCheckBox		*fBackSearchBox;
			BCheckBox		*fAllWindowsBox;
			BButton			*fReplaceButton;
			BButton			*fReplaceAllButton;
			BButton			*fCancelButton;
			BHandler		*fHandler;
			bool			fUichange;
};

#endif

