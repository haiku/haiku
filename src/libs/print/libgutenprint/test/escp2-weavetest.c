/*
 * "$Id: escp2-weavetest.c,v 1.37 2006/05/05 23:23:40 rlk Exp $"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
 *
 *   Copyright 1999-2000 Robert Krawitz (rlk@alum.mit.edu)
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
 * Test for the soft weave algorithm.  This program calculates the weave
 * parameters for each input line and verifies that a number of conditions
 * are met.  Currently, the conditions checked are:
 *
 * 1) Pass # is >= 0
 * 2) The nozzle is within the physical bounds of the printer
 * 3) The computed starting row of the pass is not greater than the row
 *    index itself.
 * 4) The computed end of the pass is not less than the row index itself.
 * 5) If this row is the last row of a pass, that the last pass completed
 *    was one less than the current pass that we're just completing.
 * 6) For a given pass, the computed starting row of the pass is the same
 *    for all rows comprising the pass.
 * 7) For a given pass, the computed last row of the pass is the same
 *    for all rows comprising the pass.
 * 8) The input row is the same as the row computed by the algorithm.
 * 9) If there are phantom rows within the pass (unused jets before the
 *    first jet that is actually printing anything), then the number of
 *    phantom rows is not less than zero nor greater than or equal to the
 *    number of physical jets in the printer.
 * 10) The physical starting row of the pass (disregarding any phantom
 *    rows) is at least zero.
 * 11) If there are phantom rows, the number of phantom rows is less than
 *    the current nozzle number.
 * 12) If we are using multiple subpasses for each row, that each row within
 *    this pass is part of the same logical subpass.  Thus, if the pass in
 *    question is the first subpass for a given row, that it is the first
 *    subpass for ALL rows comprising the pass.  This doesn't matter if we're
 *    simply overlaying data; it's important if we have to shift the print head
 *    slightly between each pass to accomplish high-resolution printing.
 * 13) We are not overprinting a specified (row, subpass) pair.
 *
 * In addition to these per-row checks, we calculate the following global
 * correctness checks:
 *
 * 1) No pass starts (logically) at a later row than an earlier pass.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#define DEBUG_SIGNAL
#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#include <gutenprint/gutenprint.h>
#include <gutenprint-internal.h>

const char header[] = "Legend:\n"
"A  Negative pass number.\n"
"B  Jet number out of range.\n"
"C  Starting row of this pass after the current row.\n"
"D  Ending row of this pass before the current row.\n"
"E  Current row is the ending row of the pass, and the pass number is not\n"
"   one greater than the previous completed pass.\n"
"F  The current pass's starting row is not consistent with the previously\n"
"   observed starting row of this pass.\n"
"G  The current pass's ending row is not consistent with the previously\n"
"   observed ending row of this pass.\n"
"H  The current row does not match the computed current row based on jet and\n"
"   start of pass.\n"
"I  The number of missing start rows is less than zero or greater than or\n"
"   equal to the actual number of jets.\n"
"J  The first printed row of this pass is less than zero.\n"
"K  The number of missing start rows of this pass is greater than the\n"
"   jet used to print the current row.\n"
"L  The subpass printed by the current pass is not consistent with an earlier\n"
"   record of the subpass printed by this pass.\n"
"M  The same physical row is being printed more than once.\n"
"N  Two different active passes are in the same slot.\n"
"O  Number of missing start rows is incorrect.\n"
"P  Physical row number out of bounds.\n"
"Q  Pass starts earlier than a prior pass.\n"
"R  Pass is never flushed.\n"
"S  Pass is flushed but not created.\n";

int *passes_flushed;

static void
print_header(void)
{
  printf("%s", header);
}

static void
flush_pass(stp_vars_t *v, int passno, int vertical_subpass)
{
  passes_flushed[passno] = 1;
}

static void
writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  fwrite(buf, 1, bytes, prn);
}

static int
run_one_weavetest(int physjets, int physsep, int hpasses, int vpasses,
		  int subpasses, int nrows, int first_line, int phys_lines,
		  int color_jet_arrangement, int strategy, int quiet)
{
  int i;
  int j;
  stp_weave_t w;
  stp_vars_t *v;
  int errors[26];
  char errcodes[26];
  int total_errors = 0;
  int lastpass = -1;
  int newestpass = -1;
  int *passstarts;
  int *logpassstarts;
  int *passends;
  int *passcounts;
  signed char *physpassstuff;
  signed char *rowdetail;
  int *current_slot;
  int vmod;
  int head_offset[8];
  int last_good_pass;

  v = stp_vars_create();
  stp_set_outfunc(v, writefunc);
  stp_set_errfunc(v, writefunc);
  stp_set_outdata(v, stdout);
  stp_set_errdata(v, stdout);

  memset(errors, 0, sizeof(int) * 26);
#if 0
  if (physjets < hpasses * vpasses * subpasses)
    {
      return 0;
    }
#endif

  for(i=0; i<8; i++)
    head_offset[i] = 0;

  switch (color_jet_arrangement)
    {
    case 1:			/* C80-type */
      head_offset[0] = (physjets+1)*physsep;
      head_offset[1] = (physjets+1)*physsep;
      head_offset[2] = 2*(physjets+1)*physsep;
      phys_lines += 2*(physjets+1)*physsep;
      break;
    case 2:			/* Offset by 1 (f360) */
      if (physsep % 2 == 0)
	{
	  head_offset[1] = physsep / 2;
	  head_offset[2] = physsep / 2;
	  phys_lines += physsep / 2;
	}
      break;
    case 3:			/* Offset by 1 or 2 (cx3650-type) */
      if (physsep % 3 == 0)
	{
	  head_offset[1] = physsep * 2 / 3;
	  head_offset[2] = physsep / 3;
	  phys_lines += physsep * 2 / 3;
	}
      break;
    default:			/* Normal */
      break;
    }


  stp_initialize_weave(v, physjets, physsep, hpasses, vpasses, subpasses,
		       7, 1, 128, nrows, first_line,
		       phys_lines, head_offset, strategy, flush_pass,
		       stp_fill_tiff, stp_pack_tiff,
		       stp_compute_tiff_linewidth);

  passstarts = stp_malloc(sizeof(int) * (nrows + physsep));
  logpassstarts = stp_malloc(sizeof(int) * (nrows + physsep));
  passends = stp_malloc(sizeof(int) * (nrows + physsep));
  passcounts = stp_malloc(sizeof(int) * (nrows + physsep));
  passes_flushed = stp_malloc(sizeof(int) * (nrows + physsep));
  vmod = 2 * physsep * hpasses * vpasses * subpasses;
  if (vmod == 0)
    vmod = 1;
  current_slot = stp_malloc(sizeof(int) * vmod);
  physpassstuff = stp_malloc((nrows + physsep));
  rowdetail = stp_malloc((nrows + physsep) * physjets);
  memset(rowdetail, 0, (nrows + physsep) * physjets);
  memset(physpassstuff, -1, (nrows + physsep));
  memset(current_slot, 0, (sizeof(int) * vmod));
  if (!quiet)
    {
      print_header();
      printf("%15s %5s %5s %5s %10s %10s %10s %10s\n", "", "row", "pass",
	     "jet", "missing", "logical", "physstart", "physend");
    }
  for (i = 0; i < vmod; i++)
    current_slot[i] = -1;
  for (i = 0; i < (nrows + physsep); i++)
    {
      passes_flushed[i] = 0;
      logpassstarts[i] = INT_MIN;
      passstarts[i] = -1;
      passends[i] = -1;
    }
  memset(errcodes, ' ', 26);
  for (i = 0; i < nrows; i++)
    {
      for (j = 0; j < hpasses * vpasses * subpasses; j++)
	{
	  int physrow;
	  stp_weave_parameters_by_row(v, i+first_line, j, &w);
	  physrow = w.logicalpassstart + physsep * w.jet;

	  errcodes[0] = (w.pass < 0 ? (errors[0]++, 'A') : ' ');
	  errcodes[1] = (w.jet < 0 || w.jet > physjets - 1 ?
			 (errors[1]++, 'B') : ' ');
	  errcodes[2] = (w.physpassstart > w.row ? (errors[2]++, 'C') : ' ');
	  errcodes[3] = (w.physpassend < w.row ? (errors[3]++, 'D') : ' ');
#if 0
	  errcodes[4] = (w.physpassend == w.row && lastpass + 1 != w.pass ?
			 (errors[4]++, 'E') : ' ');
#endif
	  errcodes[5] = (w.pass >= 0 && w.pass < nrows &&
			 passstarts[w.pass] != -1 &&
			 passstarts[w.pass] != w.physpassstart ?
			 (errors[5]++, 'F') : ' ');
	  errcodes[6] = (w.pass >= 0 && w.pass < nrows &&
			 passends[w.pass] != -1 &&
			 passends[w.pass] != w.physpassend ?
			 (errors[6]++, 'G') : ' ');
	  errcodes[7] = (w.row != physrow ? (errors[7]++, 'H') : ' ');
	  errcodes[8] = (w.missingstartrows < 0 ||
			 w.missingstartrows > physjets - 1 ?
			 (errors[8]++, 'I') : ' ');
	  errcodes[9] = (w.physpassstart < 0 ? (errors[9]++, 'J') : ' ');
	  errcodes[10] = (w.missingstartrows > w.jet ?
			  (errors[10]++, 'K') : ' ');
	  errcodes[11] = (w.pass >= 0 && w.pass < nrows &&
			  physpassstuff[w.pass] >= 0 &&
			  physpassstuff[w.pass] != j ?
			  (errors[11]++, 'L') : ' ');
	  errcodes[12] = (w.pass >= 0 && w.pass < nrows && w.jet >= 0 &&
			  w.jet < physjets &&
			  rowdetail[w.pass * physjets + w.jet] == 1 ?
			  (errors[12]++, 'M') : ' ');
	  errcodes[13] = (current_slot[w.pass % vmod] != -1 &&
			  current_slot[w.pass % vmod] != w.pass ?
			  (errors[13]++, 'N') : ' ');
	  errcodes[14] = (w.physpassstart == w.row &&
			  w.jet != w.missingstartrows ?
			  (errors[14]++, 'O') : ' ');
	  errcodes[15] = ((w.logicalpassstart < 0) ||
			  (w.logicalpassstart + physsep * (physjets - 1) >=
			   phys_lines) ?
			  (errors[15]++, 'P') : ' ');
	  errcodes[16] = '\0';

	  if (!quiet)
	    {
	      printf("%15s%5d %5d %5d %10d %10d %10d %10d\n",
		     errcodes, w.row, w.pass, w.jet, w.missingstartrows,
		     w.logicalpassstart, w.physpassstart, w.physpassend);
	      fflush(stdout);
	    }
	  if (w.pass >= 0 && w.pass < (nrows + physsep))
	    {
	      if (w.physpassend == w.row)
		{
		  lastpass = w.pass;
		  passends[w.pass] = -2;
		  current_slot[w.pass % vmod] = -1;
		}
	      else
		{
		  passends[w.pass] = w.physpassend;
		  current_slot[w.pass % vmod] = w.pass;
		}
	      passstarts[w.pass] = w.physpassstart;
	      logpassstarts[w.pass] = w.logicalpassstart;
	      if (w.jet >= 0 && w.jet < physjets)
		rowdetail[w.pass * physjets + w.jet] = 1;
	      if (physpassstuff[w.pass] == -1)
		physpassstuff[w.pass] = j;
	      if (w.pass > newestpass)
		newestpass = w.pass;
	    }
	}
    }
  if (!quiet)
    printf("Unterminated passes:\n");
  for (i = 0; i <= newestpass; i++)
    if (passends[i] >= -1 && passends[i] < nrows && logpassstarts[i] != INT_MIN)
      {
	if (!quiet)
	  printf("%d %d\n", i, passends[i]);
	errors[16]++;
      }
  if (!quiet)
    {
      printf("Last terminated pass: %d\n", lastpass);
      printf("Pass starts:\n");
    }
  last_good_pass = 0;
  errcodes[3] = '\0';
  for (i = 0; i <= newestpass; i++)
    {
      if (logpassstarts[i] != INT_MIN)
	{
	  if (i > 0)
	    errcodes[0] = logpassstarts[i] < logpassstarts[last_good_pass] ?
	      (errors[17]++, 'Q') : ' ';
	  errcodes[1] = passes_flushed[i] ? (errors[18]++, 'R') : ' ';
	  errcodes[2] = ' ';
	  if (!quiet)
	    printf("%s %d %d\n", errcodes, i, logpassstarts[i]);
	  last_good_pass = i;
	}
      else if (!quiet)
	{
	  errcodes[0] = ' ';
	  errcodes[1] = ' ';
	  errcodes[2] = '*';
	  printf("%s %d\n", errcodes, i);
	}
    }
  for (i = 0; i < 26; i++)
    total_errors += errors[i];
  stp_free(rowdetail);
  stp_free(physpassstuff);
  stp_free(current_slot);
  stp_free(passcounts);
  stp_free(passends);
  stp_free(logpassstarts);
  stp_free(passstarts);
  stp_free(passes_flushed);
  if (!quiet || (quiet == 1 && total_errors > 0))
    printf("%d total error%s\n", total_errors, total_errors == 1 ? "" : "s");
  stp_vars_destroy(v);
  return total_errors;
}

