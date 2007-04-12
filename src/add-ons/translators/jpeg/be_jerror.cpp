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
#include <stdio.h>

// JPEG headers
#include <jpeglib.h>
#include <jconfig.h>
#include <jerror.h>

// JPEGTtanslator settings header to get SETTINGS struct
#include "JPEGTranslator.h"


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

#if 0
	/* show error message */
	(new BAlert("JPEG Library Error", buffer, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
#endif

	/* Let the memory manager delete any temp files before we die */
	jpeg_destroy(cinfo);

	exit(B_ERROR);
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

#if 0
	/* If it's compressing or decompressing and user turned messages on */
	if (!cinfo->is_decompressor || cinfo->err->ShowReadWarnings)
		/* show warning message */
		(new BAlert("JPEG Library Warning", buffer, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
#endif
}


/*
 * Fill in the standard error-handling methods in a jpeg_error_mgr object.
 * Since Translator doesn't use it's own error table, we can use error_mgr's
 * variables to store some usefull data.
 * last_addon_message (as ShowReadWarnings) is used for storing SETTINGS->ShowReadWarningBox value
 */

GLOBAL(struct jpeg_error_mgr *)
be_jpeg_std_error (struct jpeg_error_mgr * err, jpeg_settings *settings)
{
	jpeg_std_error(err);

	err->error_exit = be_error_exit;
	err->output_message = be_output_message;

	err->ShowReadWarnings = settings->ShowReadWarningBox;

	return err;
}
