/*
 * Copyright 2002-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef BMP_VIEW_H
#define BMP_VIEW_H


#include "TranslatorSettings.h"

#include <View.h>


class BMPView : public BView {
	public:
		BMPView(const BRect &frame, const char *name, uint32 resizeMode,
			uint32 flags, TranslatorSettings *settings);
		virtual ~BMPView();

	private:
		TranslatorSettings *fSettings;
};

#endif	// BMP_VIEW_H
