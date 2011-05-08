/*
 * "$Id: testpattern.h,v 1.20 2010/09/06 17:08:39 rlk Exp $"
 *
 *   Test pattern generator for Gimp-Print
 *
 *   Copyright 2001 Robert Krawitz <rlk@alum.mit.edu>
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gutenprint/gutenprint.h>
#include <stdio.h>

typedef struct
{
  enum {
    E_PATTERN,
    E_XPATTERN,
    E_IMAGE,
    E_GRID
  } type;
  union {
    struct {
      double mins[32];
      double vals[32];
      double gammas[32];
      double levels[32];
      double lower;
      double upper;
    } pattern;
    struct {
      int ticks;
    } grid;
    struct {
      int x;
      int y;
      int bits;
      const char *data;
    } image;
  } d;
} testpattern_t;

/*
 * At least with flex, this forbids the scanner from reading ahead.
 * This is necessary for parsing images.
 */
#define YY_ALWAYS_INTERACTIVE 1

typedef enum {
  SIZE_RELATIVE,
  SIZE_PT,
  SIZE_IN,
  SIZE_MM
} size_mode_t;

extern stp_vars_t *global_vars;
extern double global_levels[];
extern double global_gammas[];
extern double global_gamma;
extern int global_steps;
extern double global_ink_limit;
extern char *global_printer;
extern double global_density;
extern double global_xtop;
extern double global_xleft;
extern size_mode_t global_size_mode;
extern double global_hsize;
extern double global_vsize;
extern int global_noblackline;
extern const char *global_image_type;
extern int global_color_model;
extern int global_bit_depth;
extern int global_channel_depth;
extern int global_did_something;
extern int global_invert_data;
extern int global_noscale;
extern char *global_output;
extern int global_quiet;
extern FILE *output;
extern int start_job;
extern int end_job;

extern char *c_strdup(const char *s);
extern testpattern_t *get_next_testpattern(void);
extern void close_output(void);

typedef struct yylv {
  int ival;
  double dval;
  char *sval;
} YYSTYPE;

#define YYSTYPE_IS_DECLARED 1

#include "testpatterny.h"


