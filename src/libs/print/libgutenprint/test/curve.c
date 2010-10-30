/*
 * "$Id: curve.c,v 1.23 2008/01/21 23:19:41 rlk Exp $"
 *
 *   Copyright 2002 Robert Krawitz (rlk@alum.mit.edu)
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#ifdef __GNU_LIBRARY__
#include <getopt.h>
#endif

#define DEBUG_SIGNAL
#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#include <gutenprint/gutenprint.h>

int global_test_count = 0;
int global_error_count = 0;
int verbose = 0;
int quiet = 0;

#ifdef __GNU_LIBRARY__

struct option optlist[] =
{
  { "quiet",		0,	NULL,	(int) 'q' },
  { "verbose",		0,	NULL,	(int) 'v' },
  { NULL,		0,	NULL,	0 	  }
};
#endif

struct test_failure
{
  int test_number;
  struct test_failure *next;
};

static struct test_failure *test_failure_head = NULL;
static struct test_failure *test_failure_tail = NULL;

static void
TEST_internal(const char *name, int line)
{
  global_test_count++;
  printf("%d.%d: Checking %s... ", global_test_count, line, name);
  fflush(stdout);
}

#define TEST(name) TEST_internal(name, __LINE__)

static void
TEST_PASS(void)
{
  printf("PASS\n");
  fflush(stdout);
}

static void
TEST_FAIL(void)
{
  struct test_failure *test_failure_tmp = malloc(sizeof(struct test_failure));
  test_failure_tmp->next = NULL;
  test_failure_tmp->test_number = global_test_count;

  if (!test_failure_head)
    {
      test_failure_head = test_failure_tmp;
      test_failure_tail = test_failure_head;
    }
  else
    {
      test_failure_tail->next = test_failure_tmp;
      test_failure_tail = test_failure_tmp;
    }

  global_error_count++;
  printf("FAIL\n");
  fflush(stdout);
}

#define SIMPLE_TEST_CHECK(conditional)		\
do {						\
  if ((conditional))				\
    TEST_PASS();				\
  else						\
    TEST_FAIL();				\
} while (0)

static const double standard_sat_adjustment[] =
{
  0.50, 0.6,  0.7,  0.8,  0.9,  0.86, 0.82, 0.79, /* C */
  0.78, 0.8,  0.83, 0.87, 0.9,  0.95, 1.05, 1.15, /* B */
  1.3,  1.25, 1.2,  1.15, 1.12, 1.09, 1.06, 1.03, /* M */
  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0, /* R */
  1.0,  0.9,  0.8,  0.7,  0.65, 0.6,  0.55, 0.52, /* Y */
  0.48, 0.47, 0.47, 0.49, 0.49, 0.49, 0.52, 0.51, /* G */
};

static const double reverse_sat_adjustment[] =
{
  0.50, 0.51, 0.52, 0.49, 0.49, 0.49, 0.47, 0.47,
  0.48, 0.52, 0.55, 0.6,  0.65, 0.7,  0.8,  0.9,
  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,
  1.0,  1.03, 1.06, 1.09, 1.12, 1.15, 1.2,  1.25,
  1.3,  1.15, 1.05, 0.95, 0.9,  0.87, 0.83, 0.8,
  0.78, 0.79, 0.82, 0.86, 0.9 , 0.8,  0.7,  0.6,
};

static const stp_curve_point_t standard_piecewise_sat_adjustment[] =
{
  { 0.00, 0.50},
  { 0.02, 0.6},
  { 0.04, 0.7},
  { 0.06, 0.8},
  { 0.08, 0.9},
  { 0.10, 0.86},
  { 0.12, 0.82},
  { 0.14, 0.79},
  { 0.16, 0.78},
  { 0.18, 0.8},
  { 0.20, 0.83},
  { 0.22, 0.87},
  { 0.24, 0.9},
  { 0.26, 0.95},
  { 0.28, 1.05},
  { 0.30, 1.15},
  { 0.32, 0.05},
  { 0.34, 3.95},
  { 0.36, 0.05},
  { 0.38, 1.15},
  { 0.40, 1.12},
  { 0.42, 1.09},
  { 0.44, 1.06},
  { 0.46, 1.03},
  { 0.48, 1.0},
  { 0.50, 1.0},
  { 0.52, 1.0},
  { 0.54, 1.0},
  { 0.56, 1.0},
  { 0.58, 1.0},
  { 0.60, 1.0},
  { 0.62, 1.0},
  { 0.64, 1.0},
  { 0.66, 0.9},
  { 0.68, 0.8},
  { 0.70, 0.7},
  { 0.72, 0.65},
  { 0.74, 0.6},
  { 0.76, 0.55},
  { 0.78, 0.52},
  { 0.80, 0.48},
  { 0.82, 0.47},
  { 0.84, 0.47},
  { 0.86, 0.49},
  { 0.88, 0.49},
  { 0.90, 0.49},
  { 0.93, 0.52},
  { 0.96, 0.51},
  { 1.00, 2},
};

