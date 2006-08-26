/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	LAYOUTER_H
#define	LAYOUTER_H

#include <SupportDefs.h>

namespace BPrivate {
namespace Layout {

class LayoutInfo {
public:
								LayoutInfo();
	virtual						~LayoutInfo();

	virtual	float				ElementLocation(int32 element) = 0;
	virtual	float				ElementSize(int32 element) = 0;

	virtual	float				ElementRangeSize(int32 position, int32 length);
};


class Layouter {
public:
								Layouter();
	virtual						~Layouter();

	virtual	void				AddConstraints(int32 element, int32 length,
									float min, float max, float preferred) = 0;
	virtual	void				SetWeight(int32 element, float weight) = 0;

	virtual	float				MinSize() = 0;
	virtual	float				MaxSize() = 0;
	virtual	float				PreferredSize() = 0;

	virtual	LayoutInfo*			CreateLayoutInfo() = 0;
	
	virtual	void				Layout(LayoutInfo* layoutInfo, float size) = 0;

	virtual	Layouter*			CloneLayouter() = 0;
};


}	// namespace Layout
}	// namespace BPrivate

using BPrivate::Layout::LayoutInfo;
using BPrivate::Layout::Layouter;

#endif	// LAYOUTER_H
