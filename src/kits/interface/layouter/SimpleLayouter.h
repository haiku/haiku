/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	SIMPLE_LAYOUTER_H
#define	SIMPLE_LAYOUTER_H

#include "Layouter.h"

class BList;

namespace BPrivate {
namespace Layout {

class SimpleLayouter : public Layouter {
public:
								SimpleLayouter(int32 elementCount,
									int32 spacing);
	virtual						~SimpleLayouter();

	virtual	void				AddConstraints(int32 element, int32 length,
									float min, float max, float preferred);
	virtual	void				SetWeight(int32 element, float weight);

	virtual	float				MinSize();
	virtual	float				MaxSize();
	virtual	float				PreferredSize();

	virtual	LayoutInfo*			CreateLayoutInfo();
	
	virtual	void				Layout(LayoutInfo* layoutInfo, float size);

	virtual	Layouter*			CloneLayouter();

	static	void				DistributeSize(int32 size, float weights[],
									int32 sizes[], int32 count);

private:
	static	long				_CalculateSumWeight(BList& elementInfos);
	
			void				_ValidateMinMax();
			void				_LayoutMax();
			void				_LayoutStandard();

private:
			class ElementLayoutInfo;
			class ElementInfo;
			class MyLayoutInfo;

			int32				fElementCount;
			int32				fSpacing;
			ElementInfo*		fElements;

			int32				fMin;
			int32				fMax;
			int32				fPreferred;

			bool				fMinMaxValid;

			MyLayoutInfo*		fLayoutInfo;
};

}	// namespace Layout
}	// namespace BPrivate

using BPrivate::Layout::SimpleLayouter;

#endif	// SIMPLE_LAYOUTER_H