static const stp_curve_point_t reverse_piecewise_sat_adjustment[] =
{
  { 0.00, 0.50},
  { 0.02, 0.6},
  { 0.04, 0.7},
  { 0.06, 0.8},
  { 0.08, 0.9},
  { 0.10, 0.86},
  { 0.12, 0.82},
  { 0.14, 0.79},
  { 0.16, 0.78},
  { 0.18, 0.8},
  { 0.20, 0.83},
  { 0.22, 0.87},
  { 0.24, 0.9},
  { 0.26, 0.95},
  { 0.28, 1.05},
  { 0.30, 1.15},
  { 0.32, 0.05},
  { 0.34, 3.95},
  { 0.36, 0.05},
  { 0.38, 1.15},
  { 0.40, 1.12},
  { 0.42, 1.09},
  { 0.44, 1.06},
  { 0.46, 1.03},
  { 0.48, 1.0},
  { 0.50, 1.0},
  { 0.52, 1.0},
  { 0.54, 1.0},
  { 0.56, 1.0},
  { 0.58, 1.0},
  { 0.60, 1.0},
  { 0.62, 1.0},
  { 0.64, 1.0},
  { 0.66, 0.9},
  { 0.68, 0.8},
  { 0.70, 0.7},
  { 0.72, 0.65},
  { 0.74, 0.6},
  { 0.76, 0.55},
  { 0.78, 0.52},
  { 0.80, 0.48},
  { 0.82, 0.47},
  { 0.84, 0.47},
  { 0.86, 0.49},
  { 0.88, 0.49},
  { 0.90, 0.49},
  { 0.93, 0.52},
  { 0.96, 0.51},
  { 1.00, 2},
};

const char *small_piecewise_curve = 
"<?xml version=\"1.0\"?>\n"
"<gutenprint xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\"\n"
"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
"xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd\">\n"
"<curve wrap=\"nowrap\" type=\"linear\" gamma=\"0\" piecewise=\"true\">\n"
"<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"4\">\n"
"0 0.5 0.1 0.6 1.00 0.51\n"
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

const char *good_curves[] =
  {
    /* Space separated, in same layout as output for comparison */
    "<?xml version=\"1.0\"?>\n"
    "<gutenprint xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd\">\n"
    "<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\" piecewise=\"false\">\n"
    "<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.5 0.6 0.7 0.8 0.9 0.86 0.82 0.79 0.78 0.8 0.83 0.87 0.9 0.95 1.05 1.15\n"
    "1.3 1.25 1.2 1.15 1.12 1.09 1.06 1.03 1 1 1 1 1 1 1 1 1 0.9 0.8 0.7 0.65\n"
    "0.6 0.55 0.52 0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51\n"
    "</sequence>\n"
    "</curve>\n"
    "</gutenprint>\n",

    /* Space separated, in same layout as output for comparison */
    "<?xml version=\"1.0\"?>\n"
    "<gutenprint xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd\">\n"
    "<curve wrap=\"nowrap\" type=\"linear\" gamma=\"0\" piecewise=\"false\">\n"
    "<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.5 0.6 0.7 0.8 0.9 0.86 0.82 0.79 0.78 0.8 0.83 0.87 0.9 0.95 1.05 1.15\n"
    "1.3 1.25 1.2 1.15 1.12 1.09 1.06 1.03 1 1 1 1 1 1 1 1 1 0.9 0.8 0.7 0.65\n"
    "0.6 0.55 0.52 0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51\n"
    "</sequence>\n"
    "</curve>\n"
    "</gutenprint>\n",

    /* Space separated, in same layout as output for comparison */
    "<?xml version=\"1.0\"?>\n"
    "<gutenprint xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd\">\n"
    "<curve wrap=\"wrap\" type=\"spline\" gamma=\"0\" piecewise=\"false\">\n"
    "<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.5 0.6 0.7 0.8 0.9 0.86 0.82 0.79 0.78 0.8 0.83 0.87 0.9 0.95 1.05 1.15\n"
    "1.3 1.25 1.2 1.15 1.12 1.09 1.06 1.03 1 1 1 1 1 1 1 1 1 0.9 0.8 0.7 0.65\n"
    "0.6 0.55 0.52 0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51\n"
    "</sequence>\n"
    "</curve>\n"
    "</gutenprint>\n",

    /* Space separated, in same layout as output for comparison */
    "<?xml version=\"1.0\"?>\n"
    "<gutenprint xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd\">\n"
    "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\" piecewise=\"false\">\n"
    "<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.5 0.6 0.7 0.8 0.9 0.86 0.82 0.79 0.78 0.8 0.83 0.87 0.9 0.95 1.05 1.15\n"
    "1.3 1.25 1.2 1.15 1.12 1.09 1.06 1.03 1 1 1 1 1 1 1 1 1 0.9 0.8 0.7 0.65\n"
    "0.6 0.55 0.52 0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51\n"
    "</sequence>\n"
    "</curve>\n"
    "</gutenprint>\n",

    "<?xml version=\"1.0\"?>\n"
    "<gutenprint xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd\">\n"
    "<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\" piecewise=\"true\">\n"
    "<sequence count=\"96\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0 0.5 0.02 0.6 0.04 0.7 0.06 0.8 0.08 0.9 0.1 0.86 0.12 0.82 0.14 0.79 0.16\n"
    "0.78 0.18 0.8 0.2 0.83 0.22 0.87 0.24 0.9 0.26 0.95 0.28 1.05 0.3 1.15 0.32\n"
    "1.3 0.34 1.25 0.36 1.2 0.38 1.15 0.4 1.12 0.42 1.09 0.44 1.06 0.46 1.03\n"
    "0.48 1 0.5 1 0.52 1 0.54 1 0.56 1 0.58 1 0.6 1 0.62 1 0.64 1 0.66 0.9 0.68\n"
    "0.8 0.7 0.7 0.72 0.65 0.74 0.6 0.76 0.55 0.78 0.52 0.8 0.48 0.82 0.47 0.84\n"
    "0.47 0.86 0.49 0.88 0.49 0.9 0.49 0.93 0.52 0.96 0.51\n"
    "</sequence>\n"
    "</curve>\n"
    "</gutenprint>\n",

    "<?xml version=\"1.0\"?>\n"
    "<gutenprint xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd\">\n"
    "<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\" piecewise=\"true\">\n"
    "<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0 0.5 0.02 0.6 0.96 0.51\n"
    "</sequence>\n"
    "</curve>\n"
    "</gutenprint>\n",

    /* Gamma curve 1 */
    "<?xml version=\"1.0\"?>\n"
    "<gutenprint xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd\">\n"
    "<curve wrap=\"nowrap\" type=\"linear\" gamma=\"1\" piecewise=\"false\">\n"
    "<sequence count=\"0\" lower-bound=\"0\" upper-bound=\"4\"/>\n"
    "</curve>\n"
    "</gutenprint>\n",

    /* Gamma curve 2 */
    "<?xml version=\"1.0\"?>\n"
    "<gutenprint xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd\">\n"
    "<curve wrap=\"nowrap\" type=\"linear\" gamma=\"1\" piecewise=\"false\">\n"
    "<sequence count=\"0\" lower-bound=\"0\" upper-bound=\"4\"/>\n"
    "</curve>\n"
    "</gutenprint>\n"
  };

