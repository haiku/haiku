/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "SimpleLayouter.h"

#include <math.h>

#include <LayoutUtils.h>
#include <List.h>
#include <Size.h>


// no lround() under BeOS R5 x86
#ifdef HAIKU_TARGET_PLATFORM_LIBBE_TEST
#	define lround(x)	(long)floor((x) + 0.5)
#endif


// ElementLayoutInfo
class SimpleLayouter::ElementLayoutInfo {
public:
	int32	size;
	int32	location;

	ElementLayoutInfo()
		: size(0),
		  location(0)
	{
	}
};

// ElementInfo
class SimpleLayouter::ElementInfo {
public:
	int32	index;
	int32	min;
	int32	max;
	int32	preferred;
	float	weight;
	int64	tempWeight;

	ElementInfo()
		: index(0),
		  min(0),
		  max(B_SIZE_UNLIMITED),
		  preferred(0),
		  weight(1),
		  tempWeight(0)
	{		
	}
	
	ElementInfo(int index)
		: index(index),
		  min(0),
		  max(B_SIZE_UNLIMITED),
		  preferred(0),
		  weight(1),
		  tempWeight(0)
	{		
	}
	
	void Assign(const ElementInfo& info)
	{
		min = info.min;
		max = info.max;
		preferred = info.preferred;
		weight = info.weight;
		tempWeight = info.tempWeight;
	}
};

// MyLayoutInfo
class SimpleLayouter::MyLayoutInfo : public LayoutInfo {
public:
	int32				fSize;
	ElementLayoutInfo*	fElements;
	int32				fElementCount;

	MyLayoutInfo(int32 elementCount)
		: fSize(0),
		  fElementCount(elementCount)
	{
		fElements = new ElementLayoutInfo[elementCount];
	}

	virtual ~MyLayoutInfo()
	{
		delete[] fElements;
	}
	
	virtual float ElementLocation(int32 element)
	{
		if (element < 0 || element >= fElementCount) {
			// error
			return 0;
		}

		return fElements[element].location;
	}

	virtual float ElementSize(int32 element)
	{
		if (element < 0 || element >= fElementCount) {
			// error
			return -1;
		}

		return fElements[element].size - 1;
	}
};


// constructor
SimpleLayouter::SimpleLayouter(int32 elementCount, float spacing)
	: fElementCount(elementCount),
	  fSpacing((int32)spacing),
	  fMin(0),
	  fMax(B_SIZE_UNLIMITED),
	  fPreferred(0),
	  fMinMaxValid(false),
	  fLayoutInfo(NULL)
{
	fElements = new ElementInfo[elementCount];
	for (int i = 0; i < elementCount; i++)
		fElements[i].index = i;
}

// destructor
SimpleLayouter::~SimpleLayouter()
{
	delete[] fElements;
}

// AddConstraints
void
SimpleLayouter::AddConstraints(int32 element, int32 length,
	float _min, float _max, float _preferred)
{
	if (element < 0 || element >= fElementCount) {
		// error
		return;
	}
	if (length != 1) {
		// error
		return;
	}

	int32 min = (int32)_min + 1;
	int32 max = (int32)_max + 1;
//	int32 preferred = (int32)_preferred + 1;
	
	ElementInfo& info = fElements[element];
	info.min = max_c(info.min, min);
	info.max = min_c(info.max, max);
	info.preferred = max_c(info.min, min);

	fMinMaxValid = false;
}

// SetWeight
void
SimpleLayouter::SetWeight(int32 element, float weight)
{
	if (element < 0 || element >= fElementCount) {
		// error
		return;
	}

	fElements[element].weight = weight;
}

// MinSize
float
SimpleLayouter::MinSize()
{
	_ValidateMinMax();
	return fMin - 1;
}

// MaxSize
float
SimpleLayouter::MaxSize()
{
	_ValidateMinMax();
	return fMax - 1;
}

// PreferredSize
float
SimpleLayouter::PreferredSize()
{
	_ValidateMinMax();
	return fPreferred - 1;
}

