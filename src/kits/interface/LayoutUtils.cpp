/*
 * Copyright 2006-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */

#include <LayoutUtils.h>

#include <algorithm>
#include <typeinfo>

#include <Layout.h>
#include <View.h>

#include "ViewLayoutItem.h"


// // AddSizesFloat
// float
// BLayoutUtils::AddSizesFloat(float a, float b)
// {
// 	float sum = a + b + 1;
// 	if (sum >= B_SIZE_UNLIMITED)
// 		return B_SIZE_UNLIMITED;
//
// 	return sum;
// }
//
// // AddSizesFloat
// float
// BLayoutUtils::AddSizesFloat(float a, float b, float c)
// {
// 	return AddSizesFloat(AddSizesFloat(a, b), c);
// }


// AddSizesInt32
int32
BLayoutUtils::AddSizesInt32(int32 a, int32 b)
{
	if (a >= B_SIZE_UNLIMITED - b)
		return B_SIZE_UNLIMITED;
	return a + b;
}


// AddSizesInt32
int32
BLayoutUtils::AddSizesInt32(int32 a, int32 b, int32 c)
{
	return AddSizesInt32(AddSizesInt32(a, b), c);
}


// AddDistances
float
BLayoutUtils::AddDistances(float a, float b)
{
	float sum = a + b + 1;
	if (sum >= B_SIZE_UNLIMITED)
		return B_SIZE_UNLIMITED;

	return sum;
}


// AddDistances
float
BLayoutUtils::AddDistances(float a, float b, float c)
{
	return AddDistances(AddDistances(a, b), c);
}


// // SubtractSizesFloat
// float
// BLayoutUtils::SubtractSizesFloat(float a, float b)
// {
// 	if (a < b)
// 		return -1;
// 	return a - b - 1;
// }


// SubtractSizesInt32
int32
BLayoutUtils::SubtractSizesInt32(int32 a, int32 b)
{
	if (a < b)
		return 0;
	return a - b;
}


// SubtractDistances
float
BLayoutUtils::SubtractDistances(float a, float b)
{
	if (a < b)
		return -1;
	return a - b - 1;
}


// FixSizeConstraints
void
BLayoutUtils::FixSizeConstraints(float& min, float& max, float& preferred)
{
	if (max < min)
		max = min;
	if (preferred < min)
		preferred = min;
	else if (preferred > max)
		preferred = max;
}


// FixSizeConstraints
void
BLayoutUtils::FixSizeConstraints(BSize& min, BSize& max, BSize& preferred)
{
	FixSizeConstraints(min.width, max.width, preferred.width);
	FixSizeConstraints(min.height, max.height, preferred.height);
}


// ComposeSize
BSize
BLayoutUtils::ComposeSize(BSize size, BSize layoutSize)
{
	if (!size.IsWidthSet())
		size.width = layoutSize.width;
	if (!size.IsHeightSet())
		size.height = layoutSize.height;

	return size;
}


// ComposeAlignment
BAlignment
BLayoutUtils::ComposeAlignment(BAlignment alignment, BAlignment layoutAlignment)
{
	if (!alignment.IsHorizontalSet())
		alignment.horizontal = layoutAlignment.horizontal;
	if (!alignment.IsVerticalSet())
		alignment.vertical = layoutAlignment.vertical;

	return alignment;
}


// AlignInFrame
// This method restricts the dimensions of the resulting rectangle according
// to the available size specified by maxSize.
BRect
BLayoutUtils::AlignInFrame(BRect frame, BSize maxSize, BAlignment alignment)
{
	// align according to the given alignment
	if (maxSize.width < frame.Width()
		&& alignment.horizontal != B_ALIGN_USE_FULL_WIDTH) {
		frame.left += (int)((frame.Width() - maxSize.width)
			* alignment.RelativeHorizontal());
		frame.right = frame.left + maxSize.width;
	}
	if (maxSize.height < frame.Height()
		&& alignment.vertical != B_ALIGN_USE_FULL_HEIGHT) {
		frame.top += (int)((frame.Height() - maxSize.height)
			* alignment.RelativeVertical());
		frame.bottom = frame.top + maxSize.height;
	}

	return frame;
}


// AlignInFrame
void
BLayoutUtils::AlignInFrame(BView* view, BRect frame)
{
	BSize maxSize = view->MaxSize();
	BAlignment alignment = view->LayoutAlignment();
	if (view->HasHeightForWidth()) {
		// The view has height for width, so we do the horizontal alignment
		// ourselves and restrict the height max constraint respectively.
		if (maxSize.width < frame.Width()
			&& alignment.horizontal != B_ALIGN_USE_FULL_WIDTH) {
			frame.OffsetBy(floorf((frame.Width() - maxSize.width)
				* alignment.RelativeHorizontal()), 0);
			frame.right = frame.left + maxSize.width;
		}
		alignment.horizontal = B_ALIGN_USE_FULL_WIDTH;
		float minHeight;
		float maxHeight;
		float preferredHeight;
		view->GetHeightForWidth(frame.Width(), &minHeight, &maxHeight,
			&preferredHeight);
		frame.bottom = frame.top + std::max(frame.Height(), minHeight);
		maxSize.height = minHeight;
	}
	frame = AlignInFrame(frame, maxSize, alignment);
	view->MoveTo(frame.LeftTop());
	view->ResizeTo(frame.Size());
}