static const int good_curve_count = sizeof(good_curves) / sizeof(const char *);

const char *bad_curves[] =
  {
    /* Bad point count */
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gutenprint><curve wrap=\"wrap\" type=\"linear\" gamma=\"0\" piecewise=\"false\">\n"
    "<sequence count=\"-1\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.5 0.6 0.7 0.8 0.9 0.86 0.82 0.79 0.78 0.8\n"
    "0.83 0.87 0.9 0.95 1.05 1.15 1.3 1.25 1.2 1.15\n"
    "1.12 1.09 1.06 1.03 1 1 1 1 1 1\n"
    "1 1 1 0.9 0.8 0.7 0.65 0.6 0.55 0.52\n"
    "0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51\n"
    "</sequence></curve></gutenprint>\n",

    /* Bad point count */
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gutenprint><curve wrap=\"wrap\" type=\"linear\" gamma=\"0\" piecewise=\"false\">\n"
    "<sequence count=\"200\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.5 0.6 0.7 0.8 0.9 0.86 0.82 0.79 0.78 0.8\n"
    "0.83 0.87 0.9 0.95 1.05 1.15 1.3 1.25 1.2 1.15\n"
    "1.12 1.09 1.06 1.03 1 1 1 1 1 1\n"
    "1 1 1 0.9 0.8 0.7 0.65 0.6 0.55 0.52\n"
    "0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51\n"
    "</sequence></curve></gutenprint>\n",

    "<?xml version=\"1.0\"?>\n"
    "<gutenprint xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd\">\n"
    "<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\" piecewise=\"true\">\n"
    "<sequence count=\"5\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0 0.5 0.02 0.6 0.96\n"
    "</sequence>\n"
    "</curve>\n"
    "</gutenprint>\n",

    "<?xml version=\"1.0\"?>\n"
    "<gutenprint xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd\">\n"
    "<curve wrap=\"nowrap\" type=\"linear\" gamma=\"0\" piecewise=\"true\">\n"
    "<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0 0.5 0.02 0.6 0.96 0.51\n"
    "</sequence>\n"
    "</curve>\n"
    "</gutenprint>\n",

    "<?xml version=\"1.0\"?>\n"
    "<gutenprint xmlns=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xsi:schemaLocation=\"http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd\">\n"
    "<curve wrap=\"nowrap\" type=\"linear\" gamma=\"0\" piecewise=\"true\">\n"
    "<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "0.01 0.5 0.02 0.6 1.0 0.51\n"
    "</sequence>\n"
    "</curve>\n"
    "</gutenprint>\n",

    /* Gamma curves */
    /* Incorrect wrap mode */
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gutenprint><curve wrap=\"wrap\" type=\"linear\" gamma=\"1.0\" piecewise=\"false\">\n"
    "<sequence count=\"-1\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "</sequence></curve></gutenprint>\n",

    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gutenprint><curve wrap=\"wrap\" type=\"linear\" gamma=\"1.0\" piecewise=\"false\">\n"
    "<sequence count=\"1\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "</sequence></curve></gutenprint>\n",

    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gutenprint><curve wrap=\"wrap\" type=\"linear\" gamma=\"1.0\" piecewise=\"false\">\n"
    "<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "</sequence></curve></gutenprint>\n",

    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gutenprint><curve wrap=\"wrap\" type=\"linear\" gamma=\"1.0\" piecewise=\"false\">\n"
    "<sequence count=\"0\" lower-bound=\"0\" upper-bound=\"4\">\n"
    "</sequence></curve></gutenprint>\n"
  };

static const int bad_curve_count = sizeof(bad_curves) / sizeof(const char *);

const char *linear_curve_1 =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint><curve wrap=\"nowrap\" type=\"linear\" gamma=\"0\" piecewise=\"false\">\n"
"<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0 0 0 1 1 1"
"</sequence></curve></gutenprint>";

const char *linear_curve_2 =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint><curve wrap=\"wrap\" type=\"linear\" gamma=\"0\" piecewise=\"false\">\n"
"<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0 0 0 1 1 1"
"</sequence></curve></gutenprint>";

