#include <Message.h>
#include <Messenger.h>
#include <Rect.h>
#include <TranslationUtils.h>
#include <Node.h>

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
	
	BRect textRect;
	textRect = Bounds();
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(TEXT_INSET,TEXT_INSET);
	if (DoesWordWrap() || (textRect.Width() > TextRect().Width())) {
		SetTextRect(textRect);
	}
}				

status_t
StyledEditView::GetStyledText(BPositionIO * stream)
{
	status_t result = B_OK;
	fSuppressChanges = true;	
	result = BTranslationUtils::GetStyledText(stream, this, NULL);
	
	BNode * node = dynamic_cast<BNode*>(stream);
	if (node != 0) {
		ssize_t bytesRead;
		// restore alignment
		alignment align;
		bytesRead = node->ReadAttr("alignment",0,0,&align,sizeof(align));
		if (bytesRead > 0) {
			SetAlignment(align);
		}
		// restore wrapping
		bool wrap;
		bytesRead = node->ReadAttr("wrap",0,0,&wrap,sizeof(wrap));
		if (bytesRead > 0) {
			SetWordWrap(wrap);
		}
	}
	
	fSuppressChanges = false;
	return result;
}

status_t
StyledEditView::WriteStyledEditFile(BFile * file)
{
	status_t result = B_OK;
	result = BTranslationUtils::WriteStyledEditFile(this,file);
	if (result != B_OK) {
		return result;
	}
	alignment align = Alignment();
	file->WriteAttr("alignment",B_INT32_TYPE,0,&align,sizeof(align));
	bool wrap = DoesWordWrap();
	file->WriteAttr("wrap",B_BOOL_TYPE,0,&wrap,sizeof(wrap));
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
