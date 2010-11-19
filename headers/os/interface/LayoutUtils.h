/*
 * Copyright 2006, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_LAYOUT_UTILS_H
#define	_LAYOUT_UTILS_H

#include <Alignment.h>
#include <Rect.h>
#include <Size.h>

class BView;

class BLayoutUtils {
public:
//	static	float				AddSizesFloat(float a, float b);
//	static	float				AddSizesFloat(float a, float b, float c);
	static	float				AddDistances(float a, float b);
	static	float				AddDistances(float a, float b, float c);
	static	int32				AddSizesInt32(int32 a, int32 b);
	static	int32				AddSizesInt32(int32 a, int32 b, int32 c);
//	static	float				SubtractSizesFloat(float a, float b);
	static	int32				SubtractSizesInt32(int32 a, int32 b);
	static	float				SubtractDistances(float a, float b);

	static	void				FixSizeConstraints(float& min, float& max,
									float& preferred);
	static	void				FixSizeConstraints(BSize& min, BSize& max,
									BSize& preferred);

	static	BSize				ComposeSize(BSize size, BSize layoutSize);
	static	BAlignment			ComposeAlignment(BAlignment alignment,
									BAlignment layoutAlignment);

	static	BRect				AlignInFrame(BRect frame, BSize maxSize,
									BAlignment alignment);
	static	void				AlignInFrame(BView* view, BRect frame);
	static	BRect				MoveIntoFrame(BRect rect, BSize frameSize);
};

#endif	//	_LAYOUT_UTILS_H
