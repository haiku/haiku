/*
 * Copyright 2006-2009 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Timothy Wayper <timmy@wunderbear.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef CALC_OPTIONS_H
#define CALC_OPTIONS_H

#include <SupportDefs.h>

class BMessage;

struct CalcOptions {
	bool auto_num_lock;		// automatically activate numlock
	bool audio_feedback;	// provide audio feedback
	bool show_keypad;		// show or hide the buttons

				CalcOptions();

	void		LoadSettings(const BMessage* archive);
	status_t	SaveSettings(BMessage* archive) const;
};

#endif // CALC_OPTIONS_H
