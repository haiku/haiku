/*
 * Copyright 2006-2009 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Timothy Wayper <timmy@wunderbear.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CalcOptions.h"

#include <stdlib.h>
#include <stdio.h>

#include <Message.h>


CalcOptions::CalcOptions()
	: auto_num_lock(false),
	  audio_feedback(false),
	  show_keypad(true)
{
}


void
CalcOptions::LoadSettings(const BMessage* archive)
{
	bool option;

	if (archive->FindBool("auto num lock", &option) == B_OK)
		auto_num_lock = option;

	if (archive->FindBool("audio feedback", &option) == B_OK)
		audio_feedback = option;

	if (archive->FindBool("show keypad", &option) == B_OK)
		show_keypad = option;
}


status_t
CalcOptions::SaveSettings(BMessage* archive) const
{
	status_t ret = archive->AddBool("auto num lock", auto_num_lock);

	if (ret == B_OK)
		ret = archive->AddBool("audio feedback", audio_feedback);

	if (ret == B_OK)
		ret = archive->AddBool("show keypad", show_keypad);

	return ret;
}