// CreateLayoutInfo
LayoutInfo*
SimpleLayouter::CreateLayoutInfo()
{
	return new MyLayoutInfo(fElementCount);
}

// Layout
void
SimpleLayouter::Layout(LayoutInfo* layoutInfo, float _size)
{
	int32 size = int32(_size + 1);

	fLayoutInfo = (MyLayoutInfo*)layoutInfo;
	
	_ValidateMinMax();

	if (fElementCount == 0)
		return;

	fLayoutInfo->fSize = max_c(size, fMin);

	// layout the elements
	if (fLayoutInfo->fSize >= fMax)
		_LayoutMax();
	else
		_LayoutStandard();

	// set locations
	int location = 0;
	for (int i = 0; i < fElementCount; i++) {
		fLayoutInfo->fElements[i].location = location;
		location += fSpacing + fLayoutInfo->fElements[i].size;
	}
}

// CloneLayouter
Layouter*
SimpleLayouter::CloneLayouter()
{
	SimpleLayouter* layouter = new SimpleLayouter(fElementCount, fSpacing);

	for (int i = 0; i < fElementCount; i++)
		layouter->fElements[i].Assign(fElements[i]);

	layouter->fMin = fMin;
	layouter->fMax = fMax;
	layouter->fPreferred = fPreferred;

	return layouter;
}

// DistributeSize
void
SimpleLayouter::DistributeSize(int32 size, float weights[], int32 sizes[],
	int32 count)
{
	// create element infos
	BList elementInfos(count);
	for (int32 i = 0; i < count; i++) {
		ElementInfo* info = new ElementInfo(i);
		info->weight = weights[i];
		elementInfos.AddItem(info);
	}

	// compute integer weights
	int64 sumWeight = _CalculateSumWeight(elementInfos);

	// distribute the size
	int64 weight = 0;
	int32 sumSize = 0;
	for (int32 i = 0; i < count; i++) {
		ElementInfo* info = (ElementInfo*)elementInfos.ItemAt(i);
		weight += info->tempWeight;
		int32 oldSumSize = sumSize;
		sumSize = (int32)(size * weight / sumWeight);
		sizes[i] = sumSize - oldSumSize;

		delete info;
	}
}

// _CalculateSumWeight
long
SimpleLayouter::_CalculateSumWeight(BList& elementInfos)
{
	if (elementInfos.IsEmpty())
		return 0;
	int32 count = elementInfos.CountItems();
	
	// sum up the floating point weight, so we get a scale
	double scale = 0;
	for (int32 i = 0; i < count; i++) {
		ElementInfo* info = (ElementInfo*)elementInfos.ItemAt(i);
		scale += info->weight;
	}

	int64 weight = 0;

	if (scale == 0) {
		// The weight sum is 0: We assign each info a temporary weight of 1.
		for (int32 i = 0; i < count; i++) {
			ElementInfo* info = (ElementInfo*)elementInfos.ItemAt(i);
			info->tempWeight = 1;
			weight += info->tempWeight;
		}
	} else {
		// We scale the weights so that their sum is about 100000. This should
		// give us ample resolution. If possible make the scale integer, so that
		// integer weights will produce exact results.
		if (scale >= 1 && scale <= 100000)
			scale = lround(100000 / scale);
		else
			scale = 100000 / scale;

		for (int32 i = 0; i < count; i++) {
			ElementInfo* info = (ElementInfo*)elementInfos.ItemAt(i);
			info->tempWeight = (int64)(info->weight * scale);
			weight += info->tempWeight;
		}
	}
	
	return weight;
}

// _ValidateMinMax	
void
SimpleLayouter::_ValidateMinMax()
{
	if (fMinMaxValid)
		return;

	fMinMaxValid = true;

	if (fElementCount == 0) {
		fMin = 0;
		fMax = B_SIZE_UNLIMITED;
		fPreferred = 0;
		return;
	}
	
	int spacing = (fElementCount - 1) * fSpacing;
	fMin = spacing;
	fMax = spacing;
	fPreferred = spacing;

	for (int i = 0; i < fElementCount; i++) {
		ElementInfo& info = fElements[i];

		// correct the preferred and maximum sizes
		if (info.max < info.min)
			info.max = info.min;
		if (info.preferred < info.min)
			info.preferred = info.min;
		else if (info.preferred > info.max)
			info.preferred = info.max;

		// sum up
		fMin += info.min;
		fMax = BLayoutUtils::AddSizesInt32(fMax, info.max);
		fPreferred = BLayoutUtils::AddSizesInt32(fPreferred, info.preferred);
	}
}

