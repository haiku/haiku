#include <Message.h>
#include <Messenger.h>
#include <Rect.h>
#include <TranslationUtils.h>

#include "StyledEditView.h"
#include "Constants.h"

StyledEditView::StyledEditView(BRect viewFrame, BRect textBounds, BHandler *handler)
	: BTextView(viewFrame, "textview", textBounds, 
			    B_FOLLOW_ALL, B_FRAME_EVENTS|B_WILL_DRAW)
{ 
	fHandler= handler;
	fMessenger= new BMessenger(handler);
	fSuppressChanges = false;
}/***StyledEditView()***/

StyledEditView::~StyledEditView(){

}/***~StyledEditView***/
	
void
StyledEditView::FrameResized(float width, float height)
{
	BTextView::FrameResized(width, height);
	
	if(DoesWordWrap()){
			BRect textRect;
			textRect= Bounds();
			textRect.OffsetTo(B_ORIGIN);
			textRect.InsetBy(TEXT_INSET,TEXT_INSET);
			SetTextRect(textRect);
	}
}				

status_t
StyledEditView::GetStyledText(BPositionIO * stream)
{
	status_t result = B_OK;
	fSuppressChanges = true;	
	result = BTranslationUtils::GetStyledText(stream, this, NULL);
	fSuppressChanges = false;
	return result;
}

void
StyledEditView::Reset()
{
	fSuppressChanges = true;
	SetText("");
	fSuppressChanges = false;
}

void
StyledEditView::Select(int32 start, int32 finish)
{
	if(start==finish)
		fChangeMessage= new BMessage(DISABLE_ITEMS);
	else
		fChangeMessage= new BMessage(ENABLE_ITEMS);
	
	fMessenger->SendMessage(fChangeMessage);
	
	BTextView::Select(start, finish);
}

void StyledEditView::InsertText(const char *text, int32 length, int32 offset, const text_run_array *runs)
{
	if (!fSuppressChanges)
		fMessenger-> SendMessage(new BMessage(TEXT_CHANGED));
	
	BTextView::InsertText(text, length, offset, runs);
	
}/****StyledEditView::InsertText()***/

void StyledEditView::DeleteText(int32 start, int32 finish)
{
	if (!fSuppressChanges)
		fMessenger-> SendMessage(new BMessage(TEXT_CHANGED));
	
	BTextView::DeleteText(start, finish);

}/***StyledEditView::DeleteText***/
