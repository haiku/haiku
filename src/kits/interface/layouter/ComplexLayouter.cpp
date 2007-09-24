/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ComplexLayouter.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <new>

#include <OS.h>
#include <Size.h>

#include <AutoDeleter.h>

#include "LayoutOptimizer.h"
#include "SimpleLayouter.h"


using std::nothrow;


// MyLayoutInfo
class ComplexLayouter::MyLayoutInfo : public LayoutInfo {
public:
	MyLayoutInfo(int32 elementCount, int32 spacing)
		: fCount(elementCount),
		  fSpacing(spacing)
	{
		// We also store the location of the virtual elementCountth element.
		// Thus fLocation[i + 1] - fLocation[i] is the size of the ith element
		// (not considering spacing).
		fLocations = new(nothrow) int32[elementCount + 1];
	}

	~MyLayoutInfo()
	{
		delete[] fLocations;
	}

	virtual float ElementLocation(int32 element)
	{
		if (element < 0 || element >= fCount)
			return 0;

		return fLocations[element];
	}

	virtual float ElementSize(int32 element)
	{
		if (element < 0 || element >= fCount)
			return -1;

		return fLocations[element + 1] - fLocations[element] - 1
			- fSpacing;
	}

	virtual float ElementRangeSize(int32 position, int32 length)
	{
		if (position < 0 || length < 0 || position + length > fCount)
			return -1;

		return fLocations[position + length] - fLocations[position] - 1
			- fSpacing;
	}

	void Dump()
	{
		printf("ComplexLayouter::MyLayoutInfo(): %ld elements:\n", fCount);
		for (int32 i = 0; i < fCount + 1; i++)
			printf("  %2ld: location: %4ld\n", i, fLocations[i]);
	}

public:
	int32	fCount;
	int32	fSpacing;
	int32*	fLocations;
};


// Constraint
struct ComplexLayouter::Constraint {
	Constraint(int32 start, int32 end, int32 min, int32 max)
		: start(start),
		  end(end),
		  min(min),
		  max(max),
		  next(NULL)
	{
		if (min > max)
			max = min;
		effectiveMax = max;
	}

	void Restrict(int32 newMin, int32 newMax)
	{
		if (newMin > min)
			min = newMin;
		if (newMax < max)
			max = newMax;
		if (min > max)
			max = min;
		effectiveMax = max;
	}

	int32		start;
	int32		end;
	int32		min;
	int32		max;
	int32		effectiveMax;
	Constraint*	next;
};


// SumItem
struct ComplexLayouter::SumItem {
	int32	min;
	int32	max;
	bool	minDirty;
	bool	maxDirty;
};


// SumItemBackup
struct ComplexLayouter::SumItemBackup {
	int32	min;
	int32	max;
};


// #pragma mark - ComplexLayouter


// constructor
ComplexLayouter::ComplexLayouter(int32 elementCount, int32 spacing)
	: fElementCount(elementCount),
	  fSpacing(spacing),
	  fConstraints(new(nothrow) Constraint*[elementCount]),
	  fWeights(new(nothrow) float[elementCount]),
	  fSums(new(nothrow) SumItem[elementCount + 1]),
	  fSumBackups(new(nothrow) SumItemBackup[elementCount + 1]),
	  fOptimizer(new(nothrow) LayoutOptimizer(elementCount)),
	  fLayoutValid(false)
{
// TODO: Check initialization!
	if (fConstraints)
		memset(fConstraints, 0, sizeof(Constraint*) * fElementCount);

	if (fWeights) {
		for (int32 i = 0; i < fElementCount; i++)
			fWeights[i] = 1.0f;
	}
}


// destructor
ComplexLayouter::~ComplexLayouter()
{
	for (int32 i = 0; i < fElementCount; i++) {
		Constraint* constraint = fConstraints[i];
		fConstraints[i] = NULL;
		while (constraint != NULL) {
			Constraint* next = constraint->next;
			delete constraint;
			constraint = next;
		}
	}

	delete[] fConstraints;
	delete[] fWeights;
	delete[] fSums;
	delete[] fSumBackups;
  	delete fOptimizer;
}