// _LayoutMax
void
SimpleLayouter::_LayoutMax()
{
	ElementInfo* infos = fElements;
	int32 count = fElementCount;
	if (count == 0)
		return;

	int32 additionalSpace = fLayoutInfo->fSize - fMax;

	// layout to the maximum first
	for (int i = 0; i < count; i++)
		fLayoutInfo->fElements[infos[i].index].size = infos[i].max;

// Mmh, distributing according to the weights doesn't look that good.
//	// Now distribute the additional space according to the weights.
//	int64 sumWeight = calculateSumWeight(Arrays.asList(infos));
//	int64 weight = 0;
//	int64 sumSize = 0;
//	for (int i = 0; i < infos.length; i++) {
//		weight += infos[i].tempWeight;
//		int64 oldSumSize = sumSize;
//		sumSize = (int)(additionalSpace * weight / sumWeight);
//		fLayoutInfo.fElements[infos[i].index].size += sumSize - oldSumSize;
//	}

	// distribute the additional space equally
	int64 sumSize = 0;
	for (int i = 0; i < count; i++) {
		int64 oldSumSize = sumSize;
		sumSize = additionalSpace * (i + 1) / count;
		fLayoutInfo->fElements[infos[i].index].size
			+= int32(sumSize - oldSumSize);
	}
}

// _LayoutStandard
void
SimpleLayouter::_LayoutStandard()
{
	int32 space = fLayoutInfo->fSize - (fElementCount - 1) * fSpacing;

	BList infosToLayout(fElementCount);
	for (int i = 0; i < fElementCount; i++) {
		infosToLayout.AddItem(&fElements[i]);
		fLayoutInfo->fElements[i].size = 0;
	}

	BList infosUnderMax(fElementCount);
	BList infosOverMin(fElementCount);
	while (infosToLayout.CountItems() > 0) {
		int32 remainingSpace = 0;
		int32 infoCount = infosToLayout.CountItems();
		int64 sumWeight = _CalculateSumWeight(infosToLayout);
		int64 assignedWeight = 0;
		int32 assignedSize = 0;

		for (int32 i = 0; i < infoCount; i++) {
			ElementInfo* info = (ElementInfo*)infosToLayout.ItemAt(i);
			ElementLayoutInfo& layoutInfo = fLayoutInfo->fElements[info->index];
			// The simple algorithm is this:
			// info.size += (int)(space * info.tempWeight / sumWeight);
			// I.e. we simply assign space according to the weight. To avoid the
			// rounding problematic, we make it a bit more complicated. We
			// assign the difference of total assignment for all infos including
			// the current one minus the total excluding the current one.
			assignedWeight += info->tempWeight;
			int32 oldAssignedSize = assignedSize;
			assignedSize = (int32)(space * assignedWeight / sumWeight);
			layoutInfo.size += assignedSize - oldAssignedSize;

			if (layoutInfo.size < info->min) {
				remainingSpace += layoutInfo.size - info->min;
				layoutInfo.size = info->min;
			} else if (layoutInfo.size > info->max) {
				remainingSpace += layoutInfo.size - info->max;
				layoutInfo.size = info->max;
			}

			if (layoutInfo.size > info->min)
				infosOverMin.AddItem(info);
			if (layoutInfo.size < info->max)
				infosUnderMax.AddItem(info);
		}

		infosToLayout.MakeEmpty();
		if (remainingSpace > 0)
			infosToLayout.AddList(&infosUnderMax);
		else if (remainingSpace < 0)
			infosToLayout.AddList(&infosOverMin);
		infosUnderMax.MakeEmpty();
		infosOverMin.MakeEmpty();
		space = remainingSpace;
	}
}