const char *linear_curve_3 =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint><curve wrap=\"nowrap\" type=\"linear\" gamma=\"0\" piecewise=\"false\">\n"
"<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0 0 0 1 1 1"
"</sequence></curve></gutenprint>";

const char *linear_curve_4 =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint><curve wrap=\"wrap\" type=\"linear\" gamma=\"0\" piecewise=\"false\">\n"
"<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0 0 0 1 1 1"
"</sequence></curve></gutenprint>";

const char *spline_curve_1 =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint><curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\" piecewise=\"false\">\n"
"<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0 0 0 1 1 1"
"</sequence></curve></gutenprint>";

const char *spline_curve_2 =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint><curve wrap=\"wrap\" type=\"spline\" gamma=\"0\" piecewise=\"false\">\n"
"<sequence count=\"6\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0 0 0 1 1 1"
"</sequence></curve></gutenprint>";

static void
piecewise_curve_checks(stp_curve_t *curve1, int resample_points, int expected)
{
  stp_curve_t *curve2;
  const stp_curve_point_t *curve_points;
  size_t count;
  int i;
  double low;

  TEST("get data points of piecewise curve");
  curve_points = stp_curve_get_data_points(curve1, &count);
  if (curve_points)
    {
      int bad_compare = 0;
      TEST_PASS();
      TEST("Checking count of curve points");
      if (count == expected)
	TEST_PASS();
      else
	{
	  TEST_FAIL();
	  if (!quiet)
	    printf("Expected %d points, got %d\n", expected, count);
	}
      TEST("Comparing data");
      for (i = 0; i < count; i++)
	{
	  if (curve_points[i].x != standard_piecewise_sat_adjustment[i].x ||
	      curve_points[i].y - .0000001 > standard_piecewise_sat_adjustment[i].y ||
	      curve_points[i].y + .0000001 < standard_piecewise_sat_adjustment[i].y)
	    {
	      bad_compare = 1;
	      if (!quiet)
		printf("Miscompare at element %d: (%f, %f) (%f, %f)\n", i,
		       standard_piecewise_sat_adjustment[i].x,
		       standard_piecewise_sat_adjustment[i].y,
		       curve_points[i].x, curve_points[i].y);
	    }
	}
      SIMPLE_TEST_CHECK(!bad_compare);
    }
  else
    TEST_FAIL();

  TEST("get sequence of piecewise curve (PASS is an expected failure)");
  SIMPLE_TEST_CHECK(!stp_curve_get_sequence(curve1));

  TEST("get data of piecewise curve (PASS is an expected failure)");
  SIMPLE_TEST_CHECK(!stp_curve_get_data(curve1, &count));

  TEST("set data point of piecewise curve (PASS is an expected failure)");
  SIMPLE_TEST_CHECK(!stp_curve_set_point(curve1, 2, 1.0));

  TEST("get data point of piecewise curve (PASS is an expected failure)");
  SIMPLE_TEST_CHECK(!stp_curve_get_point(curve1, 2, &low));

  TEST("interpolate piecewise curve (PASS is an expected failure)");
  SIMPLE_TEST_CHECK(!stp_curve_interpolate_value(curve1, .5, &low));

  TEST("rescale piecewise curve");
  SIMPLE_TEST_CHECK(stp_curve_rescale(curve1, .5, STP_CURVE_COMPOSE_ADD,
				      STP_CURVE_BOUNDS_RESCALE));

  TEST("get float data of piecewise curve (PASS is an expected failure)");
  SIMPLE_TEST_CHECK(!stp_curve_get_float_data(curve1, &count));

  TEST("get subrange on piecewise curve (PASS is an expected failure)");
  SIMPLE_TEST_CHECK(!(curve2 = stp_curve_get_subrange(curve1, 0, 2)));
  if (!quiet && curve2)
    stp_curve_write(stdout, curve2);

  TEST("set subrange on piecewise curve (PASS is an expected failure)");
  curve2 = stp_curve_create_from_string(linear_curve_2);
  if (stp_curve_set_subrange(curve1, curve2, 1))
    {
      TEST_FAIL();
      if (!quiet)
	stp_curve_write(stdout, curve2);
    }
  else
    TEST_PASS();

  if (resample_points > 0)
    {
      char tmpbuf[64];
      sprintf(tmpbuf, "resample piecewise curve to %d points", resample_points);
      TEST(tmpbuf);
      SIMPLE_TEST_CHECK(stp_curve_resample(curve1, resample_points));

      TEST("resampled curve is not piecewise");
      SIMPLE_TEST_CHECK(!stp_curve_is_piecewise(curve1));

      TEST("get data points of piecewise copy (PASS is an expected failure)");
      SIMPLE_TEST_CHECK(!stp_curve_get_data_points(curve1, &count));

      TEST("get sequence of piecewise copy");
      SIMPLE_TEST_CHECK(stp_curve_get_sequence(curve1));

      TEST("get data of piecewise copy");
      SIMPLE_TEST_CHECK(stp_curve_get_data(curve1, &count));

      TEST("set data point of piecewise copy");
      SIMPLE_TEST_CHECK(stp_curve_set_point(curve1, 2, 0.51));

      TEST("get data point of piecewise copy");
      SIMPLE_TEST_CHECK(stp_curve_get_point(curve1, 2, &low));

      TEST("interpolate piecewise copy");
      SIMPLE_TEST_CHECK(stp_curve_interpolate_value(curve1, .5, &low));

      TEST("rescale piecewise copy");
      SIMPLE_TEST_CHECK(stp_curve_rescale(curve1, 2,
					  STP_CURVE_COMPOSE_MULTIPLY,
					  STP_CURVE_BOUNDS_RESCALE));

      TEST("get float data of piecewise copy");
      SIMPLE_TEST_CHECK(stp_curve_get_float_data(curve1, &count));

      TEST("get subrange on piecewise copy");
      SIMPLE_TEST_CHECK((curve2 = stp_curve_get_subrange(curve1, 0, 2)));
      if (verbose && curve2)
	stp_curve_write(stdout, curve2);

      if (resample_points > 10)
	{
	  TEST("set subrange on piecewise copy");
	  curve2 = stp_curve_create_from_string(linear_curve_2);
	  if (verbose)
	    stp_curve_write(stdout, curve1);
	  if (stp_curve_set_subrange(curve1, curve2, 4))
	    {
	      TEST_PASS();
	      if (verbose)
		stp_curve_write(stdout, curve1);
	    }
	  else
	    TEST_FAIL();
	}
    }
}

