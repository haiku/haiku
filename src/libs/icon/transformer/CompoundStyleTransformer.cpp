/*
 * Copyright 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Zardshard
 */


#include "CompoundStyleTransformer.h"

#include <string.h>


_USING_ICON_NAMESPACE


CompoundStyleTransformer::CompoundStyleTransformer(
		StyleTransformer** transformers, int32 count)
	:
	fTransformers(transformers),
	fCount(count)
{
}


CompoundStyleTransformer::~CompoundStyleTransformer()
{
	for (int i = 0; i < fCount; i++)
		delete fTransformers[i];
	delete[] fTransformers;
}


void
CompoundStyleTransformer::transform(double* x, double* y) const
{
	for (int i = 0; i < fCount; i++) {
		if (fTransformers[i] != NULL)
			fTransformers[i]->transform(x, y);
	}
}


void
CompoundStyleTransformer::Invert()
{
	// reverse order of pipeline
	StyleTransformer* oldOrder[fCount];
	memcpy(oldOrder, fTransformers, fCount * sizeof(StyleTransformer*));
	for (int i = 0; i < fCount; i++) {
		fTransformers[fCount-i-1] = oldOrder[i];
	}

	// invert individual transformations
	for (int i = 0; i < fCount; i++) {
		if (fTransformers[i] != NULL)
			fTransformers[i]->Invert();
	}
}


bool
CompoundStyleTransformer::IsLinear()
{
	bool linear = true;
	for (int i = 0; i < fCount; i++) {
		if (fTransformers[i] != NULL)
			linear &= fTransformers[i]->IsLinear();
	}
	return linear;
}
