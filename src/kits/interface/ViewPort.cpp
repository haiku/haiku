/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ViewPort.h>

#include <algorithm>

#include <AbstractLayout.h>
#include <ScrollBar.h>

#include "ViewLayoutItem.h"


namespace BPrivate {


// #pragma mark - ViewPortLayout


class BViewPort::ViewPortLayout : public BAbstractLayout {
public:
	ViewPortLayout(BViewPort* viewPort)
		:
		BAbstractLayout(),
		fViewPort(viewPort),
		fHasViewChild(false),
		fIsCacheValid(false),
		fMin(),
		fMax(),
		fPreferred()
	{
	}

	BView* ChildView() const
	{
		if (!fHasViewChild)
			return NULL;
		if (BViewLayoutItem* item = dynamic_cast<BViewLayoutItem*>(ItemAt(0)))
			return item->View();
		return NULL;
	}

	void SetChildView(BView* view)
	{
		_UnsetChild();

		if (view != NULL && AddView(0, view) != NULL)
			fHasViewChild = true;
	}

	BLayoutItem* ChildItem() const
	{
		return ItemAt(0);
	}

	void SetChildItem(BLayoutItem* item)
	{
		_UnsetChild();

		if (item != NULL)
			AddItem(0, item);
	}

	virtual BSize BaseMinSize()
	{
		_ValidateMinMax();
		return fMin;
	}

	virtual BSize BaseMaxSize()
	{
		_ValidateMinMax();
		return fMax;
	}

	virtual BSize BasePreferredSize()
	{
		_ValidateMinMax();
		return fPreferred;
	}

	virtual BAlignment BaseAlignment()
	{
		return BAbstractLayout::BaseAlignment();
	}

	virtual bool HasHeightForWidth()
	{
		_ValidateMinMax();
		return false;
		// TODO: Support height-for-width!
	}

	virtual void GetHeightForWidth(float width, float* min, float* max,
		float* preferred)
	{
		if (!HasHeightForWidth())
			return;

		// TODO: Support height-for-width!
	}

	virtual void LayoutInvalidated(bool children)
	{
		fIsCacheValid = false;
	}

	virtual void DoLayout()
	{
		_ValidateMinMax();

		BLayoutItem* child = ItemAt(0);
		if (child == NULL)
			return;

		// Determine the layout area: LayoutArea() will only give us the size
		// of the view port's frame.
		BSize viewSize = LayoutArea().Size();
		BSize layoutSize = viewSize;

		BSize childMin = child->MinSize();
		BSize childMax = child->MaxSize();

		// apply the maximum constraints
		layoutSize.width = std::min(layoutSize.width, childMax.width);
		layoutSize.height = std::min(layoutSize.height, childMax.height);

		// apply the minimum constraints
		layoutSize.width = std::max(layoutSize.width, childMin.width);
		layoutSize.height = std::max(layoutSize.height, childMin.height);

		// TODO: Support height-for-width!

		child->AlignInFrame(BRect(BPoint(0, 0), layoutSize));

		_UpdateScrollBar(fViewPort->ScrollBar(B_HORIZONTAL), viewSize.width,
			layoutSize.width);
		_UpdateScrollBar(fViewPort->ScrollBar(B_VERTICAL), viewSize.height,
			layoutSize.height);
	}

private:
	void _UnsetChild()
	{
		if (CountItems() > 0) {
			BLayoutItem* item = RemoveItem((int32)0);
			if (fHasViewChild)
				delete item;
			fHasViewChild = false;
		}
	}

	void _ValidateMinMax()
	{
		if (fIsCacheValid)
			return;

		if (BLayoutItem* child = ItemAt(0)) {
			fMin = child->MinSize();
			if (_IsHorizontallyScrollable())
				fMin.width = -1;
			if (_IsVerticallyScrollable())
				fMin.height = -1;
			fMax = child->MaxSize();
			fPreferred = child->PreferredSize();
			// TODO: Support height-for-width!
		} else {
			fMin.Set(-1, -1);
			fMax.Set(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
			fPreferred.Set(20, 20);
		}

		fIsCacheValid = true;
	}

	bool _IsHorizontallyScrollable() const
	{
		return fViewPort->ScrollBar(B_HORIZONTAL) != NULL;
	}

	bool _IsVerticallyScrollable() const
	{
		return fViewPort->ScrollBar(B_VERTICAL) != NULL;
	}

	void _UpdateScrollBar(BScrollBar* scrollBar, float viewPortSize,
		float dataSize)
	{
		if (scrollBar == NULL)
			return;

		if (viewPortSize < dataSize) {
			scrollBar->SetRange(0, dataSize - viewPortSize);
			scrollBar->SetProportion(viewPortSize / dataSize);
			float smallStep;
			scrollBar->GetSteps(&smallStep, NULL);
			scrollBar->SetSteps(smallStep, viewPortSize);
		} else {
			scrollBar->SetRange(0, 0);
			scrollBar->SetProportion(1);
		}
	}

private:
	BViewPort*	fViewPort;
	bool		fHasViewChild;
	bool		fIsCacheValid;
	BSize		fMin;
	BSize		fMax;
	BSize		fPreferred;
};


// #pragma mark - BViewPort


BViewPort::BViewPort(BView* child)
	:
	BView(NULL, 0),
	fChild(NULL)
{
	_Init();
	SetChildView(child);
}


BViewPort::BViewPort(BLayoutItem* child)
	:
	BView(NULL, 0),
	fChild(NULL)
{
	_Init();
	SetChildItem(child);
}


BViewPort::BViewPort(const char* name, BView* child)
	:
	BView(name, 0),
	fChild(NULL)
{
	_Init();
	SetChildView(child);
}


BViewPort::BViewPort(const char* name, BLayoutItem* child)
	:
	BView(name, 0),
	fChild(NULL)
{
	_Init();
	SetChildItem(child);
}


BViewPort::~BViewPort()
{
}


BView*
BViewPort::ChildView() const
{
	return fLayout->ChildView();
}


void
BViewPort::SetChildView(BView* child)
{
	fLayout->SetChildView(child);
	InvalidateLayout();
}


BLayoutItem*
BViewPort::ChildItem() const
{
	return fLayout->ChildItem();
}


void
BViewPort::SetChildItem(BLayoutItem* child)
{
	fLayout->SetChildItem(child);
	InvalidateLayout();
}


void
BViewPort::_Init()
{
	fLayout = new ViewPortLayout(this);
	SetLayout(fLayout);
}


}	// namespace BPrivate


using ::BPrivate::BViewPort;
