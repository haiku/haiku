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

#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <DataIO.h>
#include <File.h>
#include <Message.h>
#include <Messenger.h>
#include <Node.h>
#include <Rect.h>
#include <TranslationUtils.h>
#include <UTF8.h>

#include <stdio.h>
#include <stdlib.h>


using namespace BPrivate;


StyledEditView::StyledEditView(BRect viewFrame, BRect textBounds,
	BHandler* handler)
	: BTextView(viewFrame, "textview", textBounds, 
		B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW)
{ 
	fMessenger = new BMessenger(handler);
	fSuppressChanges = false;
}


StyledEditView::~StyledEditView()
{
	delete fMessenger;
}


void
StyledEditView::Select(int32 start, int32 finish)
{
	fMessenger->SendMessage(start == finish ? DISABLE_ITEMS : ENABLE_ITEMS);
	fMessenger->SendMessage(UPDATE_LINE);
	BTextView::Select(start, finish);	
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
StyledEditView::SetSuppressChanges(bool suppressChanges)
{
	fSuppressChanges = suppressChanges;
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
StyledEditView::SetEncoding(uint32 encoding)
{
	fEncoding = "";
	if (encoding == 0)
		return;

	const BCharacterSet* set 
		= BCharacterSetRoster::GetCharacterSetByFontID(encoding);

	if (set != NULL)
		fEncoding = set->GetName();
}


uint32
StyledEditView::GetEncoding() const
{
	if (fEncoding == "")
		return 0;

	const BCharacterSet* set = 
		BCharacterSetRoster::FindCharacterSetByName(fEncoding.String());
	if (set != NULL)
		return set->GetFontID();

	return 0;
}


void
StyledEditView::DeleteText(int32 start, int32 finish)
{
	if (!fSuppressChanges)
		fMessenger-> SendMessage(TEXT_CHANGED);

	BTextView::DeleteText(start, finish);
	fMessenger->SendMessage(UPDATE_LINE);
}


void
StyledEditView::InsertText(const char* text, int32 length, int32 offset,
	const text_run_array* runs)
{
	if (!fSuppressChanges)
		fMessenger->SendMessage(TEXT_CHANGED);

	BTextView::InsertText(text, length, offset, runs);
	fMessenger->SendMessage(UPDATE_LINE);
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
}				