// InitCheck
status_t
ComplexLayouter::InitCheck() const
{
	if (!fConstraints || !fWeights || !fSums || !fSumBackups || !fOptimizer)
		return B_NO_MEMORY;
	return fOptimizer->InitCheck();
}


// AddConstraints
void
ComplexLayouter::AddConstraints(int32 element, int32 length,
	float _min, float _max, float _preferred)
{
	if (element < 0 || length <= 0 || element + length > fElementCount)
		return;

//printf("%p->ComplexLayouter::AddConstraints(%ld, %ld, %ld, %ld, %ld)\n",
//this, element, length, (int32)_min, (int32)_max, (int32)_preferred);

	int32 spacing = fSpacing * (length - 1);
	int32 min = (int32)_min + 1 - spacing;
	int32 max = (int32)_max + 1 - spacing;

	if (min < 0)
		min = 0;
	if (max > B_SIZE_UNLIMITED)
		max = B_SIZE_UNLIMITED;

	int32 end = element + length - 1;
	Constraint** slot = fConstraints + end;
	while (*slot != NULL && (*slot)->start > element)
		slot = &(*slot)->next;

	if (*slot != NULL && (*slot)->start == element) {
		// previous constraint exists -- use stricter values
		(*slot)->Restrict(min, max);
	} else {
		// no previous constraint -- create new one
		Constraint* constraint = new(nothrow) Constraint(element, end, min,
			max);
		if (!constraint)
			return;
		constraint->next = *slot;
		*slot = constraint;
	}
}


// SetWeight
void
ComplexLayouter::SetWeight(int32 element, float weight)
{
	if (element < 0 || element >= fElementCount)
		return;

	fWeights[element] = max_c(weight, 0);
}


// MinSize
float
ComplexLayouter::MinSize()
{
	_ValidateLayout();
	return fMin;
}


// MaxSize
float
ComplexLayouter::MaxSize()
{
	_ValidateLayout();
	return fMax;
}


// PreferredSize
float
ComplexLayouter::PreferredSize()
{
	return fMin;
}


// CreateLayoutInfo
LayoutInfo*
ComplexLayouter::CreateLayoutInfo()
{
	MyLayoutInfo* layoutInfo = new(nothrow) MyLayoutInfo(fElementCount,
		fSpacing);
	if (layoutInfo && !layoutInfo->fLocations) {
		delete layoutInfo;
		return NULL;
	}

	return layoutInfo;
}


