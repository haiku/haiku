/*
 * "$Id: pcl-unprint.c,v 1.11 2004/09/17 18:38:28 rleigh Exp $"
 *
 *   pclunprint.c - convert an HP PCL file into an image file for viewing.
 *
 *   Copyright 2000 Dave Hill (dave@minnie.demon.co.uk)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/util.h>
#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>

static const char *id="@(#) $Id: pcl-unprint.c,v 1.11 2004/09/17 18:38:28 rleigh Exp $";

/*
 * Size of buffer used to read file
 */
#define READ_SIZE 1024

/*
 * Largest data attached to a command. 1024 means that we can have up to 8192
 * pixels in a row
 */
#define MAX_DATA 1024

FILE *read_fd,*write_fd;
char read_buffer[READ_SIZE];
char data_buffer[MAX_DATA];
char initial_command[3];
int initial_command_index;
char final_command;
int numeric_arg;

int read_pointer;
int read_size;
int eof;
int combined_command = 0;
int skip_output = 0;

/*
 * Data about the image
 */

typedef struct {
    int colour_type;		/* Mono, 3/4 colour */
    int black_depth;		/* 2 level, 4 level */
    int cyan_depth;		/* 2 level, 4 level */
    int magenta_depth;		/* 2 level, 4 level */
    int yellow_depth;		/* 2 level, 4 level */
    int lcyan_depth;		/* 2 level, 4 level */
    int lmagenta_depth;		/* 2 level, 4 level */
    int image_width;
    int image_height;
    int compression_type;	/* Uncompressed or TIFF */
} image_t;

/*
 * collected data read from file
 */

typedef struct {
    char **black_bufs;			/* Storage for black rows */
    int black_data_rows_per_row;	/* Number of black rows */
    char **cyan_bufs;
    int cyan_data_rows_per_row;
    char **magenta_bufs;
    int magenta_data_rows_per_row;
    char **yellow_bufs;
    int yellow_data_rows_per_row;
    char **lcyan_bufs;
    int lcyan_data_rows_per_row;
    char **lmagenta_bufs;
    int lmagenta_data_rows_per_row;
    int buffer_length;
    int active_length;			/* Length of output data */
    int output_depth;
} output_t;

#define PCL_MONO 1
#define PCL_CMY 3
#define PCL_CMYK 4
#define PCL_CMYKcm 6

#define PCL_COMPRESSION_NONE 0
#define PCL_COMPRESSION_RUNLENGTH 1
#define PCL_COMPRESSION_TIFF 2
#define PCL_COMPRESSION_DELTA 3
#define PCL_COMPRESSION_CRDR 9		/* Compressed row delta replacement */

/* PCL COMMANDS */

typedef enum {
	PCL_RESET = 1,PCL_MEDIA_SIZE,PCL_PERF_SKIP,PCL_TOP_MARGIN,PCL_MEDIA_TYPE,
	PCL_MEDIA_SOURCE,PCL_SHINGLING,PCL_RASTERGRAPHICS_QUALITY,PCL_DEPLETION,
	PCL_CONFIGURE,PCL_RESOLUTION,PCL_COLOURTYPE,PCL_COMPRESSIONTYPE,
	PCL_LEFTRASTER_POS,PCL_TOPRASTER_POS,PCL_RASTER_WIDTH,PCL_RASTER_HEIGHT,
	PCL_START_RASTER,PCL_END_RASTER,PCL_END_COLOUR_RASTER,PCL_DATA,PCL_DATA_LAST,
	PCL_PRINT_QUALITY,PCL_ENTER_PJL,PCL_GRAY_BALANCE,PCL_DRIVER_CONFIG,
	PCL_PAGE_ORIENTATION,PCL_VERTICAL_CURSOR_POSITIONING_BY_DOTS,
	PCL_HORIZONTAL_CURSOR_POSITIONING_BY_DOTS,PCL_UNIT_OF_MEASURE,
	PCL_RELATIVE_VERTICAL_PIXEL_MOVEMENT,PCL_PALETTE_CONFIGURATION,
	PCL_LPI,PCL_CPI,PCL_PAGE_LENGTH,PCL_NUM_COPIES,PCL_DUPLEX,
	PCL_MEDIA_SIDE,RTL_CONFIGURE,PCL_ENTER_PCL,PCL_ENTER_HPGL2,
	PCL_NEGATIVE_MOTION,PCL_MEDIA_DEST,PCL_JOB_SEPARATION,
	PCL_LEFT_OFFSET_REGISTRATION,PCL_TOP_OFFSET_REGISTRATION,
	PCL_PRINT_DIRECTION,PCL_LEFT_MARGIN,PCL_RIGHT_MARGIN,
	PCL_RESET_MARGINS,PCL_TEXT_LENGTH
} command_t;

typedef struct {
  const char initial_command[3];		/* First part of command */
  const char final_command;			/* Last part of command */
  int has_data;					/* Data follows */
  command_t command;				/* Command name */
  const char *description;			/* Text for printing */
} commands_t;

const commands_t pcl_commands[] =
    {
/* Two-character sequences ESC <x> */
	{ "E", '\0', 0, PCL_RESET, "PCL RESET" },
	{ "9", '\0', 0, PCL_RESET_MARGINS, "Reset Margins" },
	{ "%", 'A', 0, PCL_ENTER_PCL, "PCL mode" },
	{ "%", 'B', 0, PCL_ENTER_HPGL2, "HPGL/2 mode" },
	{ "%", 'X', 0, PCL_ENTER_PJL, "UEL/Enter PJL mode" },
/* Parameterised sequences */
/* Raster positioning */
	{ "&a", 'G', 0, PCL_MEDIA_SIDE, "Set Media Side" },
	{ "&a", 'H', 0, PCL_LEFTRASTER_POS, "Left Raster Position" },
	{ "&a", 'L', 0, PCL_LEFT_MARGIN, "Left Margin by Column" },
	{ "&a", 'M', 0, PCL_RIGHT_MARGIN, "Right Margin by Column" },
	{ "&a", 'N', 0, PCL_NEGATIVE_MOTION, "Negative Motion" },
	{ "&a", 'P', 0, PCL_PRINT_DIRECTION, "Print Direction" },
	{ "&a", 'V', 0, PCL_TOPRASTER_POS, "Top Raster Position" },
/* Characters */
	{ "&k", 'H', 0, PCL_CPI, "Characters per Inch" },
/* Media */
	{ "&l", 'A', 0, PCL_MEDIA_SIZE , "Media Size" },
	{ "&l", 'D', 0, PCL_LPI , "Lines per Inch" },
	{ "&l", 'E', 0, PCL_TOP_MARGIN , "Top Margin" },
	{ "&l", 'F', 0, PCL_TEXT_LENGTH , "Text Length" },
	{ "&l", 'G', 0, PCL_MEDIA_DEST, "Media Destination" },
	{ "&l", 'H', 0, PCL_MEDIA_SOURCE, "Media Source" },
	{ "&l", 'L', 0, PCL_PERF_SKIP , "Perf. Skip" },
	{ "&l", 'M', 0, PCL_MEDIA_TYPE , "Media Type" },
	{ "&l", 'O', 0, PCL_PAGE_ORIENTATION, "Page Orientation" },
	{ "&l", 'P', 0, PCL_PAGE_LENGTH, "Page Length in Lines" },
	{ "&l", 'S', 0, PCL_DUPLEX, "Duplex mode" },
	{ "&l", 'T', 0, PCL_JOB_SEPARATION, "Job Separation" },
	{ "&l", 'U', 0, PCL_LEFT_OFFSET_REGISTRATION, "Left Offset Registration" },
	{ "&l", 'X', 0, PCL_NUM_COPIES, "Number of copies" },
	{ "&l", 'Z', 0, PCL_TOP_OFFSET_REGISTRATION, "Top Offset Registration" },
/* Units */
	{ "&u", 'D', 0, PCL_UNIT_OF_MEASURE, "Unit of Measure" },	/* from bpd05446 */
/* Raster data */
	{ "*b", 'B', 0, PCL_GRAY_BALANCE, "Gray Balance" },	/* from PCL Developer's Guide 6.0 */
	{ "*b", 'M', 0, PCL_COMPRESSIONTYPE, "Compression Type" },
	{ "*b", 'V', 1, PCL_DATA, "Data, intermediate" },
	{ "*b", 'W', 1, PCL_DATA_LAST, "Data, last" },
	{ "*b", 'Y', 0, PCL_RELATIVE_VERTICAL_PIXEL_MOVEMENT, "Relative Vertical Pixel Movement" },
/* Palette */
	{ "*d", 'W', 1, PCL_PALETTE_CONFIGURATION, "Palette Configuration" },
/* Plane configuration */
	{ "*g", 'W', 1, PCL_CONFIGURE, "Configure Raster Data" },
/* Raster Graphics */
	{ "*o", 'D', 0, PCL_DEPLETION, "Depletion" },
	{ "*o", 'M', 0, PCL_PRINT_QUALITY, "Print Quality" },
	{ "*o", 'Q', 0, PCL_SHINGLING, "Raster Graphics Shingling" },
	{ "*o", 'W', 1, PCL_DRIVER_CONFIG, "Driver Configuration Command" },
/* Cursor Positioning */
	{ "*p", 'X', 0, PCL_HORIZONTAL_CURSOR_POSITIONING_BY_DOTS, "Horizontal Cursor Positioning by Dots" },
	{ "*p", 'Y', 0, PCL_VERTICAL_CURSOR_POSITIONING_BY_DOTS, "Vertical Cursor Positioning by Dots" },
/* Raster graphics */
	{ "*r", 'A', 0, PCL_START_RASTER, "Start Raster Graphics" },
	{ "*r", 'B', 0, PCL_END_RASTER, "End Raster Graphics"},
	{ "*r", 'C', 0, PCL_END_COLOUR_RASTER, "End Colour Raster Graphics" },
	{ "*r", 'Q', 0, PCL_RASTERGRAPHICS_QUALITY, "Raster Graphics Quality" },
	{ "*r", 'S', 0, PCL_RASTER_WIDTH, "Raster Width" },
	{ "*r", 'T', 0, PCL_RASTER_HEIGHT, "Raster Height" },
	{ "*r", 'U', 0, PCL_COLOURTYPE, "Colour Type" },
/* Resolution */
	{ "*t", 'R', 0, PCL_RESOLUTION, "Resolution" },
/* RTL/PCL5 */
	{ "*v", 'W', 1, RTL_CONFIGURE, "RTL Configure Image Data" },
   };