int
main(int argc, char **argv)
{
  char *tmp;
  int i;
  size_t count;
  double low, high;

  stp_curve_t *curve1;
  stp_curve_t *curve2;
  stp_curve_t *curve3;
  const stp_curve_point_t *curve_points;

  while (1)
    {
#ifdef __GNU_LIBRARY__
      int option_index = 0;
      int c = getopt_long(argc, argv, "qv", optlist, &option_index);
#else
      int c = getopt(argc, argv, "qv");
#endif
      if (c == -1)
	break;
      switch (c)
	{
	case 'q':
	  quiet = 1;
	  verbose = 0;
	  break;
	case 'v':
	  quiet = 0;
	  verbose = 1;
	  break;
	default:
	  break;
	}
    }

  stp_init();

  TEST("creation of XML string from curve");
  curve1 = stp_curve_create(STP_CURVE_WRAP_AROUND);
  stp_curve_set_bounds(curve1, 0.0, 4.0);
  stp_curve_set_data(curve1, 48, standard_sat_adjustment);
  tmp = stp_curve_write_string(curve1);
  stp_curve_destroy(curve1);
  curve1 = NULL;
  SIMPLE_TEST_CHECK(tmp);
  if (verbose)
    printf("%s\n", tmp);


  TEST("creation of curve from XML string (stp_curve_create_from_string)");
  SIMPLE_TEST_CHECK((curve2 = stp_curve_create_from_string(tmp)));
  free(tmp);

  TEST("stp_curve_resample");
  if (curve2 != NULL && stp_curve_resample(curve2, 95) == 0)
    {
      TEST_FAIL();
    }
  else
    {
      TEST_PASS();
      if (verbose)
	stp_curve_write(stdout, curve2);
    }
  if (curve2)
    {
      stp_curve_destroy(curve2);
      curve2 = NULL;
    }

  if (!quiet)
    printf("Testing known bad curves...\n");
  for (i = 0; i < bad_curve_count; i++)
    {
      stp_curve_t *bad = NULL;
      TEST("BAD curve (PASS is an expected failure)");
      if ((bad = stp_curve_create_from_string(bad_curves[i])) != NULL)
	{
	  TEST_FAIL();
	  if (!quiet)
	    {
	      stp_curve_write(stdout, bad);
	      printf("\n");
	    }
	  stp_curve_destroy(bad);
	  bad = NULL;
	}
      else
	TEST_PASS();
    }

  if (!quiet)
    printf("Testing known good curves...\n");
  for (i = 0; i < good_curve_count; i++)
    {
      if (curve2)
	{
	  stp_curve_destroy(curve2);
	  curve2 = NULL;
	}
      TEST("GOOD curve");
      if ((curve2 = stp_curve_create_from_string(good_curves[i])) != NULL)
	{
	  TEST_PASS();
	  tmp = stp_curve_write_string(curve2);
	  TEST("whether XML curve is identical to original");
	  if (tmp && strcmp((const char *) tmp, good_curves[i]))
	    {
	      TEST_FAIL();
	      if (!quiet)
		{
		  printf("XML:\n");
		  printf("%s", tmp);
		  printf("Original:\n");
		  printf("%s", good_curves[i]);
		  printf("End:\n");
		}
	    }
	  else
	    {
	      TEST_PASS();
	      if (verbose)
		printf("%s", tmp);
	    }
	  free(tmp);
	}
      else
	TEST_FAIL();
    }
  if (curve2)
    {
      stp_curve_destroy(curve2);
      curve2 = NULL;
    }
  if (verbose)
    printf("Allocate 1\n");
  curve1 = stp_curve_create(STP_CURVE_WRAP_NONE);
  if (verbose)
    printf("Allocate 2\n");
  curve2 = stp_curve_create(STP_CURVE_WRAP_NONE);
  TEST("set curve 1 gamma");
  SIMPLE_TEST_CHECK(stp_curve_set_gamma(curve1, 1.2));
  if (verbose)
    stp_curve_write(stdout, curve1);
  TEST("set curve 2 gamma");
  SIMPLE_TEST_CHECK(stp_curve_set_gamma(curve2, -1.2));
  if (verbose)
    stp_curve_write(stdout, curve2);

  TEST("compose add from gamma curves");
  SIMPLE_TEST_CHECK(stp_curve_compose(&curve3, curve1, curve2,
				      STP_CURVE_COMPOSE_ADD, 64));
  if (verbose && curve3)
    stp_curve_write(stdout, curve3);

  TEST("resample curve 1");
  SIMPLE_TEST_CHECK(stp_curve_resample(curve1, 64));
  if (verbose && curve1)
    stp_curve_write(stdout, curve1);
  if (curve3)
    {
      stp_curve_destroy(curve3);
      curve3 = NULL;
    }
  TEST("compose multiply from gamma curves");
  if (!stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_MULTIPLY, 64))
    TEST_FAIL();
  else
    {
      TEST_PASS();
      if (verbose)
	{
	  stp_curve_write(stdout, curve1);
	  stp_curve_write(stdout, curve2);
	  stp_curve_write(stdout, curve3);
	}
    }

  TEST("compose add from non-gamma curves");
  stp_curve_destroy(curve1);
  stp_curve_destroy(curve2);
  stp_curve_destroy(curve3);
  curve1 = stp_curve_create_from_string(good_curves[0]);
  curve2 = stp_curve_create_from_string(linear_curve_2);
  SIMPLE_TEST_CHECK(stp_curve_compose(&curve3, curve1, curve2,
				      STP_CURVE_COMPOSE_ADD, 64));
  if (verbose && curve3)
    stp_curve_write(stdout, curve3);

  TEST("resample curve 1");
  SIMPLE_TEST_CHECK(stp_curve_resample(curve1, 64));
  if (verbose && curve1)
    stp_curve_write(stdout, curve1);
  if (curve3)
    {
      stp_curve_destroy(curve3);
      curve3 = NULL;
    }
  TEST("compose multiply from non-gamma curves");
  if (!stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_MULTIPLY, 64))
    TEST_FAIL();
  else
    {
      TEST_PASS();
      if (verbose)
	{
	  stp_curve_write(stdout, curve1);
	  stp_curve_write(stdout, curve2);
	  stp_curve_write(stdout, curve3);
	}
    }
  if (curve3)
    {
      stp_curve_destroy(curve3);
      curve3 = NULL;
    }

  TEST("multiply rescale");
  SIMPLE_TEST_CHECK(stp_curve_rescale(curve2, -1, STP_CURVE_COMPOSE_MULTIPLY,
				      STP_CURVE_BOUNDS_RESCALE));
  if (verbose && curve2)
    stp_curve_write(stdout, curve2);
  TEST("subtract compose");
  SIMPLE_TEST_CHECK(stp_curve_compose(&curve3, curve1, curve2,
				      STP_CURVE_COMPOSE_ADD, 64));
  if (verbose && curve3)
    stp_curve_write(stdout, curve3);

  if (curve3)
    {
      stp_curve_destroy(curve3);
      curve3 = NULL;
    }
  if (curve1)
    {
      stp_curve_destroy(curve1);
      curve1 = NULL;
    }
  if (curve2)
    {
      stp_curve_destroy(curve2);
      curve2 = NULL;
    }

  TEST("spline curve 1 creation");
  SIMPLE_TEST_CHECK((curve1 = stp_curve_create_from_string(spline_curve_1)));
  TEST("spline curve 2 creation");
  SIMPLE_TEST_CHECK((curve2 = stp_curve_create_from_string(spline_curve_2)));
  if (curve1)
    {
      if (verbose)
	stp_curve_write(stdout, curve1);
      TEST("spline curve 1 resample 1");
      SIMPLE_TEST_CHECK(stp_curve_resample(curve1, 41));
      if (verbose && curve1)
	stp_curve_write(stdout, curve1);
      TEST("spline curve 1 resample 2");
      SIMPLE_TEST_CHECK(stp_curve_resample(curve1, 83));
      if (verbose && curve1)
	stp_curve_write(stdout, curve1);
    }
  if (curve2)
    {
      if (verbose)
	stp_curve_write(stdout, curve2);
      TEST("spline curve 2 resample");
      SIMPLE_TEST_CHECK(stp_curve_resample(curve2, 48));
      if (verbose && curve2)
	stp_curve_write(stdout, curve2);
    }
  TEST("compose add (PASS is an expected failure)");
  if (curve1 && curve2 &&
      stp_curve_compose(&curve3, curve1, curve2, STP_CURVE_COMPOSE_MULTIPLY, -1))
    {
      TEST_FAIL();
      if (!quiet)
	printf("compose with different wrap mode should fail!\n");
    }
  else
    TEST_PASS();
  if (curve1)
    {
      stp_curve_destroy(curve1);
      curve1 = NULL;
    }
  if (curve2)
    {
      stp_curve_destroy(curve2);
      curve2 = NULL;
    }

  TEST("linear curve 1 creation");
  SIMPLE_TEST_CHECK((curve1 = stp_curve_create_from_string(linear_curve_1)));
  TEST("linear curve 2 creation");
  SIMPLE_TEST_CHECK((curve2 = stp_curve_create_from_string(linear_curve_2)));

  TEST("get data points of dense curve (PASS is an expected failure)");
  curve_points = stp_curve_get_data_points(curve2, &count);
  SIMPLE_TEST_CHECK(!curve_points);

  if (curve1)
    {
      if (verbose)
	stp_curve_write(stdout, curve1);
      TEST("linear curve 1 resample");
      SIMPLE_TEST_CHECK(stp_curve_resample(curve1, 41));
      if (verbose && curve1)
	stp_curve_write(stdout, curve1);
      stp_curve_destroy(curve1);
      curve1 = NULL;
    }
  if (curve2)
    {
      if (verbose)
	stp_curve_write(stdout, curve2);
      TEST("linear curve 2 resample");
      SIMPLE_TEST_CHECK(stp_curve_resample(curve2, 48));
      if (verbose && curve2)
	stp_curve_write(stdout, curve2);
      stp_curve_destroy(curve2);
      curve2 = NULL;
    }

  curve1 = stp_curve_create(STP_CURVE_WRAP_AROUND);
  stp_curve_set_interpolation_type(curve1, STP_CURVE_TYPE_SPLINE);
  stp_curve_set_bounds(curve1, 0.0, 4.0);
  stp_curve_set_data(curve1, 48, standard_sat_adjustment);
  TEST("setting curve data");
  SIMPLE_TEST_CHECK(curve1 && (stp_curve_count_points(curve1) == 48));
  if (verbose)
    stp_curve_write(stdout, curve1);
  TEST("curve resample");
  SIMPLE_TEST_CHECK(stp_curve_resample(curve1, 384));
  if (verbose)
    stp_curve_write(stdout, curve1);
  TEST("very large curve resample");
  SIMPLE_TEST_CHECK(stp_curve_resample(curve1, 65535));
  TEST("offsetting large curve");
