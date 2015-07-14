/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay
 */


// This command line program was created in order to be able to render
// HVIF files and then save the resultant bitmap into a PNG image file.
// The tool can be compiled for linux and was initially created for
// use with the Haiku Depot Server application server so that it was
// able to render HVIFs in the web page. 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <png.h>

#include "AutoDeleter.h"
#include "Bitmap.h"
#include "IconUtils.h"
#include "InterfaceDefs.h"
#include "SupportDefs.h"


#define SIZE_HVIF_BUFFER_STEP 1024


typedef struct h2p_hvif_buffer {
	uint8* buffer;
	size_t used;
	size_t allocated;
} h2p_hvif_buffer;


typedef struct h2p_parameters {
	int size;
	char* in_filename;
	char* out_filename;
} h2p_parameters;


typedef struct h2p_state {
	FILE* in;
	FILE* out;
	BBitmap* bitmap;
	h2p_hvif_buffer hvif_buffer;
	h2p_parameters params;
} h2p_state;


static int
h2p_fprintsyntax(FILE* stream)
{
		return fprintf(
			stream, 
			"syntax: hvif2png -s <size> [-i <input-file>] [-o <output-file>]\n");
}


static void
h2p_close_state(h2p_state* state)
{
	if (state->hvif_buffer.buffer != NULL)
		free(state->hvif_buffer.buffer);

	if (state->in != NULL)
		fclose(state->in);

	if (state->out != NULL)
		fclose(state->out);
}


static void fclose_quietly(FILE** f) {
	fclose(*f);
}


/*! Opens the input and output streams for the conversion
    \return 0 if there was some problem in opening the streams; otherwise
    	non-zero.
*/

static int
h2p_open_streams(h2p_state* state)
{
	if (state->params.in_filename != NULL)
		state->in = fopen(state->params.in_filename, "rb");
	else
		state->in = stdin;

	if (state->in == NULL) {
		fprintf(
			stderr,
			"unable to open the input file; '%s'\n",
			state->params.in_filename);
		return 0;
	}

	CObjectDeleter<FILE *> inDeleter(&(state->in), fclose_quietly); 

	if (state->params.out_filename != NULL)
		state->out = fopen(state->params.out_filename, "wb");
	else
		state->out = stdout;

	if (state->out == NULL) {
		fprintf(
			stderr,
			"unable to open the output file; '%s'\n",
			state->params.out_filename);
		return 0;
	}

	inDeleter.Detach();

	return 1;
}


static int
h2p_write_png(BBitmap* bitmap, FILE* out)
{
	int result = 0;

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png == NULL)
		fprintf(stderr, "unable to setup png write data structures\n");
	else {

		png_init_io(png, out);
		png_infop info = png_create_info_struct(png);

		if (info == NULL)
			fprintf(stderr, "unable to setup png info data structures\n");
		else {
			BRect rect = bitmap->Bounds();
			png_uint_32 width = (png_uint_32) rect.Width()+1;
			png_uint_32 height = (png_uint_32) rect.Height()+1;

			png_set_IHDR(
				png, info, width, height,
				8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

			png_set_bgr(png);

			png_write_info(png, info);

			uint8 *bitmapData = (uint8 *)bitmap->Bits();
			int32 bitmapBytesPerRow = bitmap->BytesPerRow();

			for (png_uint_32 i = 0; i < height;i++) {
				png_write_row(
					png,
					(png_bytep)&bitmapData[i * bitmapBytesPerRow]);
			}

			png_write_end(png, NULL);

			result = 1;
		}

		if (info != NULL)
			png_free_data(png, info, PNG_FREE_ALL, -1);
	}

	if (png != NULL)
		png_destroy_write_struct(&png, (png_infopp)NULL);

	return result;
}


/*! Reads the HVIF input data from the supplied input file.
    \return the quantity of bytes that were read from the
    	HVIF file or 0 if there was a problem reading the HVIF data.
*/

