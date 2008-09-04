/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Layouter implementation that can handle complex constraints.
 */
#ifndef COMPLEX_LAYOUTER_H
#define COMPLEX_LAYOUTER_H

#include <List.h>

#include "Layouter.h"


namespace BPrivate {
namespace Layout {

class LayoutOptimizer;

class ComplexLayouter : public Layouter {
public:
								ComplexLayouter(int32 elementCount,
									float spacing);
	virtual						~ComplexLayouter();

	virtual	status_t			InitCheck() const;

	virtual	void				AddConstraints(int32 element, int32 length,
									float min, float max, float preferred);
	virtual	void				SetWeight(int32 element, float weight);

	virtual	float				MinSize();
	virtual	float				MaxSize();
	virtual	float				PreferredSize();

	virtual	LayoutInfo*			CreateLayoutInfo();
	
	virtual	void				Layout(LayoutInfo* layoutInfo, float size);

	virtual	Layouter*			CloneLayouter();

private:
			class MyLayoutInfo;
			struct Constraint;
			struct SumItem;
			struct SumItemBackup;

			bool				_Layout(int32 size, SumItem* sums,
									int32* sizes);
			bool				_AddOptimizerConstraints();
			bool				_SatisfiesConstraints(int32* sizes) const;
			bool				_SatisfiesConstraintsSums(int32* sums) const;

			void				_ValidateLayout();
			void				_ApplyMaxConstraint(
									Constraint* currentConstraint, int32 index);
			void				_PropagateChanges(SumItem* sums, int32 toIndex,
									Constraint* lastMaxConstraint);
			void				_PropagateChangesBack(SumItem* sums,
									int32 changedIndex,
									Constraint* lastMaxConstraint);
		
			void				_BackupValues(int32 maxIndex);
			void				_RestoreValues(int32 maxIndex);

private:
			int32				fElementCount;
			int32				fSpacing;
			Constraint**		fConstraints;
			float*				fWeights;
			SumItem*			fSums;
			SumItemBackup*		fSumBackups;
			LayoutOptimizer*	fOptimizer;
			float				fMin;
			float				fMax;
			int32				fUnlimited;
			bool				fMinMaxValid;
			bool				fOptimizerConstraintsAdded;
};

}	// namespace Layout
}	// namespace BPrivate

using BPrivate::Layout::ComplexLayouter;


#endif	// COMPLEX_LAYOUTER_H
