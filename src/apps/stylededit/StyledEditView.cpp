#include <Message.h>
#include <Messenger.h>
#include <Rect.h>
#include <Region.h>
#include <TranslationUtils.h>
#include <Node.h>
#include <stdio.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <UTF8.h>

#include <StyledEditView.h>
#include <Constants.h>

using namespace BPrivate;

StyledEditView::StyledEditView(BRect viewFrame, BRect textBounds, BHandler *handler)
	: BTextView(viewFrame, "textview", textBounds, 
			    B_FOLLOW_ALL, B_FRAME_EVENTS|B_WILL_DRAW)
{ 
	fHandler= handler;
	fMessenger= new BMessenger(handler);
	fSuppressChanges = false;
	fEncoding = 0;
}

StyledEditView::~StyledEditView()
{
	delete fMessenger;
}
	
void
StyledEditView::FrameResized(float width, float height)
{
	BTextView::FrameResized(width, height);
	
	if (DoesWordWrap()) {
		BRect textRect;
		textRect = Bounds();
		textRect.OffsetTo(B_ORIGIN);
		textRect.InsetBy(TEXT_INSET,TEXT_INSET);
		SetTextRect(textRect);
	}
/*	// I tried to do some sort of intelligent resize thing but it just doesn't work
	// so we revert to the R5 stylededit yucky practice of setting the text rect to
	// some crazy large number when word wrap is turned off :-(
	 else if (textRect.Width() > TextRect().Width()) {
		SetTextRect(textRect);
	}

	BRegion region;
	GetTextRegion(0,TextLength(),&region);
	float textWidth = region.Frame().Width();
	if (textWidth < textRect.Width()) {
		BRect textRect(B_ORIGIN,BPoint(textWidth+TEXT_INSET*2,Bounds().Height()));
		textRect.InsetBy(TEXT_INSET,TEXT_INSET);
		SetTextRect(textRect);
	}
	*/
}				

status_t
StyledEditView::GetStyledText(BPositionIO * stream)
{
	status_t result = B_OK;

	fSuppressChanges = true;
	result = BTranslationUtils::GetStyledText(stream, this, NULL);
	fSuppressChanges = false;
	if (result != B_OK) {
		return result;
	}

	BNode * node = dynamic_cast<BNode*>(stream);
	if (node != 0) {
		ssize_t bytesRead;
		// decode encoding
		int32 encoding;
		bytesRead = node->ReadAttr("be:encoding",0,0,&encoding,sizeof(encoding));
		if (bytesRead > 0) {
			if (encoding == 65535) {
				fEncoding = 0;
			} else {
				const BCharacterSet * cs = BCharacterSetRoster::GetCharacterSetByConversionID(encoding);
				if (cs != 0) {
					fEncoding = cs->GetFontID();			
				}
			}
		}
		
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
		if (wrap == false) {
			BRect textRect;
			textRect = Bounds();
			textRect.OffsetTo(B_ORIGIN);
			textRect.InsetBy(TEXT_INSET,TEXT_INSET);
				// the width comes from stylededit R5. TODO: find a better way
			textRect.SetRightBottom(BPoint(1500.0,textRect.RightBottom().y));
			SetTextRect(textRect);
		}
	}
	if (fEncoding != 0) {
		int32 length = stream->Seek(0,SEEK_END);
		text_run_array * run_array = RunArray(0,length);
		uint32 id = BCharacterSetRoster::GetCharacterSetByFontID(fEncoding)->GetConversionID();
		fSuppressChanges = true;
		SetText("");
		fSuppressChanges = false;
		char inBuffer[32768];
		off_t location = 0;
		int32 textOffset = 0;
		int32 state = 0;
		int32 bytesRead;
		while ((bytesRead = stream->ReadAt(location,inBuffer,32768)) > 0) {
			char * inPtr = inBuffer;
			char textBuffer[32768];
			int32 textLength = 32768;
			int32 bytes = bytesRead;
			while ((textLength > 0) && (bytes > 0)) {
				result = convert_to_utf8(id,inPtr,&bytes,textBuffer,&textLength,&state);
				if (result != B_OK) {
					return result;
				}
				fSuppressChanges = true;
				InsertText(textBuffer,textLength,textOffset);
				fSuppressChanges = false;
				textOffset += textLength;
				inPtr += bytes;
				location += bytes;
				bytesRead -= bytes;
				bytes = bytesRead;
				if (textLength > 0) {
					textLength = 32768;
				}
			}
		}
		SetRunArray(0,length,run_array);
	}
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
	if (fEncoding == 0) {
		int32 encoding = 65535;
		file->WriteAttr("be:encoding",B_INT32_TYPE,0,&encoding,sizeof(encoding));
	} else {
		result = file->SetSize(0);
		if (result != B_OK) {
			return result;
		}
		result = file->Seek(0,SEEK_SET);
		if (result != B_OK) {
			return result;
		}
		const BCharacterSet * cs = BCharacterSetRoster::GetCharacterSetByFontID(fEncoding);
		if (cs != 0) {
			uint32 id = cs->GetConversionID();
			const char * outText = Text();
			int32 sourceLength = TextLength();
			int32 state = 0;
			char buffer[32768];
			while (sourceLength > 0) {
				int32 length = sourceLength;
				int32 written = 32768;
				result = convert_from_utf8(id,outText,&length,buffer,&written,&state);
				if (result != B_OK) {
					return result;
				}
				file->Write(buffer,written);
				sourceLength -= length;
				outText += length;
			}
			file->WriteAttr("be:encoding",B_INT32_TYPE,0,&id,sizeof(id));
		}
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

void
StyledEditView::SetEncoding(uint32 encoding)
{
	fEncoding = encoding;
}

uint32
StyledEditView::GetEncoding() const
{
	return fEncoding;
}

void
StyledEditView::InsertText(const char *text, int32 length, int32 offset, const text_run_array *runs)
{
	if (!fSuppressChanges)
		fMessenger-> SendMessage(new BMessage(TEXT_CHANGED));
	
	BTextView::InsertText(text, length, offset, runs);
	
}/****StyledEditView::InsertText()***/

void
StyledEditView::DeleteText(int32 start, int32 finish)
{
	if (!fSuppressChanges)
		fMessenger-> SendMessage(new BMessage(TEXT_CHANGED));
	
	BTextView::DeleteText(start, finish);

}/***StyledEditView::DeleteText***/