int pcl_find_command (void);
void fill_buffer (void);
void pcl_read_command (void);
void write_grey (output_t *output, image_t *image);
void write_colour (output_t *output, image_t *image);
int decode_tiff (char *in_buffer, int data_length, char *decode_buf,
                 int maxlen);
void pcl_reset (image_t *i);
int depth_to_rows (int depth);


/*
 * pcl_find_command(). Search the commands table for the command.
 */

int pcl_find_command(void)
{

    int num_commands = sizeof(pcl_commands) / sizeof(commands_t);
    int i;

    for (i=0; i < num_commands; i++) {
	if ((strcmp(initial_command, pcl_commands[i].initial_command) == 0) &&
	    (final_command == pcl_commands[i].final_command))
		return(i);
    }

    return (-1);
}

/*
 * fill_buffer() - Read a new chunk from the input file
 */

void fill_buffer(void)
{

    if ((read_pointer == -1) || (read_pointer >= read_size)) {
	read_size = (int) fread(&read_buffer, sizeof(char), READ_SIZE, read_fd);

#ifdef DEBUG
	fprintf(stderr, "Read %d characters\n", read_size);
#endif

	if (read_size == 0) {
#ifdef DEBUG
	    fprintf(stderr, "No More to read!\n");
#endif
	    eof = 1;
	    return;
	}
	read_pointer = 0;
    }
}

/*
 * pcl_read_command() - Read the data stream and parse the next PCL
 * command.
 */

void pcl_read_command(void)
{

    char c;
    int minus;
    int skipped_chars;

/*
   Precis from the PCL Developer's Guide 6.0:-

   There are two formats for PCL commands; "Two Character" and
   "Parameterised".

   A "Two Character" command is: ESC <x>, where <x> has a decimal
   value between 48 and 126 (inclusive).

   A "Parameterised" command is: ESC <x> <y> [n] <z>
   where x, y and z are characters, and [n] is an optional number.
   The character <x> has a decimal value between 33 and 47 (incl).
   The character <y> has a decimal value between 96 and 126 (incl).

   Some commands are followed by data, in this case, [n] is the
   number of bytes to read, otherwise <n> is a numeric argument.
   The number <n> can consist of +, - and 0-9 (and .).

   The character <z> is either in the range 64-94 for a termination
   or 96-126 for a combined command. (The ref guide gives these
   as "96-126" and "64-96" which cannot be right as 96 appears twice!)

   It is possible to combine parameterised commands if the
   command prefix is the same, e.g. ESC & l 26 a 0 L is the same as
   ESC & l 26 A ESC & l 0 L. The key to this is that the terminator for
   the first command is in the range 96-126 (lower case).

   There is a problem with the "escape command" (ESC %) as it does not
   conform to this specification, so we have to check for it specifically!
*/

#define PCL_DIGIT(c) (((unsigned int) c >= 48) && ((unsigned int) c <= 57))
#define PCL_TWOCHAR(c) (((unsigned int) c >= 48) && ((unsigned int) c <= 126))
#define PCL_PARAM(c) (((unsigned int) c >= 33) && ((unsigned int) c <= 47))
#define PCL_PARAM2(c) (((unsigned int) c >= 96) && ((unsigned int) c <= 126))
#define PCL_COMBINED_TERM(c) (((unsigned int) c >= 96) && ((unsigned int) c <= 126))
#define PCL_TERM(c) (((unsigned int) c >= 64) && ((unsigned int) c <= 94))
#define PCL_CONVERT_TERM(c) (c - (char) 32)

    numeric_arg=0;
    minus = 0;
    final_command = '\0';

    fill_buffer();
    if (eof == 1)
	return;

    c = read_buffer[read_pointer++];
#ifdef DEBUG
    fprintf(stderr, "Got %c\n", c);
#endif

/*
 * If we are not in a "combined command", we are looking for ESC
 */

    if (combined_command == 0) {

	if(c != (char) 0x1b) {

	    fprintf(stderr, "ERROR: No ESC found (out of sync?) searching... ");

/*
 * all we can do is to chew through the file looking for another ESC.
 */

	    skipped_chars = 0;
	    while (c != (char) 0x1b) {
		if (c == (char) 0x0c)
		    fprintf(stderr, "FF ");
		skipped_chars++;
		fill_buffer();
		if (eof == 1) {
		    fprintf(stderr, "ERROR: EOF looking for ESC!\n");
		    return;
		}
		c = read_buffer[read_pointer++];
	    }
	    fprintf(stderr, "%d characters skipped.\n", skipped_chars);
	}

/*
 * We got an ESC, process normally
 */

	initial_command_index=0;
	initial_command[initial_command_index] = '\0';
	fill_buffer();
	if (eof == 1) {
	    fprintf(stderr, "ERROR: EOF after ESC!\n");
	    return;
	}

/* Get first command letter */

	c = read_buffer[read_pointer++];
	initial_command[initial_command_index++] = c;

#ifdef DEBUG
	fprintf(stderr, "Got %c\n", c);
#endif

/* Check to see if this character forms a "two character" command,
   or is a special command. */

	if (PCL_TWOCHAR(c)) {
#ifdef DEBUG
	    fprintf(stderr, "Two character command\n");
#endif
	    initial_command[initial_command_index] = '\0';
	    return;
	}	/* two character check */

/* Now check for a "parameterised" sequence. */

	else if (PCL_PARAM(c)) {
#ifdef DEBUG
	    fprintf(stderr, "Parameterised command\n");
#endif

/* Get the next character in the command */

	    fill_buffer();
	    if (eof == 1) {

#ifdef DEBUG
		fprintf(stderr, "EOF in middle of command!\n");
#endif
		eof = 0;		/* Ignore it */
		initial_command[initial_command_index] = '\0';
		return;
	    }
	    c = read_buffer[read_pointer++];
#ifdef DEBUG
	    fprintf(stderr, "Got %c\n", c);
#endif

/* Check that it is legal and store it */

	    if (PCL_PARAM2(c)) {
		initial_command[initial_command_index++] = c;
		initial_command[initial_command_index] = '\0';

/* Get the next character in the command then fall into the numeric part */

		fill_buffer();
		if (eof == 1) {

#ifdef DEBUG
		    fprintf(stderr, "EOF in middle of command!\n");
#endif
		    eof = 0;		/* Ignore it */
		    return;
		}
		c = read_buffer[read_pointer++];
#ifdef DEBUG
		fprintf(stderr, "Got %c\n", c);
#endif

	    }
	    else {
/* The second character is not legal. If the first character is '%' then allow it
 * through */

		    if (initial_command[0] == '%') {
#ifdef DEBUG
			fprintf(stderr, "ESC%% commmand\n");
#endif
			initial_command[initial_command_index] = '\0';
		    }
		    else {
			fprintf(stderr, "ERROR: Illegal second character %c in parameterised command.\n",
		    c);
			initial_command[initial_command_index] = '\0';
			return;
		    }
	    }
	}	/* Parameterised check */

/* If we get here, the command is illegal */

	else {
	    fprintf(stderr, "ERROR: Illegal first character %c in command.\n",
		c);
	    initial_command[initial_command_index] = '\0';
	    return;
	}
    }						/* End of (combined_command) */

/*
   We get here if either this is a combined sequence, or we have processed
   the beginning of a parameterised sequence. There is an optional number
   next, which may be preceeded by "+" or "-". FIXME We should also handle
   decimal points.
*/

    if ((c == '-') || (c == '+') || (PCL_DIGIT(c))) {
	if (c == '-')
	    minus = 1;
	else if (c == '+')
	    minus = 0;
	else
	    numeric_arg = (int) (c - '0');

/* Continue until non-numeric seen */

	while (1) {
	    fill_buffer();
	    if (eof == 1) {
		fprintf(stderr, "ERROR: EOF in middle of command!\n");
		return;
	    }
	    c = read_buffer[read_pointer++];

#ifdef DEBUG
	    fprintf(stderr, "Got %c\n", c);
#endif

	    if (! PCL_DIGIT(c)) {
		break;		/* End of loop */
	    }
	    numeric_arg = (10 * numeric_arg) + (int) (c - '0');
	}
    }

/*
   We fell out of the loop when we read a non-numeric character.
   Treat this as the terminating character and check for a combined
   command. We should check that the letter is a valid terminator,
   but it doesn't matter as we'll just not recognize the command!
 */

    combined_command = (PCL_COMBINED_TERM(c) != 0);
    if (combined_command == 1) {
#ifdef DEBUG
	fprintf(stderr, "Combined command\n");
#endif
	final_command = PCL_CONVERT_TERM(c);
    }
    else
	final_command = c;

    if (minus == 1)
	numeric_arg = -numeric_arg;

    return;
}

