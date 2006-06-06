/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mikael Konradson, mikael.konradson@gmail.com
 */
#ifndef FONT_DEMO_H
#define FONT_DEMO_H


#include <Application.h>


class FontDemo : public BApplication {
	public:
		FontDemo();
		virtual ~FontDemo();

		virtual void ReadyToRun();
};

#endif	// FONT_DEMO_H
