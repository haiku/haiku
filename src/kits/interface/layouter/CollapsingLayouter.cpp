/*
 * Copyright 2011, Haiku, Inc.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "CollapsingLayouter.h"

#include "ComplexLayouter.h"
#include "OneElementLayouter.h"
#include "SimpleLayouter.h"

#include <ObjectList.h>
#include <Size.h>


class CollapsingLayouter::ProxyLayoutInfo : public LayoutInfo {
public:
	ProxyLayoutInfo(LayoutInfo* target, int32 elementCount)
		:
		fTarget(target),
		fElementCount(elementCount)
	{
		fElements = new int32[elementCount];
	}

	~ProxyLayoutInfo()
	{
		delete[] fElements;
		delete fTarget;
	}

	void
	LayoutTarget(Layouter* layouter, float size)
	{
		if (layouter)
			layouter->Layout(fTarget, size);
	}

	void
	SetElementPosition(int32 element, int32 position)
	{
		fElements[element] = position;
	}

	float
	ElementLocation(int32 element)
	{
		if (element < 0 || element >= fElementCount || fElements[element] < 0)
			return 0;
		return fTarget->ElementLocation(fElements[element]);
	}

	float
	ElementSize(int32 element)
	{
		if (element < 0 || element >= fElementCount || fElements[element] < 0)
			return 0;
		return fTarget->ElementSize(fElements[element]);
	}

	float
	ElementRangeSize(int32 element, int32 length)
	{
		if (element < 0 || element >= fElementCount || fElements[element] < 0)
			return 0;
		return fTarget->ElementRangeSize(fElements[element], length);
	}

private:
	int32*					fElements;
	LayoutInfo*				fTarget;
	int32					fElementCount;
};


struct CollapsingLayouter::Constraint {
	int32 length;
	float min;
	float max;
	float preferred;
};


struct CollapsingLayouter::ElementInfo {
	float weight;
	int32 position;
	bool valid;
	BObjectList<Constraint> constraints;

	ElementInfo()
		:
		weight(0),
		position(-1),
		valid(false),
		constraints(5, true)
	{
	}

	~ElementInfo()
	{
	}

	void SetTo(const ElementInfo& other)
	{
		weight = other.weight;
		position = other.position;
		valid = other.valid;
		for (int32 i = other.constraints.CountItems() - 1; i >= 0; i--)
			constraints.AddItem(new Constraint(*other.constraints.ItemAt(i)));
	}
};


CollapsingLayouter::CollapsingLayouter(int32 elementCount, float spacing)
	:
	fElementCount(elementCount),
	fElements(new ElementInfo[elementCount]),
	fValidElementCount(0),
	fHaveMultiElementConstraints(false),
	fSpacing(spacing),
	fLayouter(NULL)
{
}


CollapsingLayouter::~CollapsingLayouter()
{
	delete[] fElements;
	delete fLayouter;
}


void
CollapsingLayouter::AddConstraints(int32 element, int32 length, float min,
	float max, float preferred)
{
	if (min == B_SIZE_UNSET && max == B_SIZE_UNSET)
		return;
	if (element < 0 || length <= 0 || element + length > fElementCount)
		return;

	Constraint* constraint = new Constraint();
	constraint->length = length;
	constraint->min = min;
	constraint->max = max;
	constraint->preferred = preferred;

	if (length > 1)
		fHaveMultiElementConstraints = true;
	
	int32 validElements = fValidElementCount;

	for (int32 i = element; i < element + length; i++) {
		if (fElements[i].valid == false) {
			fElements[i].valid = true;
			fValidElementCount++;
		}
	}

	fElements[element].constraints.AddItem(constraint);
	if (fValidElementCount > validElements) {
		delete fLayouter;
		fLayouter = NULL;
	}

	if (fLayouter)
		_AddConstraints(element, constraint);
	
}


void
CollapsingLayouter::SetWeight(int32 element, float weight)
{
	if (element < 0 || element >= fElementCount)
		return;

	ElementInfo& elementInfo = fElements[element];
	elementInfo.weight = weight;

	if (fLayouter && elementInfo.position >= 0)
		fLayouter->SetWeight(elementInfo.position, weight);
}


float
CollapsingLayouter::MinSize()
{
	_ValidateLayouter();
	return fLayouter ? fLayouter->MinSize() : 0;
}


float
CollapsingLayouter::MaxSize()
{
	_ValidateLayouter();
	return fLayouter ? fLayouter->MaxSize() : B_SIZE_UNLIMITED;
}


float
CollapsingLayouter::PreferredSize()
{
	_ValidateLayouter();
	return fLayouter ? fLayouter->PreferredSize() : 0;
}


LayoutInfo*
CollapsingLayouter::CreateLayoutInfo()
{
	_ValidateLayouter();

	LayoutInfo* info = fLayouter ? fLayouter->CreateLayoutInfo() : NULL;
	return new ProxyLayoutInfo(info, fElementCount);
}


void
CollapsingLayouter::Layout(LayoutInfo* layoutInfo, float size)
{
	_ValidateLayouter();
	ProxyLayoutInfo* info = static_cast<ProxyLayoutInfo*>(layoutInfo);
	for (int32 i = 0; i < fElementCount; i++) {
		info->SetElementPosition(i, fElements[i].position);
	}

	info->LayoutTarget(fLayouter, size);
}


Layouter*
CollapsingLayouter::CloneLayouter()
{
	CollapsingLayouter* clone = new CollapsingLayouter(fElementCount, fSpacing);
	for (int32 i = 0; i < fElementCount; i++)
		clone->fElements[i].SetTo(fElements[i]);

	clone->fValidElementCount = fValidElementCount;
	clone->fHaveMultiElementConstraints = fHaveMultiElementConstraints;

	if (fLayouter)
		clone->fLayouter = fLayouter->CloneLayouter();
	return clone;
}


void
CollapsingLayouter::_ValidateLayouter()
{
	if (fLayouter)
		return;

	_CreateLayouter();
	_DoCollapse();
	_AddConstraints();
	_SetWeights();
}


Layouter*
CollapsingLayouter::_CreateLayouter()
{
	if (fLayouter)
		return fLayouter;

	if (fValidElementCount == 0) {
		fLayouter =  NULL;
	} else if (fValidElementCount == 1) {
		fLayouter =  new OneElementLayouter();
	} else if (fHaveMultiElementConstraints) {
		fLayouter =  new ComplexLayouter(fValidElementCount, fSpacing);
	} else {
		fLayouter = new SimpleLayouter(fValidElementCount, fSpacing);
	}

	return fLayouter;
}


void
CollapsingLayouter::_DoCollapse()
{
	int32 shift = 0;
	for (int32 i = 0; i < fElementCount; i++) {
		ElementInfo& element = fElements[i];
		if (!element.valid) {
			shift++;
			element.position = -1;
			continue;
		} else {
			element.position = i - shift;
		}
	}
}


void
CollapsingLayouter::_AddConstraints()
{
	if (fLayouter == NULL)
		return;

	for (int32 i = 0; i < fElementCount; i++) {
		ElementInfo& element = fElements[i];
		for (int32 i = element.constraints.CountItems() - 1; i >= 0; i--)
			_AddConstraints(element.position, element.constraints.ItemAt(i));
	}
}


void
CollapsingLayouter::_AddConstraints(int32 position, const Constraint* c)
{
	fLayouter->AddConstraints(position, c->length, c->min, c->max,
		c->preferred);
}


void
CollapsingLayouter::_SetWeights()
{
	if (!fLayouter)
		return;

	for (int32 i = 0; i < fElementCount; i++) {
		fLayouter->SetWeight(fElements[i].position, fElements[i].weight);
	}
}
