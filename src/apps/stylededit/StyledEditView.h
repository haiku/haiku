#ifndef STYLED_EDIT_VIEW_H
#define STYLED_EDIT_VIEW_H

#ifndef _TEXTVIEW_H
#include <TextView.h>
#endif

class StyledEditView : public BTextView {
	public:
		StyledEditView(BRect viewframe, BRect textframe, BHandler *handler);
		~StyledEditView();
	
		virtual void Select(int32 start, int32 finish);
		virtual void FrameResized(float width, float height);
	protected:
		virtual void InsertText(const char *text, int32 length, int32 offset, const text_run_array *runs);
		virtual void DeleteText(int32 start, int32 finish);
				
	private:
		BHandler	*fHandler;
		BMessage	*fChangeMessage;
		BMessenger 	*fMessenger;	
};
#endif







		


