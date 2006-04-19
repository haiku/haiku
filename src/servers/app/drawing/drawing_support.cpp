#include "drawing_support.h"

#include <Rect.h>

void
align_rect_to_pixels(BRect* rect)
{
	// round the rect with the least ammount of distortion
	rect->OffsetTo(roundf(rect->left), roundf(rect->top));
	rect->right = roundf(rect->right);
	rect->bottom = roundf(rect->bottom);
}

