/*
 * Copyright 2011, Haiku, Inc.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef COLLAPSING_LAYOUTER_H	
#define COLLAPSING_LAYOUTER_H

#include "Layouter.h"


namespace BPrivate {
namespace Layout {


/* This layouter wraps either a Compound, Simple or OneElement layouter, and
 * removes elements which have no constraints, or min/max constraints of
 * B_SIZE_UNSET. The child layouter is given only the constraints for the
 * remaining elements. When using the LayoutInfo of this layouter,
 * collapsed (removed) elements are given no space on screen.
 */
class CollapsingLayouter : public Layouter {
public:
								CollapsingLayouter(int32 elementCount,
									float spacing);
	virtual						~CollapsingLayouter();

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
	class	ProxyLayoutInfo;
	struct	Constraint;
	struct	ElementInfo;

			void				_ValidateLayouter();
			Layouter*			_CreateLayouter();
			void				_DoCollapse();
			void				_AddConstraints();
			void				_AddConstraints(int32 position,
									const Constraint* c);
			void				_SetWeights();

			int32				fElementCount;
			ElementInfo*		fElements;
			int32				fValidElementCount;
			bool				fHaveMultiElementConstraints;
			float				fSpacing;
			Layouter*			fLayouter;
};

} // namespace Layout
} // namespace BPrivate

using BPrivate::Layout::CollapsingLayouter;

#endif // COLLAPSING_LAYOUTER_H
