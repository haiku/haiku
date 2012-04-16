/*

Copyright (c) 2002-2003, Marcin 'Shard' Konicki
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation and/or
      other materials provided with the distribution.
    * Name "Marcin Konicki", "Shard" or any combination of them,
      must not be used to endorse or promote products derived from this
      software without specific prior written permission from Marcin Konicki.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


/* 
	Modified jerror.c from libjpeg
	to make it possible to turn on/off error dialog-box
	only two functions modified, rest is used from original error handling code
*/


// Be headers
#include <Alert.h>
#include <Catalog.h>
#include <stdio.h>

// JPEG headers
#include <jpeglib.h>
#include <jconfig.h>
#include <jerror.h>

// JPEGTtanslator settings header to get settings stuff
#include "JPEGTranslator.h"
#include "TranslatorSettings.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "be_jerror"

// Since Translator doesn't use it's own error table, we can use error_mgr's
// variables to store some usefull data.
// last_addon_message (as ShowReadWarnings) is used for storing SETTINGS->ShowReadWarningBox value
#define ShowReadWarnings last_addon_message


/*
 * Error exit handler: must not return to caller.
 */
GLOBAL(void)
be_error_exit (j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	/* Create the message */
	(*cinfo->err->format_message) (cinfo, buffer);

	fprintf(stderr, B_TRANSLATE("JPEG Library Error: %s\n"), buffer);

	jmp_buf longJumpBuffer;
	memcpy(&longJumpBuffer, &(cinfo->err->long_jump_buffer), sizeof(jmp_buf));

	/* Let the memory manager delete any temp files before we die */
	jpeg_destroy(cinfo);

	// jump back directly to the high level function's "breakpoint"
	longjmp(longJumpBuffer, 0);
}


/*
 * Actual output of an error or trace message.
 */
GLOBAL(void)
be_output_message (j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	/* Create the message */
	(*cinfo->err->format_message) (cinfo, buffer);

	cinfo->err->num_warnings++;

	/* If it's compressing or decompressing and user turned messages on */
	if (!cinfo->is_decompressor || cinfo->err->ShowReadWarnings) {
		/* show warning message */
		fprintf(stderr, B_TRANSLATE("JPEG Library Warning: %s\n"), buffer);
	}
}


/*
 * Fill in the standard error-handling methods in a jpeg_error_mgr object.
 * Since Translator doesn't use it's own error table, we can use error_mgr's
 * variables to store some usefull data.
 * last_addon_message (as ShowReadWarnings) is used for storing SETTINGS->ShowReadWarningBox value
 */

GLOBAL(struct jpeg_error_mgr *)
be_jpeg_std_error (struct jpeg_error_mgr * err, TranslatorSettings* settings,
	const jmp_buf* longJumpBuffer)
{
	settings->Acquire();
	jpeg_std_error(err);

	err->error_exit = be_error_exit;
	err->output_message = be_output_message;

	err->ShowReadWarnings = settings->SetGetBool(JPEG_SET_SHOWREADWARNING, NULL);
	memcpy(&(err->long_jump_buffer), longJumpBuffer, sizeof(jmp_buf));

	settings->Release();
	return err;
}
