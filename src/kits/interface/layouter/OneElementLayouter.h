/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	ONE_ELEMENT_LAYOUTER_H
#define	ONE_ELEMENT_LAYOUTER_H

#include "Layouter.h"

namespace BPrivate {
namespace Layout {

class OneElementLayouter : public Layouter {
public:
								OneElementLayouter();
	virtual						~OneElementLayouter();

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

			float				fMin;
			float				fMax;
			float				fPreferred;
};

}	// namespace Layout
}	// namespace BPrivate

using BPrivate::Layout::OneElementLayouter;

#endif	// ONE_ELEMENT_LAYOUTER_H