// Layout
void
ComplexLayouter::Layout(LayoutInfo* _layoutInfo, float _size)
{
//printf("%p->ComplexLayouter::Layout(%ld)\n", this, (int32)_size);
	if (fElementCount == 0)
		return;

	_ValidateLayout();

	MyLayoutInfo* layoutInfo = (MyLayoutInfo*)_layoutInfo;

	int32 min = fSums[fElementCount].min;
	int32 max = fSums[fElementCount].max;

	int32 size = (int32)_size + 1 - (fElementCount - 1) * fSpacing;
	if (size < min)
		size = min;
	if (size > max)
		size = max;

	SumItem sums[fElementCount + 1];
	memcpy(sums, fSums, (fElementCount + 1) * sizeof(SumItem));

	sums[fElementCount].min = size;
	sums[fElementCount].max = size;
	sums[fElementCount].minDirty = (size != min);
	sums[fElementCount].maxDirty = (size != max);

	// propagate the size
	_PropagateChangesBack(sums, fElementCount - 1, NULL);
	_PropagateChanges(sums, fElementCount - 1, NULL);

printf("Layout(%ld)\n", size);
for (int32 i = 0; i < fElementCount; i++) {
SumItem& sum = sums[i + 1];
printf("[%ld] minc = %4ld,  maxc = %4ld\n", i + 1, sum.min, sum.max);
}

// TODO: Test whether the desired solution already satisfies all constraints.
// If so, we can skip the constraint solving part.


// TODO: We should probably already setup the constraints in _ValidateLayout().
// This way we might not be able to skip as many redundant constraints, but
// supposedly it doesn't matter all that much, since those constraints
// wouldn't make it into the active set anyway.
	fOptimizer->RemoveAllConstraints();

	// add constraints
	for (int32 i = 0; i < fElementCount; i++) {
		SumItem& sum = fSums[i + 1];

		Constraint* constraint = fConstraints[i];
		while (constraint != NULL) {
			SumItem& base = fSums[constraint->start];
			int32 sumMin = base.min + constraint->min;
			int32 baseMax = sum.max - constraint->min;
			bool minRedundant = (sumMin < sum.min && baseMax > base.max);

			int32 sumMax = base.max + constraint->effectiveMax;
			int32 baseMin = sum.min - constraint->effectiveMax;
			bool maxRedundant = (sumMax > sum.max && baseMin < base.min);

			if (!minRedundant || !maxRedundant) {
				bool success = true;
				if (constraint->min == constraint->effectiveMax) {
					// min and max equal -- add an equality constraint
					success = fOptimizer->AddConstraint(constraint->start - 1,
						constraint->end, constraint->min, true);
				} else {
					// min and max not equal -- add them individually,
					// unless redundant
					if (!minRedundant) {
						success |= fOptimizer->AddConstraint(
							constraint->start - 1, constraint->end,
							constraint->min, false);
					}
					if (!maxRedundant) {
						success |= fOptimizer->AddConstraint(constraint->end,
							constraint->start - 1,
							-constraint->effectiveMax, false);
					}
				}
			}

			constraint = constraint->next;
		}
	}

	// prepare a feasible solution (the minimum)
	double values[fElementCount];
	for (int32 i = 0; i < fElementCount; i++)
		values[i] = sums[i + 1].min - sums[i].min;

	// prepare the desired solution
	int32 sizes[fElementCount];
	SimpleLayouter::DistributeSize(size, fWeights, sizes, fElementCount);
	double realSizes[fElementCount];
	for (int32 i = 0; i < fElementCount; i++)
		realSizes[i] = sizes[i];

printf("feasible solution vs. desired solution:\n");
for (int32 i = 0; i < fElementCount; i++)
printf("%8.4f   %8.4f\n", values[i], realSizes[i]);

	// solve
bigtime_t time = system_time();
	if (!fOptimizer->Solve(realSizes, size, values))
		return;
time = system_time() - time;

	// compute integer solution
	// The basic strategy is to floor() the sums. This guarantees that the
	// difference between two rounded sums remains in the range of floor()
	// and ceil() of their real value difference. Since the constraints have
	// integer values, the integer solution will satisfy all constraints the
	// real solution satisfied.
printf("computed solution in %lld us:\n", time);
	double realSum = 0;
	int32 spacing = 0;
	layoutInfo->fLocations[0] = 0;
	for (int32 i = 0; i < fElementCount; i++) {
		realSum += values[i];
		double roundedRealSum = floor(realSum);
		if (fuzzy_equals(realSum, roundedRealSum + 1))
			realSum = roundedRealSum + 1;
		layoutInfo->fLocations[i + 1] = (int32)roundedRealSum + spacing;
		spacing += fSpacing;

printf("x[%ld] = %8.4f   %4ld\n", i, values[i], layoutInfo->fLocations[i + 1] - layoutInfo->fLocations[i]);
	}
}


// CloneLayouter
Layouter*
ComplexLayouter::CloneLayouter()
{
	ComplexLayouter* layouter
		= new(nothrow) ComplexLayouter(fElementCount, fSpacing);
	ObjectDeleter<ComplexLayouter> layouterDeleter(layouter);
	if (!layouter || layouter->InitCheck() != B_OK
		|| !layouter->fOptimizer->AddConstraintsFrom(fOptimizer)) {
		return NULL;
	}

	// clone the constraints
	for (int32 i = 0; i < fElementCount; i++) {
		Constraint* constraint = fConstraints[i];
		Constraint** end = layouter->fConstraints + i;
		while (constraint) {
			*end = new(nothrow) Constraint(constraint->start, constraint->end,
				constraint->min, constraint->max);
			if (!*end)
				return NULL;

			end = &(*end)->next;
			constraint = constraint->next;
		}
	}

	// copy the other stuff
	memcpy(layouter->fWeights, fWeights, fElementCount * sizeof(float));
	memcpy(layouter->fSums, fSums, (fElementCount + 1) * sizeof(SumItem));
	memcpy(layouter->fSumBackups, fSumBackups,
		(fElementCount + 1) * sizeof(SumItemBackup));
	layouter->fMin = fMin;
	layouter->fMax = fMax;
	layouter->fLayoutValid = fLayoutValid;

	return layouterDeleter.Detach();
}


