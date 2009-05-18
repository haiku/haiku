/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */
#ifndef HVIF_VIEW_H
#define HVIF_VIEW_H

#include <View.h>

class HVIFView : public BView {
public:
							HVIFView(const BRect &frame, const char *name,
								uint32 resizeMode, uint32 flags);
};

#endif	// HVIF_VIEW_H
