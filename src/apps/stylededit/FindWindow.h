#ifndef FIND_WINDOW_H 
#define FIND_WINDOW_H 

class FindWindow : public BWindow {
	public:
				FindWindow(BRect frame, BHandler* handler, BString *searchString, bool *caseState, bool *wrapState, bool *backState);
	
	virtual void MessageReceived(BMessage* message);
	virtual void DispatchMessage(BMessage* message, BHandler* handler);
			
	private:
	void	ExtractToMsg(BMessage *message);
			
			BView 			*fFindView;
			BTextControl	*fSearchString;
			BCheckBox		*fCaseSensBox;
			BCheckBox		*fWrapBox;
			BCheckBox		*fBackSearchBox;
			BButton			*fCancelButton;
			BButton			*fSearchButton;
		
			BHandler		*fHandler;
								
};
#endif






