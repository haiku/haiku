/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONVERT_H
#define CONVERT_H

#include <DataIO.h>

#include "RTF.h"


extern status_t convert_to_stxt(RTF::Header &header, BDataIO &target);
extern status_t convert_to_plain_text(RTF::Header &header, BPositionIO &target);
extern status_t convert_styled_text_to_rtf(
	BPositionIO* source, BPositionIO* target);
extern status_t convert_plain_text_to_rtf(
	BPositionIO& source, BPositionIO& target);
	
#endif	/* CONVERT_H */