// _ValidateLayout
void
ComplexLayouter::_ValidateLayout()
{
	if (fLayoutValid)
		return;

	fSums[0].min = 0;
	fSums[0].max = 0;

	for (int32 i = 0; i < fElementCount; i++) {
		SumItem& sum = fSums[i + 1];
		sum.min = 0;
		sum.max = B_SIZE_UNLIMITED;
		sum.minDirty = false;
		sum.maxDirty = false;
	}

	// apply min constraints forward:
	//   minc[k] >= minc[i-1] + min[i,j]
	for (int32 i = 0; i < fElementCount; i++) {
		SumItem& sum = fSums[i + 1];

		Constraint* constraint = fConstraints[i];
		while (constraint != NULL) {
			int32 minSum = fSums[constraint->start].min + constraint->min;
			if (minSum > sum.min)
				sum.min = minSum;
else {
printf("min constraint is redundant: x%ld + ... + x%ld >= %ld\n",
constraint->start, constraint->end, constraint->min);
}

			constraint = constraint->next;
		}
	}

	// apply min constraints backwards:
	//   maxc[i-1] <= maxc[k] - min[i,j]
	for (int32 i = fElementCount - 1; i >= 0; i--) {
		SumItem& sum = fSums[i + 1];

		Constraint* constraint = fConstraints[i];
		while (constraint != NULL) {
			SumItem& base = fSums[constraint->start];
			int32 baseMax = sum.max - constraint->min;
			if (baseMax < base.max)
				base.max = baseMax;

			constraint = constraint->next;
		}
	}

	// apply max constraints
	for (int32 i = 0; i < fElementCount; i++) {
		Constraint* constraint = fConstraints[i];
		while (constraint != NULL) {
			_ApplyMaxConstraint(constraint, i);

			constraint = constraint->next;
		}
//printf("fSums[%ld] = {%ld, %ld}\n", i + 1, sum.min, sum.max);
	}

for (int32 i = 0; i < fElementCount; i++) {
SumItem& sum = fSums[i + 1];
printf("[%ld] minc = %4ld,  maxc = %4ld\n", i + 1, sum.min, sum.max);
}

	if (fElementCount == 0) {
		fMin = -1;
		fMax = B_SIZE_UNLIMITED;
	} else {
		int32 spacing = (fElementCount - 1) * fSpacing;
		fMin = fSums[fElementCount].min + spacing - 1;
		fMax = fSums[fElementCount].max + spacing - 1;
	}

	fLayoutValid = true;
}

/*
		x[i] + ... + x[i+j] >= min[i,j]
		x[i] + ... + x[i+j] <= max[i,j]
	with
		1 <= i <= n
		0 <= j <= n - i
		0 <= min[i,j] <= max[i,j]

	Let

		c[0] = 0
		c[k] = x[1] + ... + x[k]	for 1 <= k <= n

	it follows

		x[i] + ... + x[i+j] = c[i+j] - c[i-1]

	and thus the constraints can be rewritten as

		c[i+j] - c[i-1] >= min[i,j]
		c[i+j] - c[i-1] <= max[i,j]

	or	

		c[i+j] >= c[i-1] + min[i,j]
		c[i+j] <= c[i-1] + max[i,j]

	We're looking for minimal minc[] and maximal maxc[] such that

		minc[i+j] >= minc[i-1] + min[i,j]
		maxc[i+j] <= maxc[i-1] + max[i,j]

		minc[i+j] <= minc[i-1] + max[i,j]
		maxc[i+j] >= maxc[i-1] + min[i,j]

	holds for all i and j. The latter two kinds of constraints have to be
	enforced backwards:

		minc[i-1] >= minc[i+j] - max[i,j]
		maxc[i-1] <= maxc[i+j] - min[i,j]



-----------------

	// (1) maxc[k] <= maxc[i-1] + max[i,j]
	// (2) minc[i-1] >= minc[k] - max[i,j]

	Modifying maxc[k] according to (1) potentially invalidates constraints of
	these forms:

		(i)  maxc[i'-1] <= maxc[k] - min[i',j']
		(ii) maxc[k+1+j'] <= maxc[k] + max[k+1,j']

	After propagating (i) constraints backwards, all of them will be hold,
	though more (ii) constraints might have been invalidated, though.
	Propagating (ii) constraints forward afterwards, will make them all hold.
	Since the min[i,j] and max[i,j] constraints are separation constraints and
	the CSP not including the newly added constraint was conflict-free,
	propagating the (i) and (ii) constraints won't change the maxc[i], i < k by
	more than what maxc[k] changed. If afterwards the constraint (1) doesn't
	hold, it apparently conflicts with the other constraints.
*/


