#ifndef STYLED_EDIT_VIEW_H
#include "StyledEditView.h"
#endif

#ifndef _MESSAGE_H
#include <Message.h>
#endif

#ifndef CONSTANTS_H
#include "Constants.h"
#endif

#ifndef _MESSENGER_H
#include <Messenger.h>
#endif

#ifndef _RECT_H
#include <Rect.h>
#endif



StyledEditView::StyledEditView(BRect viewFrame, BRect textBounds, BHandler *handler) : BTextView(viewFrame, "textview",
	textBounds, B_FOLLOW_ALL, B_FRAME_EVENTS|B_WILL_DRAW){ 
		fHandler= handler;
		fMessenger= new BMessenger(handler);
	}/***StyledEditView()***/

	StyledEditView::~StyledEditView(){

	}/***~StyledEditView***/
	
void StyledEditView::FrameResized(float width, float height){
	BTextView::FrameResized(width, height);
	
	if(DoesWordWrap()){
			BRect textRect;
			textRect= Bounds();
			textRect.OffsetTo(B_ORIGIN);
			textRect.InsetBy(TEXT_INSET,TEXT_INSET);
			SetTextRect(textRect);
	}
}				

void StyledEditView::Select(int32 start, int32 finish){
	
	if(start==finish)
		fChangeMessage= new BMessage(DISABLE_ITEMS);
	else
		fChangeMessage= new BMessage(ENABLE_ITEMS);
	
	fMessenger->SendMessage(fChangeMessage);
	
	BTextView::Select(start, finish);
}

void StyledEditView::InsertText(const char *text, int32 length, int32 offset, const text_run_array *runs){
	
	fMessenger-> SendMessage(new BMessage(TEXT_CHANGED));
	
	BTextView::InsertText(text, length, offset, runs);
	
}/****StyledEditView::InsertText()***/

void StyledEditView::DeleteText(int32 start, int32 finish){
	
	fMessenger-> SendMessage(new BMessage(TEXT_CHANGED));
	
	BTextView::DeleteText(start, finish);

}/***StyledEditView::DeleteText***/






		
		

	
		
	
	
	
	
	
	