static size_t
h2p_read_hvif_input(h2p_hvif_buffer* result, FILE* in)
{
	result->buffer = (uint8 *)malloc(SIZE_HVIF_BUFFER_STEP);
	result->allocated = SIZE_HVIF_BUFFER_STEP;
	result->used = 0;

	if (result->buffer == NULL) {
		fprintf(stderr,"out of memory\n");
		return 0;
	}

	while (!feof(in)) {
		if (result->used == result->allocated) {
			result->buffer = (uint8 *)realloc(
				result->buffer,
				result->allocated + SIZE_HVIF_BUFFER_STEP);

			if (result->buffer == NULL) {
				fprintf(stderr,"out of memory\n");
				return 0;
			}
		}

		result->used += fread(
			&result->buffer[result->used],
			sizeof(uint8),
			result->allocated - result->used, in);

		int err = ferror(in);

		if (err != 0) {
			fprintf(stderr, "error reading input; %s\n", strerror(err));
			return 0;
		}
	}

	return result->used;
}


/*! Parse the arguments to the conversion program from the command line.
		\return 0 if there was a problem reading the parameters and non-zero
    	otherwise.
*/

static int
h2p_parse_args(h2p_parameters* result, int argc, char* argv[])
{
	for (int i = 1;i < argc;) {

		if (argv[i][0] != '-') {
			fprintf(stderr, "was expecting a switch; found '%s'\n",argv[i]);
			h2p_fprintsyntax(stderr);
			return 0;
		}

		if (strlen(argv[i]) != 2) {
			fprintf(stderr, "illegal switch; '%s'\n",argv[i]);
			h2p_fprintsyntax(stderr);
			return 0;
		}

		switch (argv[i][1]) {

			case 's':
				if (i == argc - 1) {
					fprintf(stderr,"the size has not been specified\n");
					h2p_fprintsyntax(stderr);
					return 0;
				}  

				result->size = atoi(argv[i + 1]);

				if (result->size <= 0 || result->size > 1024) {
					fprintf(stderr,"bad size specified; '%s'\n", argv[i]);
					h2p_fprintsyntax(stderr);
					return 0;
				}

				i+=2;

				break;

			case 'i':
				if (i == argc - 1) {
					fprintf(stderr,"the input filename has not been specified\n");
					h2p_fprintsyntax(stderr);
					return 0;
				}

				result->in_filename = argv[i + 1];
				i+=2;
				break;

			case 'o':

				if (i == argc - 1) {
					fprintf(stderr,"the output filename has not been specified\n");
					h2p_fprintsyntax(stderr);
					return 0;
				}

				result->out_filename = argv[i + 1];
				i += 2;
				break;

			default:
				fprintf(stderr, "unrecognized switch; '%s'\n", argv[i]);
				h2p_fprintsyntax(stderr);
				return 0;
		}

	}

	if (result->size == 0) {
		fprintf(stderr, "size has not been specified\n");
		h2p_fprintsyntax(stderr);
		return 0;
	}
 
	return 1;
}


int
main(int argc, char* argv[])
{
	int exitResult = 1;

	if (argc == 1)
		h2p_fprintsyntax(stderr);
	else {

		h2p_state state;
		bzero(&state, sizeof(state));

		if (h2p_parse_args(&(state.params), argc, argv)) {

			if (h2p_open_streams(&state)) {

				if (h2p_read_hvif_input(&(state.hvif_buffer), state.in) > 0) {

					// create the bitmap and then parse and render the HVIF icon data
					// into the bitmap.

					state.bitmap = new BBitmap(
						BRect(0.0, 0.0, state.params.size - 1, state.params.size - 1),
						B_RGBA32); // actual storage is BGRA

					status_t gviStatus = BIconUtils::GetVectorIcon(
						state.hvif_buffer.buffer,
						state.hvif_buffer.used,
						state.bitmap);

					if (gviStatus != B_OK) {
						fprintf(
							stderr,
							"the hvif data (%zdB) was not able to be parsed / rendered\n",
							state.hvif_buffer.used);
					}
					else {
						// write the bitmap data out again as a PNG.
						if (h2p_write_png(state.bitmap, state.out))
							exitResult = 0;
					}
				}

			}
		}

		// clean up
		h2p_close_state(&state);
	}

	return exitResult;

}
