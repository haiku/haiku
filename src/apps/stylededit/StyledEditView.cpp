/*
 * Copyright 2002-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "Constants.h"
#include "StyledEditView.h"

#include <Message.h>
#include <Messenger.h>
#include <Rect.h>
#include <Region.h>
#include <TranslationUtils.h>
#include <Node.h>
#include <stdio.h>
#include <stdlib.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <UTF8.h>


using namespace BPrivate;


StyledEditView::StyledEditView(BRect viewFrame, BRect textBounds, BHandler *handler)
	: BTextView(viewFrame, "textview", textBounds, 
		B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW)
{ 
	fHandler = handler;
	fMessenger = new BMessenger(handler);
	fSuppressChanges = false;
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
		textRect.InsetBy(TEXT_INSET, TEXT_INSET);
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
StyledEditView::GetStyledText(BPositionIO* stream)
{
	fSuppressChanges = true;
	status_t result = BTranslationUtils::GetStyledText(stream, this,
		fEncoding.String());
	fSuppressChanges = false;

	if (result != B_OK)
		return result;

	BNode* node = dynamic_cast<BNode*>(stream);
	if (node != NULL) {
		// get encoding
		if (node->ReadAttrString("be:encoding", &fEncoding) != B_OK) {
			// try to read as "int32"
			int32 encoding;
			ssize_t bytesRead = node->ReadAttr("be:encoding", B_INT32_TYPE, 0,
				&encoding, sizeof(encoding));
			if (bytesRead == (ssize_t)sizeof(encoding)) {
				if (encoding == 65535) {
					fEncoding = "UTF-8";
				} else {
					const BCharacterSet* characterSet
						= BCharacterSetRoster::GetCharacterSetByConversionID(encoding);
					if (characterSet != NULL)
						fEncoding = characterSet->GetName();
				}
			}
		}

		// TODO: move those into BTranslationUtils::GetStyledText() as well?

		// restore alignment
		int32 align;
		ssize_t bytesRead = node->ReadAttr("alignment", 0, 0, &align, sizeof(align));
		if (bytesRead == (ssize_t)sizeof(align))
			SetAlignment((alignment)align);

		// restore wrapping
		bool wrap;
		bytesRead = node->ReadAttr("wrap", 0, 0, &wrap, sizeof(wrap));
		if (bytesRead == (ssize_t)sizeof(wrap)) {
			SetWordWrap(wrap);
			if (wrap == false) {
				BRect textRect;
				textRect = Bounds();
				textRect.OffsetTo(B_ORIGIN);
				textRect.InsetBy(TEXT_INSET, TEXT_INSET);
					// the width comes from stylededit R5. TODO: find a better way
				textRect.SetRightBottom(BPoint(1500.0, textRect.RightBottom().y));
				SetTextRect(textRect);
			}
		}
	}

	return result;
}


status_t
StyledEditView::WriteStyledEditFile(BFile* file)
{
	return BTranslationUtils::WriteStyledEditFile(this, file,
		fEncoding.String());
}


void
StyledEditView::Reset()
{
	fSuppressChanges = true;
	SetText("");
	fEncoding = "";
	fSuppressChanges = false;
}


void
StyledEditView::Select(int32 start, int32 finish)
{
	fChangeMessage = new BMessage(start == finish ? DISABLE_ITEMS : ENABLE_ITEMS);
	fMessenger->SendMessage(fChangeMessage);

	BTextView::Select(start, finish);
}


void
StyledEditView::SetEncoding(uint32 encoding)
{
	if (encoding == 0) {
		fEncoding = "";
		return;
	}

	const BCharacterSet* set = BCharacterSetRoster::GetCharacterSetByFontID(encoding);
	if (set != NULL)
		fEncoding = set->GetName();
	else
		fEncoding = "";
}


uint32
StyledEditView::GetEncoding() const
{
	if (fEncoding == "")
		return 0;

	const BCharacterSet* set = BCharacterSetRoster::FindCharacterSetByName(fEncoding.String());
	if (set != NULL)
		return set->GetFontID();

	return 0;
}


void
StyledEditView::InsertText(const char *text, int32 length, int32 offset,
	const text_run_array *runs)
{
	if (!fSuppressChanges)
		fMessenger->SendMessage(new BMessage(TEXT_CHANGED));

	BTextView::InsertText(text, length, offset, runs);	
}


void
StyledEditView::DeleteText(int32 start, int32 finish)
{
	if (!fSuppressChanges)
		fMessenger-> SendMessage(new BMessage(TEXT_CHANGED));

	BTextView::DeleteText(start, finish);
}

