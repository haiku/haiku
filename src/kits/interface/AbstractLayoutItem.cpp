/*
 * Copyright 2010, Haiku, Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <AbstractLayoutItem.h>

#include <LayoutUtils.h>
#include <Message.h>


namespace {
	const char* const kSizesField = "BAbstractLayoutItem:sizes";
		// kSizesField == {min, max, preferred}
	const char* const kAlignmentField = "BAbstractLayoutItem:alignment";
}


BAbstractLayoutItem::BAbstractLayoutItem()
	:
	fMinSize(),
	fMaxSize(),
	fPreferredSize(),
	fAlignment()
{
}


BAbstractLayoutItem::BAbstractLayoutItem(BMessage* from)
	:
	BLayoutItem(from),
	fMinSize(),
	fMaxSize(),
	fPreferredSize(),
	fAlignment()
{
	from->FindSize(kSizesField, 0, &fMinSize);
	from->FindSize(kSizesField, 1, &fMaxSize);
	from->FindSize(kSizesField, 2, &fPreferredSize);
	from->FindAlignment(kAlignmentField, &fAlignment);
}


BAbstractLayoutItem::~BAbstractLayoutItem()
{
}


BSize
BAbstractLayoutItem::MinSize()
{
	return BLayoutUtils::ComposeSize(fMinSize, BaseMinSize());
}


BSize
BAbstractLayoutItem::MaxSize()
{
	return BLayoutUtils::ComposeSize(fMaxSize, BaseMaxSize());
}


BSize
BAbstractLayoutItem::PreferredSize()
{
	return BLayoutUtils::ComposeSize(fMaxSize, BasePreferredSize());
}


BAlignment
BAbstractLayoutItem::Alignment()
{
	return BLayoutUtils::ComposeAlignment(fAlignment, BaseAlignment());
}


void
BAbstractLayoutItem::SetExplicitMinSize(BSize size)
{
	fMinSize = size;
}


void
BAbstractLayoutItem::SetExplicitMaxSize(BSize size)
{
	fMaxSize = size;
}


void
BAbstractLayoutItem::SetExplicitPreferredSize(BSize size)
{
	fPreferredSize = size;
}


void
BAbstractLayoutItem::SetExplicitAlignment(BAlignment alignment)
{
	fAlignment = alignment;
}


BSize
BAbstractLayoutItem::BaseMinSize()
{
	return BSize(0, 0);
}


BSize
BAbstractLayoutItem::BaseMaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


BSize
BAbstractLayoutItem::BasePreferredSize()
{
	return BSize(0, 0);
}


BAlignment
BAbstractLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER);
}


status_t
BAbstractLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BLayoutItem::Archive(into, deep);

	if (err == B_OK)
		err = into->AddSize(kSizesField, fMinSize);

	if (err == B_OK)
		err = into->AddSize(kSizesField, fMaxSize);

	if (err == B_OK)
		err = into->AddSize(kSizesField, fPreferredSize);

	if (err == B_OK)
		err = into->AddAlignment(kAlignmentField, fAlignment);

	return archiver.Finish(err);
}


status_t
BAbstractLayoutItem::Perform(perform_code d, void* arg)
{
	return BLayoutItem::Perform(d, arg);
}


void BAbstractLayoutItem::_ReservedAbstractLayoutItem1() {}
void BAbstractLayoutItem::_ReservedAbstractLayoutItem2() {}
void BAbstractLayoutItem::_ReservedAbstractLayoutItem3() {}
void BAbstractLayoutItem::_ReservedAbstractLayoutItem4() {}
void BAbstractLayoutItem::_ReservedAbstractLayoutItem5() {}
void BAbstractLayoutItem::_ReservedAbstractLayoutItem6() {}
void BAbstractLayoutItem::_ReservedAbstractLayoutItem7() {}
void BAbstractLayoutItem::_ReservedAbstractLayoutItem8() {}
void BAbstractLayoutItem::_ReservedAbstractLayoutItem9() {}
void BAbstractLayoutItem::_ReservedAbstractLayoutItem10() {}

