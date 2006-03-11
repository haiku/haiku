/*
 * Copyright 2002-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef STXT_VIEW_H
#define STXT_VIEW_H


#include "TranslatorSettings.h"

#include <View.h>


class STXTView : public BView {
	public:
		STXTView(const BRect &frame, const char *name, uint32 resizeMode,
			uint32 flags, TranslatorSettings *settings);
		virtual ~STXTView();

	private:
		TranslatorSettings *fSettings;
};

#endif	// STXT_VIEW_H