// _ApplyMaxConstraint
void
ComplexLayouter::_ApplyMaxConstraint(Constraint* currentConstraint, int32 index)
{
	SumItem& sum = fSums[index + 1];
	SumItem& base = fSums[currentConstraint->start];

	// We want to apply:
	//   c[i+j] <= c[i-1] + max[i,j]
	//
	// This has the following direct consequences (let k = i + j):
	// (1) maxc[k] <= maxc[i-1] + max[i,j]
	// (2) minc[i-1] >= minc[k] - max[i,j]
	//
	// If maxc[k] or minc[i-i] changed, those changes have to be propagated
	// back.

	// apply (1) maxc[k] <= maxc[i-1] + max[i,j]
	int32 max = currentConstraint->effectiveMax;
	int32 sumMax = base.max + max;

	// enforce maxc[i+j] >= minc[i+j]
	if (sumMax < sum.min) {
		sumMax = sum.min;
		max = sumMax - base.max;
	}

	// apply (2) minc[i-1] >= minc[k] - max[i,j]
	// and check minc[i-1] <= maxc[i-1]
	int32 baseMin = sum.min - max;
	if (baseMin > base.max) {
		baseMin = base.max;
		max = sum.min - baseMin;
		sumMax = base.max + max;
	}

	// apply changes

if (currentConstraint->effectiveMax != max) {
printf("relaxing conflicting max constraint (1): x%ld + ... + x%ld <= %ld -> %ld\n",
currentConstraint->start, currentConstraint->end,
currentConstraint->effectiveMax, max);
}
	currentConstraint->effectiveMax = max;

	if (baseMin <= base.min && sumMax >= sum.max)
{
printf("max constraint is redundant: x%ld + ... + x%ld <= %ld\n",
currentConstraint->start, currentConstraint->end, currentConstraint->effectiveMax);
		return;
}

	// backup old values, in case we detect a conflict later
	_BackupValues(index);

	int32 diff;
	do {
		// apply the changes
		int32 changedIndex = currentConstraint->start;

		if (baseMin > base.min) {
			base.min = baseMin;
			base.minDirty = true;
		}

		if (sumMax < sum.max) {
			changedIndex = index;
			sum.max = sumMax;
			sum.maxDirty = true;
		}

		// propagate the changes
		_PropagateChangesBack(fSums, changedIndex, currentConstraint);
		_PropagateChanges(fSums, index, currentConstraint);

		// check the new constraint again -- if it doesn't hold, it
		// conflicts with the other constraints
		diff = 0;

		// check (1) maxc[k] <= maxc[i-1] + max[i,j]
		max = currentConstraint->effectiveMax;
		sumMax = base.max + max;
		if (sumMax < sum.max)
			diff = sum.max - sumMax;

		// check (2) minc[i-1] >= minc[k] - max[i,j]
		baseMin = sum.min - max;
		if (baseMin > base.min)
			diff = max_c(diff, baseMin - base.min);

		// clear the dirty flags
		for (int32 i = 0; i <= changedIndex; i++) {
			SumItem& sum = fSums[i + 1];
			sum.minDirty = false;
			sum.maxDirty = false;
		}

		// if we've got a conflict, we relax the constraint and try again
		if (diff > 0) {
			max += diff;
printf("relaxing conflicting max constraint (2): x%ld + ... + x%ld <= %ld -> %ld\n",
currentConstraint->start, currentConstraint->end,
currentConstraint->effectiveMax, max);
			currentConstraint->effectiveMax = max;

			_RestoreValues(index);

			sumMax = base.max + max;
			baseMin = sum.min - max;

			if (baseMin <= base.min && sumMax >= sum.max)
				return;
		}
	} while (diff > 0);
}