#if 0
  stp_curve_get_range(curve1, &low, &high);
  fprintf(stderr, "Result original max %f min %f\n", high, low);
  stp_curve_get_bounds(curve1, &low, &high);
  fprintf(stderr, "Bounds original max %f min %f\n", high, low);
#endif
  SIMPLE_TEST_CHECK(stp_curve_rescale(curve1, 2.0, STP_CURVE_COMPOSE_ADD,
				      STP_CURVE_BOUNDS_RESCALE));

  stp_curve_rescale(curve1, 1.0, STP_CURVE_COMPOSE_ADD,
		    STP_CURVE_BOUNDS_RESCALE);
  stp_curve_get_bounds(curve1, &low, &high);
  stp_curve_rescale(curve1, 0.0, STP_CURVE_COMPOSE_ADD,
		    STP_CURVE_BOUNDS_RESCALE);
  stp_curve_get_bounds(curve1, &low, &high);
  stp_curve_rescale(curve1, 0.0, STP_CURVE_COMPOSE_ADD,
		    STP_CURVE_BOUNDS_RESCALE);
  stp_curve_get_bounds(curve1, &low, &high);
  stp_curve_rescale(curve1, 0.0, STP_CURVE_COMPOSE_ADD,
		    STP_CURVE_BOUNDS_RESCALE);
  stp_curve_get_bounds(curve1, &low, &high);
  TEST("writing very large curve to string");
  tmp = stp_curve_write_string(curve1);
  if (tmp == NULL)
    TEST_FAIL();
  else
    TEST_PASS();
  if (tmp)
    {
      TEST("reading back very large curve");
      curve2 = stp_curve_create_from_string(tmp);
      SIMPLE_TEST_CHECK(stp_curve_count_points(curve2) == 65535);

      free(tmp);
      TEST("Rescaling readback");
      SIMPLE_TEST_CHECK(stp_curve_rescale(curve2, -1.0,
					  STP_CURVE_COMPOSE_MULTIPLY,
					  STP_CURVE_BOUNDS_RESCALE));
      TEST("Adding curves");
      SIMPLE_TEST_CHECK(stp_curve_compose(&curve3, curve1, curve2,
					  STP_CURVE_COMPOSE_ADD, 65535));

      stp_curve_get_range(curve3, &low, &high);
      TEST("Comparing results");
      SIMPLE_TEST_CHECK(low < .00001 && low > -.00001 &&
			high < .00001 && high > -.00001);

      if (curve1)
	{
	  stp_curve_destroy(curve1);
	  curve1 = NULL;
	}
    }

  TEST("Creating piecewise wrap-around curve");
  curve1 = stp_curve_create(STP_CURVE_WRAP_AROUND);
  stp_curve_set_bounds(curve1, 0.0, 4.0);

  SIMPLE_TEST_CHECK(stp_curve_set_data_points
		    (curve1, 48, standard_piecewise_sat_adjustment));

  TEST("Writing piecewise wrap-around curve to string");
  tmp = stp_curve_write_string(curve1);
  SIMPLE_TEST_CHECK(tmp);
  if (verbose)
    printf("%s\n", tmp);

  TEST("Check curve is piecewise");
  SIMPLE_TEST_CHECK(stp_curve_is_piecewise(curve1));

  TEST("Create copy of piecewise curve");
  curve2 = stp_curve_create_copy(curve1);
  SIMPLE_TEST_CHECK(curve2);

  TEST("Check copy is piecewise");
  SIMPLE_TEST_CHECK(stp_curve_is_piecewise(curve2));

  piecewise_curve_checks(curve1, 0, 48);
  stp_curve_rescale(curve1, -.5, STP_CURVE_COMPOSE_ADD,
		    STP_CURVE_BOUNDS_RESCALE);
  piecewise_curve_checks(curve2, 3, 48);

  TEST("Copy in place piecewise curve");
  stp_curve_copy(curve2, curve1);
  SIMPLE_TEST_CHECK(curve1);
  piecewise_curve_checks(curve2, 10, 48);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 15, 48);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 47, 48);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 48, 48);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 49, 48);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 50, 48);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 100, 48);
  stp_curve_destroy(curve1);

  TEST("Creating piecewise no-wrap curve with not enough data (PASS is an expected failure)");
  curve1 = stp_curve_create(STP_CURVE_WRAP_NONE);
  stp_curve_set_bounds(curve1, 0.0, 4.0);
  SIMPLE_TEST_CHECK(!stp_curve_set_data_points
		    (curve1, 48, standard_piecewise_sat_adjustment));

  TEST("Creating piecewise no-wrap curve correctly");
  SIMPLE_TEST_CHECK(stp_curve_set_data_points
		    (curve1, 49, standard_piecewise_sat_adjustment));

  TEST("Writing piecewise no-wrap curve to string");
  tmp = stp_curve_write_string(curve1);
  SIMPLE_TEST_CHECK(tmp);
  if (verbose)
    printf("%s\n", tmp);

  TEST("Check curve is piecewise");
  SIMPLE_TEST_CHECK(stp_curve_is_piecewise(curve1));

  TEST("Create copy of piecewise curve");
  curve2 = stp_curve_create_copy(curve1);
  SIMPLE_TEST_CHECK(curve2);
  TEST("Check copy is piecewise");
  SIMPLE_TEST_CHECK(stp_curve_is_piecewise(curve2));

  piecewise_curve_checks(curve1, 0, 49);
  stp_curve_rescale(curve1, -.5, STP_CURVE_COMPOSE_ADD,
		    STP_CURVE_BOUNDS_RESCALE);
  piecewise_curve_checks(curve2, 3, 49);

  TEST("Copy in place piecewise curve");
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 10, 49);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 15, 49);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 47, 49);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 48, 49);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 49, 49);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 50, 49);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 100, 49);


  TEST("Creating piecewise spline wrap-around curve");
  curve1 = stp_curve_create(STP_CURVE_WRAP_AROUND);
  stp_curve_set_interpolation_type(curve1, STP_CURVE_TYPE_SPLINE);
  stp_curve_set_bounds(curve1, 0.0, 4.0);

  SIMPLE_TEST_CHECK(stp_curve_set_data_points
		    (curve1, 48, standard_piecewise_sat_adjustment));

  TEST("Writing piecewise wrap-around curve to string");
  tmp = stp_curve_write_string(curve1);
  SIMPLE_TEST_CHECK(tmp);
  if (verbose)
    printf("%s\n", tmp);

  TEST("Check curve is piecewise");
  SIMPLE_TEST_CHECK(stp_curve_is_piecewise(curve1));

  TEST("Create copy of piecewise curve");
  curve2 = stp_curve_create_copy(curve1);
  SIMPLE_TEST_CHECK(curve2);
  TEST("Check copy is piecewise");
  SIMPLE_TEST_CHECK(stp_curve_is_piecewise(curve2));

  piecewise_curve_checks(curve1, 0, 48);
  stp_curve_rescale(curve1, -.5, STP_CURVE_COMPOSE_ADD,
		    STP_CURVE_BOUNDS_RESCALE);
  piecewise_curve_checks(curve2, 3, 48);

  TEST("Copy in place piecewise curve");
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 10, 48);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 15, 48);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 47, 48);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 48, 48);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 49, 48);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 50, 48);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 100, 48);
  stp_curve_destroy(curve1);

  TEST("Creating piecewise spline wrap-around curve");
  curve1 = stp_curve_create(STP_CURVE_WRAP_NONE);
  stp_curve_set_interpolation_type(curve1, STP_CURVE_TYPE_SPLINE);
  stp_curve_set_bounds(curve1, 0.0, 4.0);

  SIMPLE_TEST_CHECK(stp_curve_set_data_points
		    (curve1, 49, standard_piecewise_sat_adjustment));

  TEST("Writing piecewise no-wrap curve to string");
  tmp = stp_curve_write_string(curve1);
  SIMPLE_TEST_CHECK(tmp);
  if (verbose)
    printf("%s\n", tmp);

  TEST("Check curve is piecewise");
  SIMPLE_TEST_CHECK(stp_curve_is_piecewise(curve1));

  TEST("Create copy of piecewise curve");
  curve2 = stp_curve_create_copy(curve1);
  SIMPLE_TEST_CHECK(curve2);
  TEST("Check copy is piecewise");
  SIMPLE_TEST_CHECK(stp_curve_is_piecewise(curve2));

  piecewise_curve_checks(curve1, 0, 49);
  stp_curve_rescale(curve1, -.5, STP_CURVE_COMPOSE_ADD,
		    STP_CURVE_BOUNDS_RESCALE);
  piecewise_curve_checks(curve2, 3, 49);

  TEST("Copy in place piecewise curve");
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 10, 49);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 15, 49);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 47, 49);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 48, 49);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 49, 49);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 50, 49);
  stp_curve_copy(curve2, curve1);
  piecewise_curve_checks(curve2, 100, 49);

  TEST("Create small piecewise curve");
  curve1 = stp_curve_create_from_string(small_piecewise_curve);
  SIMPLE_TEST_CHECK(curve1);

  stp_curve_destroy(curve1);
  curve1 = NULL;
  stp_curve_destroy(curve2);
  curve2 = NULL;
  stp_curve_destroy(curve3);
  curve3 = NULL;

  if (global_error_count)
    {
      printf("%d/%d tests FAILED.\n", global_error_count, global_test_count);
      printf("Failures:\n");
      while (test_failure_head)
	{
	  printf("  %3d\n", test_failure_head->test_number);
	  test_failure_head = test_failure_head->next;
	}
    }
  else
    printf("All tests passed successfully.\n");
  return global_error_count ? 1 : 0;
}
