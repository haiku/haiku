/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef B_JERROR_H
#define B_JERROR_H


#include <stdio.h>
	// for jpeglib.h -- it doesn't seem to be self-contained
#include <setjmp.h>

#include <jpeglib.h>


class TranslatorSettings;


struct be_jpeg_error_mgr : jpeg_error_mgr {
	const jmp_buf*	long_jump_buffer;
};


struct jpeg_error_mgr* be_jpeg_std_error(be_jpeg_error_mgr* err,
	TranslatorSettings* settings, const jmp_buf* longJumpBuffer);


#endif	// B_JERROR_H
