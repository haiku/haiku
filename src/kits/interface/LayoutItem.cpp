/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <LayoutItem.h>

#include <Layout.h>
#include <LayoutUtils.h>


// constructor
BLayoutItem::BLayoutItem()
	: fLayout(NULL),
	  fLayoutData(NULL)
{
}

// destructor
BLayoutItem::~BLayoutItem()
{
}

// Layout
BLayout*
BLayoutItem::Layout() const
{
	return fLayout;
}

// HasHeightForWidth
bool
BLayoutItem::HasHeightForWidth()
{
	// no "height for width" by default
	return false;
}

// GetHeightForWidth
void
BLayoutItem::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	// no "height for width" by default
}

// View
BView*
BLayoutItem::View()
{
	return NULL;
}

// InvalidateLayout
void
BLayoutItem::InvalidateLayout()
{
	if (fLayout)
		fLayout->InvalidateLayout();
}

// LayoutData
void*
BLayoutItem::LayoutData() const
{
	return fLayoutData;
}

// SetLayoutData
void
BLayoutItem::SetLayoutData(void* data)
{
	fLayoutData = data;
}

// AlignInFrame
void
BLayoutItem::AlignInFrame(BRect frame)
{
	BSize maxSize = MaxSize();
	BAlignment alignment = Alignment();

	if (HasHeightForWidth()) {
		// The item has height for width, so we do the horizontal alignment
		// ourselves and restrict the height max constraint respectively.
		if (maxSize.width < frame.Width()
			&& alignment.horizontal != B_ALIGN_USE_FULL_WIDTH) {
			frame.left += (int)((frame.Width() - maxSize.width)
				* alignment.horizontal);
			frame.right = frame.left + maxSize.width;
		}
		alignment.horizontal = B_ALIGN_USE_FULL_WIDTH;

		float minHeight;
		GetHeightForWidth(frame.Width(), &minHeight, NULL, NULL);
		
		frame.bottom = frame.top + max_c(frame.Height(), minHeight);
		maxSize.height = minHeight;
	}

	SetFrame(BLayoutUtils::AlignInFrame(frame, maxSize, alignment));
}

// SetLayout
void
BLayoutItem::SetLayout(BLayout* layout)
{
	fLayout = layout;
}
