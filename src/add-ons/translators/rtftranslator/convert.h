/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONVERT_H
#define CONVERT_H


#include <RTF.h>
#include <DataIO.h>


extern status_t convert_to_stxt(RTFHeader &header, BDataIO &target);
extern status_t convert_to_plain_text(RTFHeader &header, BPositionIO &target);

#endif	/* CONVERT_H */
