#include "ALMGroup.h"


#include <ALMLayout.h>
#include <Tab.h>


ALMGroup::ALMGroup(BLayoutItem* item)
{
	_Init(item, NULL);
}


ALMGroup::ALMGroup(BView* view)
{
	_Init(NULL, view);
}


BLayoutItem*
ALMGroup::LayoutItem() const
{
	return fLayoutItem;
}


BView*
ALMGroup::View() const
{
	return fView;
}


const std::vector<ALMGroup>&
ALMGroup::Groups() const
{
	return fGroups;
}


enum orientation
ALMGroup::Orientation() const
{
	return fOrientation;
}


ALMGroup&
ALMGroup::operator|(const ALMGroup& right)
{
	return _AddItem(right, B_HORIZONTAL);
}


ALMGroup&
ALMGroup::operator/(const ALMGroup& bottom)
{
	return _AddItem(bottom, B_VERTICAL);
}


void
ALMGroup::BuildLayout(BALMLayout* layout, XTab* left, YTab* top, XTab* right,
	YTab* bottom)
{
	if (left == NULL)
		left = layout->Left();
	if (top == NULL)
		top = layout->Top();
	if (right == NULL)
		right = layout->Right();
	if (bottom == NULL)
		bottom = layout->Bottom();

	_Build(layout, left, top, right, bottom);
}


ALMGroup::ALMGroup()
{
	_Init(NULL, NULL);
}


void
ALMGroup::_Init(BLayoutItem* item, BView* view, enum orientation orien)
{
	fLayoutItem = item;
	fView = view;
	fOrientation = orien;
}


void
ALMGroup::_Build(BALMLayout* layout, BReference<XTab> left,
	BReference<YTab> top, BReference<XTab> right, BReference<YTab> bottom) const
{
	if (LayoutItem())
		layout->AddItem(LayoutItem(), left, top, right, bottom);
	else if (View()) {
		layout->AddView(View(), left, top, right, bottom);
	} else {
		for (unsigned int i = 0; i < Groups().size(); i++) {
			const ALMGroup& current = Groups()[i];
			if (Orientation() == B_HORIZONTAL) {
				BReference<XTab> currentRight;
				if (i == Groups().size() - 1)
					currentRight = right;
				else
					currentRight = layout->AddXTab();
				current._Build(layout, left, top, currentRight, bottom);
				left = currentRight;
			} else {
				BReference<YTab> currentBottom;
				if (i == Groups().size() - 1)
					currentBottom = bottom;
				else
					currentBottom = layout->AddYTab();
				current._Build(layout, left, top, right, currentBottom);
				top = currentBottom;
			}
		}
	}
}


ALMGroup&
ALMGroup::_AddItem(const ALMGroup& item, enum orientation orien)
{
	if (fGroups.size() == 0)
		fGroups.push_back(*this);
	else if (fOrientation != orien) {
		ALMGroup clone = *this;
		fGroups.clear();
		_Init(NULL, NULL, orien);
		fGroups.push_back(clone);
	}

	_Init(NULL, NULL, orien);
	fGroups.push_back(item);
	return *this;
}