// _PropagateChanges
/*!	Propagate changes forward using min and max constraints. Max constraints
	Beyond \a toIndex or at \a to toIndex after (and including)
	\a lastMaxConstraint will be ignored. To have all constraints be
	considered pass \c fElementCount and \c NULL.
*/
void
ComplexLayouter::_PropagateChanges(SumItem* sums, int32 toIndex,
	Constraint* lastMaxConstraint)
{
	for (int32 i = 0; i < fElementCount; i++) {
		SumItem& sum = sums[i + 1];

		bool ignoreMaxConstraints = (i > toIndex);

		Constraint* constraint = fConstraints[i];
		while (constraint != NULL) {
			SumItem& base = sums[constraint->start];

			if (constraint == lastMaxConstraint)
				ignoreMaxConstraints = true;

			// minc[k] >= minc[i-1] + min[i,j]
			if (base.minDirty) {
				int32 sumMin = base.min + constraint->min;
				if (sumMin > sum.min) {
					sum.min = sumMin;
					sum.minDirty = true;
				}
			}

			// maxc[k] <= maxc[i-1] + max[i,j]
			if (base.maxDirty && !ignoreMaxConstraints) {
				int32 sumMax = base.max + constraint->effectiveMax;
				if (sumMax < sum.max) {
					sum.max = sumMax;
					sum.maxDirty = true;
				}
			}

			constraint = constraint->next;
		}

		if (sum.minDirty || sum.maxDirty) {
			if (sum.min > sum.max) {
// TODO: Can this actually happen?
printf("adjusted max in propagation phase: index: %ld: %ld -> %ld\n", i, sum.max, sum.min);
				sum.max = sum.min;
				sum.maxDirty = true;
			}
		}
	}
}


// _PropagateChangesBack
void
ComplexLayouter::_PropagateChangesBack(SumItem* sums, int32 changedIndex,
	Constraint* lastMaxConstraint)
{
	for (int32 i = changedIndex; i >= 0; i--) {
		SumItem& sum = sums[i + 1];

		bool ignoreMaxConstraints = false;

		Constraint* constraint = fConstraints[i];
		while (constraint != NULL) {
			SumItem& base = sums[constraint->start];

			if (constraint == lastMaxConstraint)
				ignoreMaxConstraints = true;

			// minc[i-1] >= minc[k] - max[i,j]
			if (sum.minDirty && !ignoreMaxConstraints) {
				int32 baseMin = sum.min - constraint->effectiveMax;
				if (baseMin > base.min) {
if (baseMin > base.max) {
printf("min above max in back propagation phase: index: (%ld -> %ld), "
"min: %ld, max: %ld\n", i, constraint->start, baseMin, base.max);
}
					base.min = baseMin;
					base.minDirty = true;
				}
			}

			// maxc[i-1] <= maxc[k] - min[i,j]
			if (sum.maxDirty) {
				int32 baseMax = sum.max - constraint->min;
				if (baseMax < base.max) {
if (baseMax < base.min) {
printf("max below min in back propagation phase: index: (%ld -> %ld), "
"max: %ld, min: %ld\n", i, constraint->start, baseMax, base.min);
}
					base.max = baseMax;
					base.maxDirty = true;
				}
			}

			constraint = constraint->next;
		}
	}
}


// _BackupValues
void
ComplexLayouter::_BackupValues(int32 maxIndex)
{
	for (int32 i = 0; i <= maxIndex; i++) {
		SumItem& sum = fSums[i + 1];
		fSumBackups[i + 1].min = sum.min;
		fSumBackups[i + 1].max = sum.max;
	}
}


// _RestoreValues
void
ComplexLayouter::_RestoreValues(int32 maxIndex)
{
	for (int32 i = 0; i <= maxIndex; i++) {
		SumItem& sum = fSums[i + 1];
		sum.min = fSumBackups[i + 1].min;
		sum.max = fSumBackups[i + 1].max;
	}
}