static int
run_weavetest_from_stdin(void)
{
  int nrows;
  int physjets;
  int physsep;
  int hpasses, vpasses, subpasses;
  int first_line, phys_lines;
  int color_jet_arrangement;
  int strategy;
  int previous_strategy = -1;
  int previous_jets = -1;
  int previous_separation = -1;
  int total_cases = 0;
  int failures = 0;
  char linebuf[4096];
  const char *spinner = "/-\\|";
  int rotor = 0;
  int do_spinner = isatty(fileno(stdout));
  while (fgets(linebuf, 4096, stdin))
    {
      int retval;
      (void) sscanf(linebuf, "%d%d%d%d%d%d%d%d%d%d", &physjets, &physsep,
		    &hpasses, &vpasses, &subpasses, &nrows, &first_line,
		    &phys_lines, &color_jet_arrangement, &strategy);
      fflush(stdout);
      if (vpasses * subpasses > physjets)
	continue;
      retval = run_one_weavetest(physjets, physsep, hpasses, vpasses,
				 subpasses, nrows, first_line, phys_lines,
				 color_jet_arrangement, strategy, 2);
      if (getenv("QUIET"))
	{
	  /* Assume that we're running within run-weavetest, and */
	  /* print out the heartbeat */
	      
	  if (previous_strategy != strategy)
	    {
	      printf("%s%d:", previous_strategy == -1 ? "" : "\n", strategy);
	      previous_jets = -1;
	      previous_separation = -1;
	    }
	  if (previous_jets != physjets)
	    {
	      printf("%d", physjets);
	      previous_separation = -1;
	    }
	  if (previous_separation != physsep)
	    printf(".");
	  if (do_spinner)
	    {
	      putchar((int) (spinner[rotor++]));
	      putchar((int) '\b');
	      rotor &= 3;
	    }
	  previous_strategy = strategy;
	  previous_jets = physjets;
	  previous_separation = physsep;
	}
      if (!getenv("QUIET") || retval)
	{
	  printf("%d %d %d %d %d %d %d %d %d %d ", physjets, physsep,
		 hpasses, vpasses, subpasses, nrows, first_line,
		 phys_lines, color_jet_arrangement, strategy);
	  if (retval)
	    printf("%d total error%s", retval, retval == 1 ? "" : "s");
	  putc('\n', stdout);
	}

      total_cases++;
      if (retval)
	failures++;
      fflush(stdout);
    }
  printf("%sTotal cases: %d, failures: %d\n",
	 (getenv("QUIET") ? "\n" : ""), total_cases, failures);
  return (failures ? 1 : 0);
}

static int
run_weavetest_from_cmdline(int argc, char **argv)
{
  if (argc != 11)
    {
      fprintf(stderr, "Usage: %s jets separation hpasses vpasses subpasses rows start end arrangement strategy\n",
	      argv[0]);
      return 2;
    }
  else
    {
      int quiet = 0;
      int physjets = atoi(argv[1]);
      int physsep = atoi(argv[2]);
      int hpasses = atoi(argv[3]);
      int vpasses = atoi(argv[4]);
      int subpasses = atoi(argv[5]);
      int nrows = atoi(argv[6]);
      int first_line = atoi(argv[7]);
      int phys_lines = atoi(argv[8]);
      int color_jet_arrangement = atoi(argv[9]);
      int strategy = atoi(argv[10]);
      int status;
      if (getenv("QUIET"))
	quiet = 2;
      status = run_one_weavetest(physjets, physsep, hpasses, vpasses,
				 subpasses, nrows, first_line, phys_lines,
				 color_jet_arrangement, strategy, quiet);
      if (status)
	return 1;
      else
	return 0;
    }
}

int
main(int argc, char **argv)
{
  stp_init();

  if (argc == 1)
    return run_weavetest_from_stdin();
  else
    return run_weavetest_from_cmdline(argc, argv);
}