/*
 * write_grey() - write out one line of mono PNM image
 */

/* FIXME - multiple levels */

void write_grey(output_t *output,	/* I: data */
		image_t *image)		/* I: Image data */
{
    int wholebytes = image->image_width / 8;
    int crumbs = image->image_width - (wholebytes * 8);
    char *buf = output->black_bufs[0];

    int i, j;
    char tb[8];

#ifdef DEBUG
    fprintf(stderr, "Data Length: %d, wholebytes: %d, crumbs: %d\n",
	output->active_length, wholebytes, crumbs);
#endif

    for (i=0; i < wholebytes; i++) {
	for (j=0; j < 8; j++) {
	    tb[j] = (((buf[i] >> (7-j)) & 1));
	    tb[j] = output->output_depth - tb[j];
	}
	(void) fwrite(&tb[0], sizeof(char), 8, write_fd);
    }
    for (j=0; j < crumbs; j++) {
	tb[j] = (((buf[wholebytes] >> (7-j)) & 1));
	tb[j] = output->output_depth - tb[j];
    }
    (void) fwrite(&tb[0], sizeof(char), (size_t) crumbs, write_fd);
}

/*
 * write_colour() - Write out one row of RGB PNM data.
 */

/* FIXME - multiple levels and CMYK/CMYKcm */