// AlignOnRect
// This method, unlike AlignInFrame(), provides the possibility to return
// a rectangle with dimensions greater than the available size.
BRect
BLayoutUtils::AlignOnRect(BRect rect, BSize size, BAlignment alignment)
{
	rect.left += (int)((rect.Width() - size.width)
		* alignment.RelativeHorizontal());
	rect.top += (int)(((rect.Height() - size.height))
		* alignment.RelativeVertical());
	rect.right = rect.left + size.width;
	rect.bottom = rect.top + size.height;

	return rect;
}


/*!	Offsets a rectangle's location so that it lies fully in a given rectangular
	frame.

	If the rectangle is too wide/high to fully fit in the frame, its left/top
	edge is offset to 0. The rect's size always remains unchanged.

	\param rect The rectangle to be moved.
	\param frameSize The size of the frame the rect shall be moved into. The
		frame's left-top is (0, 0).
	\return The modified rect.
*/
/*static*/ BRect
BLayoutUtils::MoveIntoFrame(BRect rect, BSize frameSize)
{
	BPoint leftTop(rect.LeftTop());

	// enforce horizontal limits; favor left edge
	if (rect.right > frameSize.width)
		leftTop.x -= rect.right - frameSize.width;
	if (leftTop.x < 0)
		leftTop.x = 0;

	// enforce vertical limits; favor top edge
	if (rect.bottom > frameSize.height)
		leftTop.y -= rect.bottom - frameSize.height;
	if (leftTop.y < 0)
		leftTop.y = 0;

	return rect.OffsetToSelf(leftTop);
}


/*static*/ BString
BLayoutUtils::GetLayoutTreeDump(BView* view)
{
	BString result;
	_GetLayoutTreeDump(view, 0, result);
	return result;
}


/*static*/ BString
BLayoutUtils::GetLayoutTreeDump(BLayoutItem* item)
{
	BString result;
	_GetLayoutTreeDump(item, 0, false, result);
	return result;
}


/*static*/ void
BLayoutUtils::_GetLayoutTreeDump(BView* view, int level, BString& _output)
{
	BString indent;
	indent.SetTo(' ', level * 2);

	if (view == NULL) {
		_output << indent << "<null view>\n";
		return;
	}

	BRect frame = view->Frame();
	BSize min = view->MinSize();
	BSize max = view->MinSize();
	BSize preferred = view->PreferredSize();
	_output << BString().SetToFormat(
		"%sview %p (%s):\n"
		"%s  frame: (%f, %f, %f, %f)\n"
		"%s  min:   (%f, %f)\n"
		"%s  max:   (%f, %f)\n"
		"%s  pref:  (%f, %f)\n",
		indent.String(), view, typeid(*view).name(),
		indent.String(), frame.left, frame.top, frame.right, frame.bottom,
		indent.String(), min.width, min.height,
		indent.String(), max.width, max.height,
		indent.String(), preferred.width, preferred.height);

	if (BLayout* layout = view->GetLayout()) {
		_GetLayoutTreeDump(layout, level, true, _output);
		return;
	}

	int32 count = view->CountChildren();
	for (int32 i = 0; i < count; i++) {
		_output << indent << "  ---\n";
		_GetLayoutTreeDump(view->ChildAt(i), level + 1, _output);
	}
}


/*static*/ void
BLayoutUtils::_GetLayoutTreeDump(BLayoutItem* item, int level,
	bool isViewLayout, BString& _output)
{
	if (BViewLayoutItem* viewItem = dynamic_cast<BViewLayoutItem*>(item)) {
		_GetLayoutTreeDump(viewItem->View(), level, _output);
		return;
	}

	BString indent;
	indent.SetTo(' ', level * 2);

	if (item == NULL) {
		_output << indent << "<null item>\n";
		return;
	}

	BLayout* layout = dynamic_cast<BLayout*>(item);
	BRect frame = item->Frame();
	BSize min = item->MinSize();
	BSize max = item->MinSize();
	BSize preferred = item->PreferredSize();
	if (isViewLayout) {
		_output << indent << BString().SetToFormat("  [layout %p (%s)]\n",
			layout, typeid(*layout).name());
	} else {
		_output << indent << BString().SetToFormat("item %p (%s):\n",
			item, typeid(*item).name());
	}
	_output << BString().SetToFormat(
		"%s  frame: (%f, %f, %f, %f)\n"
		"%s  min:   (%f, %f)\n"
		"%s  max:   (%f, %f)\n"
		"%s  pref:  (%f, %f)\n",
		indent.String(), frame.left, frame.top, frame.right, frame.bottom,
		indent.String(), min.width, min.height,
		indent.String(), max.width, max.height,
		indent.String(), preferred.width, preferred.height);

	if (layout == NULL)
		return;

	int32 count = layout->CountItems();
	for (int32 i = 0; i < count; i++) {
		_output << indent << "  ---\n";
		_GetLayoutTreeDump(layout->ItemAt(i), level + 1, false, _output);
	}
}
