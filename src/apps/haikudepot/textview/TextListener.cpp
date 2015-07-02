/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextListener.h"


// #pragma mark - TextChangeEvent


TextChangeEvent::TextChangeEvent(int32 firstChangedParagraph,
		int32 changedParagraphCount)
	:
	fFirstChangedParagraph(firstChangedParagraph),
	fChangedParagraphCount(changedParagraphCount)
{
}


TextChangeEvent::TextChangeEvent()
	:
	fFirstChangedParagraph(0),
	fChangedParagraphCount(0)
{
}


TextChangeEvent::~TextChangeEvent()
{
}


// #pragma mark - TextChangedEvent


TextChangingEvent::TextChangingEvent(int32 firstChangedParagraph,
		int32 changedParagraphCount)
	:
	TextChangeEvent(firstChangedParagraph, changedParagraphCount),
	fIsCanceled(false)
{
}


TextChangingEvent::TextChangingEvent()
	:
	TextChangeEvent(),
	fIsCanceled(false)
{
}


TextChangingEvent::~TextChangingEvent()
{
}


void
TextChangingEvent::Cancel()
{
	fIsCanceled = true;
}


// #pragma mark - TextChangedEvent


TextChangedEvent::TextChangedEvent(int32 firstChangedParagraph,
		int32 changedParagraphCount)
	:
	TextChangeEvent(firstChangedParagraph, changedParagraphCount)
{
}


TextChangedEvent::TextChangedEvent()
	:
	TextChangeEvent()
{
}


TextChangedEvent::~TextChangedEvent()
{
}


// #pragma mark - TextListener


TextListener::TextListener()
	:
	BReferenceable()
{
}


TextListener::~TextListener()
{
}


void
TextListener::TextChanging(TextChangingEvent& event)
{
}


void
TextListener::TextChanged(const TextChangedEvent& event)
{
}