void write_colour(output_t *output,		/* I: Data buffers */
		 image_t *image)		/* I: Image data */
{
    int wholebytes = image->image_width / 8;
    int crumbs = image->image_width - (wholebytes * 8);

    int i, j, jj;
    char tb[8*3];

    char *cyan_buf;
    char *magenta_buf;
    char *yellow_buf;
    char *black_buf;

    cyan_buf = output->cyan_bufs[0];
    magenta_buf = output->magenta_bufs[0];
    yellow_buf = output->yellow_bufs[0];
    if (image->colour_type != PCL_CMY)
	black_buf = output->black_bufs[0];
    else
	black_buf = NULL;

#ifdef DEBUG
    fprintf(stderr, "Data Length: %d, wholebytes: %d, crumbs: %d, planes: %d\n",
	output->active_length, wholebytes, crumbs, image->colour_type);

    fprintf(stderr, "Cyan: ");
    for (i=0; i < output->active_length; i++) {
	fprintf(stderr, "%02x ", (unsigned char) cyan_buf[i]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "Magenta: ");
    for (i=0; i < output->active_length; i++) {
	fprintf(stderr, "%02x ", (unsigned char) magenta_buf[i]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "Yellow: ");
    for (i=0; i < output->active_length; i++) {
	fprintf(stderr, "%02x ", (unsigned char) yellow_buf[i]);
    }
    fprintf(stderr, "\n");
    if (image->colour_type == PCL_CMYK) {
	fprintf(stderr, "Black: ");
	for (i=0; i < output->active_length; i++) {
	    fprintf(stderr, "%02x ", (unsigned char) black_buf[i]);
	}
	fprintf(stderr, "\n");
    }
#endif

    if (image->colour_type == PCL_CMY) {
	for (i=0; i < wholebytes; i++) {
	    for (j=0,jj=0; j < 8; j++,jj+=3) {
		tb[jj] = (((cyan_buf[i] >> (7-j)) & 1));
		tb[jj] = output->output_depth - tb[jj];
		tb[jj+1] = (((magenta_buf[i] >> (7-j)) & 1));
		tb[jj+1] = output->output_depth - tb[jj+1];
		tb[jj+2] = (((yellow_buf[i] >> (7-j)) & 1));
		tb[jj+2] = output->output_depth - tb[jj+2];
	    }
	    (void) fwrite(&tb[0], sizeof(char), (size_t) (8*3), write_fd);
	}
	for (j=0,jj=0; j < crumbs; j++,jj+=3) {
	    tb[jj] = (((cyan_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj] = output->output_depth - tb[jj];
	    tb[jj+1] = (((magenta_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj+1] = output->output_depth - tb[jj+1];
	    tb[jj+2] = (((yellow_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj+2] = output->output_depth - tb[jj+2];
	}
	(void) fwrite(&tb[0], sizeof(char), (size_t) crumbs*3, write_fd);
    }
    else {
	for (i=0; i < wholebytes; i++) {
	    for (j=0,jj=0; j < 8; j++,jj+=3) {
#if !defined OUTPUT_CMYK_ONLY_K && !defined OUTPUT_CMYK_ONLY_CMY
		tb[jj] = ((((cyan_buf[i]|black_buf[i]) >> (7-j)) & 1));
		tb[jj+1] = ((((magenta_buf[i]|black_buf[i]) >> (7-j)) & 1));
		tb[jj+2] = ((((yellow_buf[i]|black_buf[i]) >> (7-j)) & 1));
#endif
#ifdef OUTPUT_CMYK_ONLY_K
		tb[jj] = (((black_buf[i] >> (7-j)) & 1));
		tb[jj+1] = (((black_buf[i] >> (7-j)) & 1));
		tb[jj+2] = (((black_buf[i] >> (7-j)) & 1));
#endif
#ifdef OUTPUT_CMYK_ONLY_CMY
		tb[jj] = (((cyan_buf[i] >> (7-j)) & 1));
		tb[jj+1] = (((magenta_buf[i] >> (7-j)) & 1));
		tb[jj+2] = (((yellow_buf[i] >> (7-j)) & 1));
#endif
		tb[jj] = output->output_depth - tb[jj];
		tb[jj+1] = output->output_depth - tb[jj+1];
		tb[jj+2] = output->output_depth - tb[jj+2];
	    }
	    (void) fwrite(&tb[0], sizeof(char), (size_t) (8*3), write_fd);
	}
	for (j=0,jj=0; j < crumbs; j++,jj+=3) {
#if !defined OUTPUT_CMYK_ONLY_K && !defined OUTPUT_CMYK_ONLY_CMY
	    tb[jj] = ((((cyan_buf[wholebytes]|black_buf[wholebytes]) >> (7-j)) & 1));
	    tb[jj+1] = ((((magenta_buf[wholebytes]|black_buf[wholebytes]) >> (7-j)) & 1));
	    tb[jj+2] = ((((yellow_buf[wholebytes]|black_buf[wholebytes]) >> (7-j)) & 1));
#endif
#ifdef OUTPUT_CMYK_ONLY_K
	    tb[jj] = (((black_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj+1] = (((black_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj+2] = (((black_buf[wholebytes] >> (7-j)) & 1));
#endif
#ifdef OUTPUT_CMYK_ONLY_CMY
	    tb[jj] = (((cyan_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj+1] = (((magenta_buf[wholebytes] >> (7-j)) & 1));
	    tb[jj+2] = (((yellow_buf[wholebytes] >> (7-j)) & 1));
#endif
	    tb[jj] = output->output_depth - tb[jj];
	    tb[jj+1] = output->output_depth - tb[jj+1];
	    tb[jj+2] = output->output_depth - tb[jj+2];
	}
	(void) fwrite(&tb[0], sizeof(char), (size_t) crumbs*3, write_fd);
    }
}

/*
 * decode_tiff() - Uncompress a TIFF encoded buffer
 */

int decode_tiff(char *in_buffer,		/* I: Data buffer */
		int data_length,		/* I: Length of data */
		char *decode_buf,		/* O: decoded data */
		int maxlen)			/* I: Max length of decode_buf */
{
/* The TIFF coding consists of either:-
 *
 * (0 <= count <= 127) (count+1 bytes of data) for non repeating data
 * or
 * (-127 <= count <= -1) (data) for 1-count bytes of repeating data
 */

    int count;
    int pos = 0;
    int dpos = 0;
#ifdef DEBUG
    int i;
#endif

    while(pos < data_length ) {

	count = in_buffer[pos];

	if ((count >= 0) && (count <= 127)) {
#ifdef DEBUG
	    fprintf(stderr, "%d bytes of nonrepeated data\n", count+1);
	    fprintf(stderr, "DATA: ");
	    for (i=0; i< (count+1); i++) {
		fprintf(stderr, "%02x ", (unsigned char) in_buffer[pos + 1 + i]);
	    }
	    fprintf(stderr, "\n");
#endif
	    if ((dpos + count + 1) > maxlen) {
		fprintf(stderr, "ERROR: Too much expanded data (%d)!\n", dpos + count + 1);
		exit(EXIT_FAILURE);
	    }
	    memcpy(&decode_buf[dpos], &in_buffer[pos+1], (size_t) (count + 1));
	    dpos += count + 1;
	    pos += count + 2;
	}
	else if ((count >= -127) && (count < 0)) {
#ifdef DEBUG
	    fprintf(stderr, "%02x repeated %d times\n", (unsigned char) in_buffer[pos + 1], 1 - count);
#endif
	    if ((dpos + 1 - count) > maxlen) {
		fprintf(stderr, "ERROR: Too much expanded data (%d)!\n", dpos + 1 - count);
		exit(EXIT_FAILURE);
	    }
	    memset(&decode_buf[dpos], in_buffer[pos + 1], (size_t) (1 - count));
	    dpos += 1 - count;
	    pos += 2;
	}
	else {
	    fprintf(stderr, "ERROR: Illegal TIFF count: %d, skipped\n", count);
	    pos += 2;
	}
    }

#ifdef DEBUG
    fprintf(stderr, "TIFFOUT: ");
    for (i=0; i< dpos; i++) {
	fprintf(stderr, "%02x ", (unsigned char) decode_buf[i]);
    }
    fprintf(stderr, "\n");
#endif
    return(dpos);
}

/*
 * pcl_reset() - Rest image parameters to default
 */

void pcl_reset(image_t *i)
{
    i->colour_type = PCL_MONO;
    i->black_depth = 2;		/* Assume mono */
    i->cyan_depth = 0;
    i->magenta_depth = 0;
    i->yellow_depth = 0;
    i->lcyan_depth = 0;
    i->lmagenta_depth = 0;
    i->image_width = -1;
    i->image_height = -1;
    i->compression_type = 0;	/* should this be NONE? */
}

/*
 * depth_to_rows() - convert the depth of the colour into the number
 * of data rows needed to represent it. Assumes that depth is a power
 * of 2, FIXME if not!
 */

int depth_to_rows(int depth)
{
    int rows;

    if (depth == 0)
	return(0);

    for (rows = 1; rows < 8; rows++) {
    if ((depth >> rows) == 1)
	return(rows);
    }
    fprintf(stderr, "ERROR: depth %d too big to handle in depth_to_rows()!\n",
	depth);
    return(0);	/* ?? */
}

/*
 * Main
 */

int main(int argc, char *argv[])
{

    int command_index;
    command_t command;
    int i, j;				/* Loop/general variables */
    int image_row_counter = -1;		/* Count of current row */
    int current_data_row = -1;		/* Count of data rows received for this output row */
    int expected_data_rows_per_row = -1;
					/* Expected no of data rows per output row */
    image_t image_data;			/* Data concerning image */
    long filepos = -1;

/*
 * Holders for the decoded lines
 */

    output_t output_data;

/*
 * The above pointers (when allocated) are then copied into this
 * variable in the correct order so that the received data can
 * be stored.
 */

    char **received_rows;

    output_data.black_bufs = NULL;		/* Storage for black rows */
    output_data.black_data_rows_per_row = 0;	/* Number of black rows */
    output_data.cyan_bufs = NULL;
    output_data.cyan_data_rows_per_row = 0;
    output_data.magenta_bufs = NULL;
    output_data.magenta_data_rows_per_row = 0;
    output_data.yellow_bufs = NULL;
    output_data.yellow_data_rows_per_row = 0;
    output_data.lcyan_bufs = NULL;
    output_data.lcyan_data_rows_per_row = 0;
    output_data.lmagenta_bufs = NULL;
    output_data.lmagenta_data_rows_per_row = 0;
    output_data.buffer_length = 0;
    output_data.active_length = 0;
    output_data.output_depth = 0;

    id = id;				/* Remove compiler warning */
    received_rows = NULL;

    if(argc == 1){
	read_fd = stdin;
	write_fd = stdout;
    }
    else if(argc == 2){
	read_fd = fopen(argv[1],"r");
	write_fd = stdout;
    }
    else {
	if(*argv[1] == '-'){
	    read_fd = stdin;
	    write_fd = fopen(argv[2],"w");
	}
	else {
	    read_fd = fopen(argv[1],"r");
	    write_fd = fopen(argv[2],"w");
	}
    }

    if (read_fd == (FILE *)NULL) {
	fprintf(stderr, "ERROR: Error Opening input file.\n");
	exit (EXIT_FAILURE);
    }

    if (write_fd == (FILE *)NULL) {
	fprintf(stderr, "ERROR: Error Opening output file.\n");
	exit (EXIT_FAILURE);
    }

    read_pointer=-1;
    eof=0;
    initial_command_index=0;
    initial_command[initial_command_index] = '\0';
    numeric_arg=0;
    final_command = '\0';


    pcl_reset(&image_data);

    while (1) {
	pcl_read_command();
	if (eof == 1) {
/* #ifdef DEBUG */
	    fprintf(stderr, "EOF while reading command.\n");
/* #endif */
	    (void) fclose(read_fd);
	    (void) fclose(write_fd);
	    exit(EXIT_SUCCESS);
	}

#ifdef DEBUG
	fprintf(stderr, "initial_command: %s, numeric_arg: %d, final_command: %c\n",
	    initial_command, numeric_arg, final_command);
#endif

	command_index = pcl_find_command();
	if (command_index == -1) {
	    fprintf(stderr, "ERROR: Unknown (and unhandled) command: %s%d%c\n", initial_command,
		numeric_arg, final_command);
/* We may have to skip some data here */
	}
	else {
	    command = pcl_commands[command_index].command;
	    if (pcl_commands[command_index].has_data == 1) {

/* Read the data into data_buffer */

#ifdef DEBUG
		fprintf(stderr, "Data: ");
#endif

		if (numeric_arg > MAX_DATA) {
		    fprintf(stderr, "ERROR: Too much data (%d), increase MAX_DATA!\n", numeric_arg);
		    exit(EXIT_FAILURE);
		}

		for (i=0; i < numeric_arg; i++) {
		    fill_buffer();
		    if (eof == 1) {
			fprintf(stderr, "ERROR: Unexpected EOF whilst reading data\n");
			exit(EXIT_FAILURE);
		    }
		    data_buffer[i] = read_buffer[read_pointer++];

#ifdef DEBUG
		    fprintf(stderr, "%02x ", (unsigned char) data_buffer[i]);
#endif

		}

#ifdef DEBUG
		fprintf(stderr, "\n");
#endif

	    }
	    switch(command) {
	    case PCL_RESET :
		fprintf(stderr, "%s\n", pcl_commands[command_index].description);
		pcl_reset(&image_data);
		break;

	    case PCL_RESET_MARGINS :
		fprintf(stderr, "%s\n", pcl_commands[command_index].description);
		break;

	    case PCL_START_RASTER :
		fprintf(stderr, "%s\n", pcl_commands[command_index].description);

/* Make sure we have all the stuff needed to work out what we are going
   to write out. */

		i = 0;		/* use as error indicator */

		if (image_data.image_width == -1) {
		    fprintf(stderr, "ERROR: Image width not set!\n");
		    i++;
		}
		if (image_data.image_height == -1) {
		    fprintf(stderr, "WARNING: Image height not set!\n");
		}

		if ((image_data.black_depth != 0) &&
		    (image_data.black_depth != 2)) {
		    fprintf(stderr, "WARNING: Only 2 level black dithers handled.\n");
		}
		if ((image_data.cyan_depth != 0) &&
		    (image_data.cyan_depth != 2)) {
		    fprintf(stderr, "WARNING: Only 2 level cyan dithers handled.\n");
		}
		if ((image_data.magenta_depth != 0) &&
		    (image_data.magenta_depth != 2)) {
		    fprintf(stderr, "WARNING: Only 2 level magenta dithers handled.\n");
		}
		if ((image_data.yellow_depth != 0) &&
		    (image_data.yellow_depth != 2)) {
		    fprintf(stderr, "WARNING: only 2 level yellow dithers handled.\n");
		}
		if (image_data.lcyan_depth != 0) {
		    fprintf(stderr, "WARNING: Light cyan dithers not yet handled.\n");
		}
		if (image_data.lmagenta_depth != 0) {
		    fprintf(stderr, "WARNING: Light magenta dithers not yet handled.\n");
		}

		if ((image_data.compression_type != PCL_COMPRESSION_NONE) &&
			(image_data.compression_type != PCL_COMPRESSION_TIFF)) {
		    fprintf(stderr,
			"Sorry, only 'no compression' or 'tiff compression' handled.\n");
		    i++;
		}

		if (i != 0) {
		    fprintf(stderr, "PNM output suppressed, will continue diagnostic output.\n");
		    skip_output = 1;
		}

		if (skip_output == 0) {
		    if (image_data.colour_type == PCL_MONO)
			(void) fputs("P5\n", write_fd);	/* Raw, Grey */
		    else
			(void) fputs("P6\n", write_fd);	/* Raw, RGB */

		    (void) fputs("# Written by pclunprint.\n", write_fd);

/*
 * Remember the file position where we wrote the image width and height
 * (you don't want to know why!)
 */

		    filepos = ftell(write_fd);

		    fprintf(write_fd, "%10d %10d\n", image_data.image_width,
			image_data.image_height);

/*
 * Write the depth of the image
 */

		    if (image_data.black_depth != 0)
			output_data.output_depth = image_data.black_depth - 1;
		    else
			output_data.output_depth = image_data.cyan_depth - 1;
		    fprintf(write_fd, "%d\n", output_data.output_depth);

		    image_row_counter = 0;
		    current_data_row = 0;

		    output_data.black_data_rows_per_row = depth_to_rows(image_data.black_depth);
		    output_data.cyan_data_rows_per_row = depth_to_rows(image_data.cyan_depth);
		    output_data.magenta_data_rows_per_row = depth_to_rows(image_data.magenta_depth);
		    output_data.yellow_data_rows_per_row = depth_to_rows(image_data.yellow_depth);
		    output_data.lcyan_data_rows_per_row = depth_to_rows(image_data.lcyan_depth);
		    output_data.lmagenta_data_rows_per_row = depth_to_rows(image_data.lmagenta_depth);

/*
 * Allocate some storage for the expected planes
 */

		    output_data.buffer_length = (image_data.image_width + 7) / 8;

		    if (output_data.black_data_rows_per_row != 0) {
			output_data.black_bufs = stp_malloc(output_data.black_data_rows_per_row * sizeof (char *));
			for (i=0; i < output_data.black_data_rows_per_row; i++) {
			    output_data.black_bufs[i] = stp_malloc(output_data.buffer_length * sizeof (char));
			}
		    }
		    if (output_data.cyan_data_rows_per_row != 0) {
			output_data.cyan_bufs = stp_malloc(output_data.cyan_data_rows_per_row * sizeof (char *));
			for (i=0; i < output_data.cyan_data_rows_per_row; i++) {
			    output_data.cyan_bufs[i] = stp_malloc(output_data.buffer_length * sizeof (char));
			}
		    }
		    if (output_data.magenta_data_rows_per_row != 0) {
			output_data.magenta_bufs = stp_malloc(output_data.magenta_data_rows_per_row * sizeof (char *));
			for (i=0; i < output_data.magenta_data_rows_per_row; i++) {
			    output_data.magenta_bufs[i] = stp_malloc(output_data.buffer_length * sizeof (char));
			}
		    }
		    if (output_data.yellow_data_rows_per_row != 0) {
			output_data.yellow_bufs = stp_malloc(output_data.yellow_data_rows_per_row * sizeof (char *));
			for (i=0; i < output_data.yellow_data_rows_per_row; i++) {
			    output_data.yellow_bufs[i] = stp_malloc(output_data.buffer_length * sizeof (char));
			}
		    }
		    if (output_data.lcyan_data_rows_per_row != 0) {
			output_data.lcyan_bufs = stp_malloc(output_data.lcyan_data_rows_per_row * sizeof (char *));
			for (i=0; i < output_data.lcyan_data_rows_per_row; i++) {
			     output_data.lcyan_bufs[i] = stp_malloc(output_data.buffer_length * sizeof (char));
			}
		    }
		    if (output_data.lmagenta_data_rows_per_row != 0) {
			output_data.lmagenta_bufs = stp_malloc(output_data.lmagenta_data_rows_per_row * sizeof (char *));
			for (i=0; i < output_data.lmagenta_data_rows_per_row; i++) {
			    output_data.lmagenta_bufs[i] = stp_malloc(output_data.buffer_length * sizeof (char));
			}
		    }

/*
 * Now store the pointers in the right order to make life easier in the
 * decoding phase
 */

		    expected_data_rows_per_row = output_data.black_data_rows_per_row +
			output_data.cyan_data_rows_per_row + output_data.magenta_data_rows_per_row +
			output_data.yellow_data_rows_per_row + output_data.lcyan_data_rows_per_row +
			output_data.lmagenta_data_rows_per_row;

		    received_rows = stp_malloc(expected_data_rows_per_row * sizeof(char *));
		    j = 0;
		    for (i = 0; i < output_data.black_data_rows_per_row; i++)
			received_rows[j++] = output_data.black_bufs[i];
		    for (i = 0; i < output_data.cyan_data_rows_per_row; i++)
			received_rows[j++] = output_data.cyan_bufs[i];
		    for (i = 0; i < output_data.magenta_data_rows_per_row; i++)
			received_rows[j++] = output_data.magenta_bufs[i];
		    for (i = 0; i < output_data.yellow_data_rows_per_row; i++)
			received_rows[j++] = output_data.yellow_bufs[i];
		    for (i = 0; i < output_data.lcyan_data_rows_per_row; i++)
			received_rows[j++] = output_data.lcyan_bufs[i];
		    for (i = 0; i < output_data.lmagenta_data_rows_per_row; i++)
			received_rows[j++] = output_data.lmagenta_bufs[i];
		}
		break;

	    case PCL_END_RASTER :
	    case PCL_END_COLOUR_RASTER :
		fprintf(stderr, "%s\n", pcl_commands[command_index].description);

		if (skip_output == 0) {

/*
 * Check that we got the correct number of rows of data. If the expected number is
 * -1, we have to go back and fill in the PNM parameters (which is why we remembered
 * where they were in the file!)
 */

		    if (image_data.image_height == -1) {
			image_data.image_height = image_row_counter;
			if (fseek(write_fd, filepos, SEEK_SET) != -1) {
			    fprintf(write_fd, "%10d %10d\n", image_data.image_width,
				image_data.image_height);
			    fseek(write_fd, 0L, SEEK_END);
			}
		    }

		    if (image_row_counter != image_data.image_height)
			fprintf(stderr, "ERROR: Row count mismatch. Expected %d rows, got %d rows.\n",
			    image_data.image_height, image_row_counter);
		    else
			fprintf(stderr, "\t%d rows processed.\n", image_row_counter);

		    image_data.image_height = -1;
		    image_row_counter = -1;

		    if (output_data.black_data_rows_per_row != 0) {
			for (i=0; i < output_data.black_data_rows_per_row; i++) {
			    stp_free(output_data.black_bufs[i]);
			}
			stp_free(output_data.black_bufs);
			output_data.black_bufs = NULL;
		    }
		    output_data.black_data_rows_per_row = 0;
		    if (output_data.cyan_data_rows_per_row != 0) {
			for (i=0; i < output_data.cyan_data_rows_per_row; i++) {
			    stp_free(output_data.cyan_bufs[i]);
			}
			stp_free(output_data.cyan_bufs);
			output_data.cyan_bufs = NULL;
		    }
		    output_data.cyan_data_rows_per_row = 0;
		    if (output_data.magenta_data_rows_per_row != 0) {
			for (i=0; i < output_data.magenta_data_rows_per_row; i++) {
			    stp_free(output_data.magenta_bufs[i]);
			}
			stp_free(output_data.magenta_bufs);
			output_data.magenta_bufs = NULL;
		    }
		    output_data.magenta_data_rows_per_row = 0;
		    if (output_data.yellow_data_rows_per_row != 0) {
			for (i=0; i < output_data.yellow_data_rows_per_row; i++) {
			    stp_free(output_data.yellow_bufs[i]);
			}
			stp_free(output_data.yellow_bufs);
			output_data.yellow_bufs = NULL;
		    }
		    output_data.yellow_data_rows_per_row = 0;
		    if (output_data.lcyan_data_rows_per_row != 0) {
			for (i=0; i < output_data.lcyan_data_rows_per_row; i++) {
			    stp_free(output_data.lcyan_bufs[i]);
			}
			stp_free(output_data.lcyan_bufs);
			output_data.lcyan_bufs = NULL;
		    }
		    output_data.lcyan_data_rows_per_row = 0;
		    if (output_data.lmagenta_data_rows_per_row != 0) {
			for (i=0; i < output_data.lmagenta_data_rows_per_row; i++) {
			    stp_free(output_data.lmagenta_bufs[i]);
			}
			stp_free(output_data.lmagenta_bufs);
			output_data.lmagenta_bufs = NULL;
		    }
		    output_data.lmagenta_data_rows_per_row = 0;
		    stp_free(received_rows);
		    received_rows = NULL;
		}
		break;

	    case PCL_MEDIA_SIZE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 2 :
		    fprintf(stderr, "Letter\n");
		    break;
		case 3 :
		    fprintf(stderr, "Legal\n");
		    break;
		case 6 :
		    fprintf(stderr, "Tabloid\n");
		    break;
		case 26 :
		    fprintf(stderr, "A4\n");
		    break;
		case 27 :
		    fprintf(stderr, "A3\n");
		    break;
		case 101 :
		    fprintf(stderr, "Custom\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_MEDIA_TYPE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "Plain\n");
		    break;
		case 1 :
		    fprintf(stderr, "Bond\n");
		    break;
		case 2 :
		    fprintf(stderr, "Premium\n");
		    break;
		case 3 :
		    fprintf(stderr, "Glossy/Photo\n");
		    break;
		case 4 :
		    fprintf(stderr, "Transparency\n");
		    break;
		case 5 :
		    fprintf(stderr, "Quick-dry Photo\n");
		    break;
		case 6 :
		    fprintf(stderr, "Quick-dry Transparency\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_MEDIA_SOURCE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case -2 :
		    fprintf(stderr, "FEED CURRENT\n");
		    break;
		case 0 :
		    fprintf(stderr, "EJECT\n");
		    break;
		case 1 :
		    fprintf(stderr, "LJ Tray 2 or Portable CSF or DJ Tray\n");
		    break;
		case 2 :
		    fprintf(stderr, "Manual\n");
		    break;
		case 3 :
		    fprintf(stderr, "Envelope\n");
		    break;
		case 4 :
		    fprintf(stderr, "LJ Tray 3 or Desktop CSF or DJ Tray 2\n");
		    break;
		case 5 :
		    fprintf(stderr, "LJ Tray 4 or DJ optional\n");
		    break;
		case 7 :
		    fprintf(stderr, "DJ Autosource\n");
		    break;
		case 8 :
		    fprintf(stderr, "LJ Tray 1\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_SHINGLING :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "None\n");
		    break;
		case 1 :
		    fprintf(stderr, "2 passes\n");
		    break;
		case 2 :
		    fprintf(stderr, "4 passes\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_RASTERGRAPHICS_QUALITY :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "(set by printer controls)\n");
		case 1 :
		    fprintf(stderr, "Draft\n");
		    break;
		case 2 :
		    fprintf(stderr, "High\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_DEPLETION :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 1 :
		    fprintf(stderr, "None\n");
		    break;
		case 2 :
		    fprintf(stderr, "25%%\n");
		    break;
		case 3 :
		    fprintf(stderr, "50%%\n");
		    break;
		case 5 :
		    fprintf(stderr, "50%% with gamma correction\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_PRINT_QUALITY :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case -1 :
		    fprintf(stderr, "Draft\n");
		    break;
		case 0 :
		    fprintf(stderr, "Normal\n");
		    break;
		case 1 :
		    fprintf(stderr, "Presentation\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_RELATIVE_VERTICAL_PIXEL_MOVEMENT :
		fprintf(stderr, "%s: %d\n", pcl_commands[command_index].description, numeric_arg);

		if (skip_output == 0) {

/* Check that we are in raster mode */

		    if (expected_data_rows_per_row == -1)
			fprintf(stderr, "ERROR: raster data without start raster!\n");

/*
  What we need to do now is to write out "N" rows of all-white data to
  simulate the vertical slew
*/

		    for (i=0; i<expected_data_rows_per_row; i++)
		    {
			memset(received_rows[i], 0, (size_t) output_data.buffer_length * sizeof(char));
		    }
		    for (i=0; i<numeric_arg; i++)
		    {
			if (image_data.colour_type == PCL_MONO)
			    write_grey(&output_data, &image_data);
			else
			    write_colour(&output_data, &image_data);
			image_row_counter++;
		    }
		}
		break;

	    case PCL_DUPLEX :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "None\n");
		    break;
		case 1 :
		    fprintf(stderr, "Duplex No Tumble (Long Edge)\n");
		    break;
		case 2 :
		    fprintf(stderr, "Duplex Tumble (Short Edge)\n");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)\n", numeric_arg);
		    break;
		}
		break;

	    case PCL_PERF_SKIP :
	    case PCL_TOP_MARGIN :
	    case PCL_RESOLUTION :
	    case PCL_LEFTRASTER_POS :
	    case PCL_TOPRASTER_POS :
	    case PCL_VERTICAL_CURSOR_POSITIONING_BY_DOTS :
	    case PCL_HORIZONTAL_CURSOR_POSITIONING_BY_DOTS :
	    case PCL_PALETTE_CONFIGURATION :
	    case PCL_UNIT_OF_MEASURE :
	    case PCL_GRAY_BALANCE :
	    case PCL_DRIVER_CONFIG :
	    case PCL_LPI :
	    case PCL_CPI :
	    case PCL_PAGE_LENGTH :
	    case PCL_NUM_COPIES :
	    case RTL_CONFIGURE :
	    case PCL_ENTER_PCL :
	    case PCL_NEGATIVE_MOTION :
	    case PCL_JOB_SEPARATION :
	    case PCL_LEFT_OFFSET_REGISTRATION :
	    case PCL_TOP_OFFSET_REGISTRATION :
	    case PCL_PRINT_DIRECTION :
	    case PCL_LEFT_MARGIN :
	    case PCL_RIGHT_MARGIN :
	    case PCL_TEXT_LENGTH :
		fprintf(stderr, "%s: %d (ignored)", pcl_commands[command_index].description, numeric_arg);
		if (pcl_commands[command_index].has_data == 1) {
		    fprintf(stderr, " Data: ");
		    for (i=0; i < numeric_arg; i++) {
			fprintf(stderr, "%02x ", (unsigned char) data_buffer[i]);
		    }
		}
		fprintf(stderr, "\n");
		break;

	    case PCL_COLOURTYPE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		image_data.colour_type = -numeric_arg;
		switch (image_data.colour_type) {
		    case PCL_MONO :
			fprintf(stderr, "MONO\n");
			break;
		    case PCL_CMY :
			fprintf(stderr, "CMY (one cart)\n");
			image_data.black_depth = 0;	/* Black levels */
			image_data.cyan_depth = 2;	/* Cyan levels */
			image_data.magenta_depth = 2;	/* Magenta levels */
			image_data.yellow_depth = 2;	/* Yellow levels */
			break;
		    case PCL_CMYK :
			fprintf(stderr, "CMYK (two cart)\n");
			image_data.black_depth = 2;	/* Black levels */
			image_data.cyan_depth = 2;	/* Cyan levels */
			image_data.magenta_depth = 2;	/* Magenta levels */
			image_data.yellow_depth = 2;	/* Yellow levels */
			break;
		    default :
			fprintf(stderr, "Unknown (%d)\n", -numeric_arg);
			break;
		}
		break;

	    case PCL_COMPRESSIONTYPE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		image_data.compression_type = numeric_arg;
		switch (image_data.compression_type) {
		    case PCL_COMPRESSION_NONE :
			fprintf(stderr, "NONE\n");
			break;
		    case PCL_COMPRESSION_RUNLENGTH :
			fprintf(stderr, "Runlength\n");
			break;
		    case PCL_COMPRESSION_TIFF :
			fprintf(stderr, "TIFF\n");
			break;
		    case PCL_COMPRESSION_CRDR :
			fprintf(stderr, "Compressed Row Delta Replacement\n");
			break;
		    default :
			fprintf(stderr, "Unknown (%d)\n", image_data.compression_type);
			break;
		}
		break;

	    case PCL_RASTER_WIDTH :
		fprintf(stderr, "%s: %d\n", pcl_commands[command_index].description, numeric_arg);
		image_data.image_width = numeric_arg;
		break;

	    case PCL_RASTER_HEIGHT :
		fprintf(stderr, "%s: %d\n", pcl_commands[command_index].description, numeric_arg);
		image_data.image_height = numeric_arg;
		break;

	    case PCL_CONFIGURE :
		fprintf(stderr, "%s (size=%d)\n", pcl_commands[command_index].description,
		    numeric_arg);
		fprintf(stderr, "\tFormat: %d\n", data_buffer[0]);
		if (data_buffer[0] == 2) {

/*
 * the data that follows depends on the colour type (buffer[1]). The size
 * of the data should be 2 + (6 * number of planes).
 */

		    fprintf(stderr, "\tOutput Planes: ");
		    image_data.colour_type = data_buffer[1]; 	/* # output planes */
		    switch (image_data.colour_type) {
			case PCL_MONO :
			    fprintf(stderr, "MONO\n");

/* Size should be 8 */

			    if (numeric_arg != 8)
				fprintf(stderr, "ERROR: Expected 8 bytes of data, got %d\n", numeric_arg);

			    fprintf(stderr, "\tBlack: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[2]<<8)+(unsigned char)data_buffer[3],
				((unsigned char) data_buffer[4]<<8)+(unsigned char) data_buffer[5], data_buffer[7]);
			    image_data.black_depth = data_buffer[7];	/* Black levels */
			    break;
			case PCL_CMY :
			    fprintf(stderr, "CMY (one cart)\n");

/* Size should be 20 */

			    if (numeric_arg != 20)
				fprintf(stderr, "ERROR: Expected 20 bytes of data, got %d\n", numeric_arg);

			    fprintf(stderr, "\tCyan: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[2]<<8)+(unsigned char) data_buffer[3],
				((unsigned char) data_buffer[4]<<8)+(unsigned char) data_buffer[5], data_buffer[7]);
			    fprintf(stderr, "\tMagenta: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[8]<<8)+(unsigned char) data_buffer[9],
			    ((unsigned char) data_buffer[10]<<8)+(unsigned char) data_buffer[11], data_buffer[13]);
			    fprintf(stderr, "\tYellow: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[14]<<8)+(unsigned char) data_buffer[15],
				((unsigned char) data_buffer[16]<<8)+(unsigned char) data_buffer[17], data_buffer[19]);
			    image_data.black_depth = 0;
			    image_data.cyan_depth = data_buffer[7];		/* Cyan levels */
			    image_data.magenta_depth = data_buffer[13];	/* Magenta levels */
			    image_data.yellow_depth = data_buffer[19];	/* Yellow levels */
			    break;
			case PCL_CMYK :
			    fprintf(stderr, "CMYK (two cart)\n");

/* Size should be 26 */

			    if (numeric_arg != 26)
				fprintf(stderr, "ERROR: Expected 26 bytes of data, got %d\n", numeric_arg);

			    fprintf(stderr, "\tBlack: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[2]<<8)+(unsigned char) data_buffer[3],
				((unsigned char) data_buffer[4]<<8)+(unsigned char) data_buffer[5], data_buffer[7]);
			    fprintf(stderr, "\tCyan: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[8]<<8)+(unsigned char) data_buffer[9],
				((unsigned char) data_buffer[10]<<8)+(unsigned char) data_buffer[11], data_buffer[13]);
			    fprintf(stderr, "\tMagenta: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[14]<<8)+(unsigned char) data_buffer[15],
				((unsigned char) data_buffer[16]<<8)+(unsigned char) data_buffer[17], data_buffer[19]);
			    fprintf(stderr, "\tYellow: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[20]<<8)+(unsigned char) data_buffer[21],
				((unsigned char) data_buffer[22]<<8)+(unsigned char) data_buffer[23], data_buffer[25]);
			    image_data.black_depth = data_buffer[7];	/* Black levels */
			    image_data.cyan_depth = data_buffer[13];	/* Cyan levels */
			    image_data.magenta_depth = data_buffer[19];	/* Magenta levels */
			    image_data.yellow_depth = data_buffer[25];	/* Yellow levels */
			    break;
			case PCL_CMYKcm :
			    fprintf(stderr, "CMYKcm (two cart photo)\n");

/* Size should be 38 */

			    if (numeric_arg != 38)
				fprintf(stderr, "ERROR: Expected 38 bytes of data, got %d\n", numeric_arg);

			    fprintf(stderr, "\tBlack: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[2]<<8)+(unsigned char) data_buffer[3],
				((unsigned char) data_buffer[4]<<8)+(unsigned char) data_buffer[5], data_buffer[7]);
			    fprintf(stderr, "\tCyan: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[8]<<8)+(unsigned char) data_buffer[9],
				((unsigned char) data_buffer[10]<<8)+(unsigned char) data_buffer[11], data_buffer[13]);
			    fprintf(stderr, "\tMagenta: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[14]<<8)+(unsigned char) data_buffer[15],
				((unsigned char) data_buffer[16]<<8)+(unsigned char) data_buffer[17], data_buffer[19]);
			    fprintf(stderr, "\tYellow: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[20]<<8)+(unsigned char) data_buffer[21],
				((unsigned char) data_buffer[22]<<8)+(unsigned char) data_buffer[23], data_buffer[25]);
			    fprintf(stderr, "\tLight Cyan: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[26]<<8)+(unsigned char) data_buffer[27],
				((unsigned char) data_buffer[28]<<8)+(unsigned char) data_buffer[29], data_buffer[31]);
			    fprintf(stderr, "\tLight Magenta: X dpi: %d, Y dpi: %d, Levels: %d\n", ((unsigned char) data_buffer[32]<<8)+(unsigned char) data_buffer[33],
				((unsigned char) data_buffer[34]<<8)+(unsigned char) data_buffer[35], data_buffer[37]);
			    image_data.black_depth = data_buffer[7];	/* Black levels */
			    image_data.cyan_depth = data_buffer[13];	/* Cyan levels */
			    image_data.magenta_depth = data_buffer[19];	/* Magenta levels */
			    image_data.yellow_depth = data_buffer[25];	/* Yellow levels */
			    image_data.lcyan_depth = data_buffer[31];	/* Cyan levels */
			    image_data.lmagenta_depth = data_buffer[37];	/* Magenta levels */
			    break;
			default :
			    fprintf(stderr, "Unknown (%d)\n", data_buffer[1]);
			    break;
		    }
		}
		else {
		    fprintf(stderr, "Unknown format %d\n", data_buffer[0]);
		    fprintf(stderr, "Data: ");
		    for (i=0; i < numeric_arg; i++) {
			fprintf(stderr, "%02x ", (unsigned char) data_buffer[i]);
		    }
		    fprintf(stderr, "\n");
		}
		break;

	    case PCL_DATA :
	    case PCL_DATA_LAST :
		if (skip_output == 1) {
		    fprintf(stderr, "%s, length: %d\n", pcl_commands[command_index].description, numeric_arg);
		}
		else {

/*
 * Make sure that we have enough data to process this command!
 */

		    if (expected_data_rows_per_row == -1)
			fprintf(stderr, "ERROR: raster data without start raster!\n");

/*
 * The last flag indicates that this is the end of the planes for a row
 * so we check it against the number of planes we have seen and are
 * expecting.
 */

		    if (command == PCL_DATA_LAST) {
			if (current_data_row != (expected_data_rows_per_row - 1))
			    fprintf(stderr, "ERROR: 'Last Plane' set on plane %d of %d!\n",
				current_data_row, expected_data_rows_per_row);
		    }
		    else {
			if (current_data_row == (expected_data_rows_per_row - 1))
			    fprintf(stderr, "ERROR: Expected 'last plane', but not set!\n");
		    }

/*
 * Accumulate the data rows for each output row,then write the image.
 */

		    if (image_data.compression_type == PCL_COMPRESSION_NONE) {
			memcpy(received_rows[current_data_row], &data_buffer, (size_t) numeric_arg);
			output_data.active_length = numeric_arg;
		    }
		    else
			output_data.active_length = decode_tiff(data_buffer, numeric_arg, received_rows[current_data_row], output_data.buffer_length);

		    if (command == PCL_DATA_LAST) {
			if (image_data.colour_type == PCL_MONO)
			    write_grey(&output_data, &image_data);
			else
			    write_colour(&output_data, &image_data);
			current_data_row = 0;
			image_row_counter++;
		    }
		    else
			current_data_row++;
		}
		break;

	    case PCL_ENTER_HPGL2 :
	    case PCL_ENTER_PJL : {
		    int c;
		    fprintf(stderr, "%s %d\n", pcl_commands[command_index].description, numeric_arg);

/*
 * This is a special command. Read up to the next ESC and output it.
 */

		    c = 0;
		    while (1) {
			fill_buffer();
			if (eof == 1) {
			    fprintf(stderr, "\n");
			    break;
			}
			c = read_buffer[read_pointer++];
			if (c == '\033')
			    break;
			fprintf(stderr, "%c", c);
		    }

/*
 * Now we are sitting at the "ESC" that is the start of the next command.
 */
		    read_pointer--;
		    fprintf(stderr, "\n");
		    fprintf(stderr, "End of %s\n", pcl_commands[command_index].description);
		}
		break;

	    case PCL_PAGE_ORIENTATION :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "Portrait");
		    break;
		case 1 :
		    fprintf(stderr, "Landscape");
		    break;
		case 2 :
		    fprintf(stderr, "Reverse Portrait");
		    break;
		case 3 :
		    fprintf(stderr, "Reverse Landscape");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)", numeric_arg);
		    break;
		}
		fprintf(stderr, " (ignored)\n");
		break;

	    case PCL_MEDIA_SIDE :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 0 :
		    fprintf(stderr, "Next side");
		    break;
		case 1 :
		    fprintf(stderr, "Front side");
		    break;
		case 2 :
		    fprintf(stderr, "Back side");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)", numeric_arg);
		    break;
		}
		fprintf(stderr, " (ignored)\n");
		break;

	    case PCL_MEDIA_DEST :
		fprintf(stderr, "%s: ", pcl_commands[command_index].description);
		switch (numeric_arg) {
		case 1 :
		    fprintf(stderr, "Upper Output bin");
		    break;
		case 2 :
		    fprintf(stderr, "Lower (Rear) Output bin");
		    break;
		default :
		    fprintf(stderr, "Unknown (%d)", numeric_arg);
		    break;
		}
		fprintf(stderr, " (ignored)\n");
		break;

	    default :
		fprintf(stderr, "ERROR: No handler for %s!\n", pcl_commands[command_index].description);
		break;
	    }
	}
    }
}
