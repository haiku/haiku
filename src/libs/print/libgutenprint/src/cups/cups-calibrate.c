/*
 * "$Id: cups-calibrate.c,v 1.6 2007/12/23 17:31:51 easysw Exp $"
 *
 *   Super simple color calibration program for the Common UNIX
 *   Printing System.
 *
 *   Copyright 1993-2000 by Easy Software Products.
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
 * Contents:
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>

/*
 * min/max/abs macros...
 */

#ifndef max
#  define 	max(a,b)	((a) > (b) ? (a) : (b))
#endif /* !max */
#ifndef min
#  define 	min(a,b)	((a) < (b) ? (a) : (b))
#endif /* !min */
#ifndef abs
#  define	abs(a)		((a) < 0 ? -(a) : (a))
#endif /* !abs */


/*
 * Prototypes...
 */

float	get_calibration(const char *prompt, float minval, float maxval);
char	*safe_gets(const char *prompt, char *s, int size);
int	safe_geti(const char *prompt, int defval);
void	send_prolog(FILE *fp);
void	send_pass1(FILE *fp);
void	send_pass2(FILE *fp, float kd, float rd, float yellow);
void	send_pass3(FILE *fp, float kd, float rd, float g, float yellow);
void	send_pass4(FILE *fp, float red, float green, float blue, const char *p);


/*
 * 'main()' - Main entry for calibration program.
 */

int
main(int  argc,
     char *argv[])
{
  char	printer[255];
  char	resolution[255];
  char	mediatype[255];
  char	*profile;
  char	cupsProfile[1024];
  char	command[1024];
  char  lpoptionscommand[1024];
  char  line[255];
  char	junk[255];
  FILE	*fp;
  float	kd, rd, g;
  float	color;
  float	red, green, blue;
  float	cyan, magenta, yellow;
  float	m[3][3];


  puts("ESP Printer Calibration Tool v1.0");
  puts("Copyright 1999-2000 by Easy Software Products, All Rights Reserved.");
  puts("");
  puts("This program allows you to calibrate the color output of printers");
  puts("using the Gutenprint CUPS or ESP Print Pro drivers.");
  puts("");
  puts("Please note that this program ONLY works with the Gutenprint CUPS or");
  puts("ESP Print Pro drivers. If you are using the Gimp-Print stp driver of");
  puts("GhostScript or the drivers of the Print plug-in for the GIMP, this");
  puts("calibration will not work.");
  puts("");
  puts("These drivers by the text \"CUPS+Gutenprint\" or \"ESP Print Pro\" in");
  puts("the model description displayed by the CUPS web interface, KUPS,");
  puts("the ESP Print Pro Printer Manager, or printerdrake.");
  puts("");
  puts("If you are not using the correct driver, press CTRL+C now and");
  puts("reinstall your printer queue with the appropriate driver first.");
  puts("");
  puts("To make a calibration profile for all users, run this program as");
  puts("the \"root\" user.");
  puts("");
  puts("");

  safe_gets("Printer name [default]?", printer, sizeof(printer));
  safe_gets("Resolution [default]?", resolution, sizeof(resolution));
  safe_gets("Media type [default]?", mediatype, sizeof(mediatype));

  strcpy(command, "lp -s");
  if (printer[0])
  {
    strcat(command, " -d ");
    strcat(command, printer);
  }
  if (resolution[0])
  {
    strcat(command, " -o resolution=");
    strcat(command, resolution);
  }
  if (mediatype[0])
  {
    strcat(command, " -o media=");
    strcat(command, mediatype);
  }

  strcat(command, " -o profile=");

  profile = command + strlen(command);

  strcpy(profile, "1000,1000,1000,0,0,0,1000,0,0,0,1000");

  safe_gets("Press ENTER to print pass #1 or N to skip...", junk, sizeof(junk));

  if (!junk[0])
  {
    puts("Sending calibration pass #1 for density/saturation levels...");

    fp = popen(command, "w");
    send_prolog(fp);
    send_pass1(fp);
    fputs("showpage\n", fp);
    pclose(fp);

    puts("Calibration pass #1 sent.");
  }

  puts("");
  puts("Please select the character that corresponds to the black block that");
  puts("is 100% saturated (dark) while not bleeding through the paper.  If");
  puts("the saturation point appears to occur between two characters, enter");
  puts("both characters.");
  puts("");

  kd = get_calibration("Black density?", 0.25f, 1.0f);

  puts("");
  puts("Now select the character that corresponds to the yellow block that is");
  puts("100% saturated (dark) while not bleeding through the paper. If the");
  puts("saturation point appears to occur between two characters, enter both");
  puts("characters.");
  puts("");

  cyan    = kd;
  magenta = kd;
  yellow  = get_calibration("Yellow density?", 0.25f, 1.0f);

  puts("");
  puts("Now select the character that corresponds to the red block that is");
  puts("100% saturated (dark) while not bleeding through the paper. If the");
  puts("saturation point appears to occur between two characters, enter both");
  puts("characters.");
  puts("");

  rd = get_calibration("Red density?", 0.5f, 2.0f);

  puts("");
  puts("Thank you.  Now insert the page back into the printer and press the");
  puts("ENTER key to print calibration pass #2.");
  puts("");

  safe_gets("Press ENTER to print pass #2 or N to skip...", junk, sizeof(junk));

  if (!junk[0])
  {
    puts("Sending calibration pass #2 for gamma levels...");

    fp = popen(command, "w");
    send_prolog(fp);
    send_pass2(fp, kd, rd, yellow);
    fputs("showpage\n", fp);
    pclose(fp);

    puts("Calibration pass #2 sent.");
  }

  puts("");
  puts("Please select the character that corresponds to the column of gray");
  puts("blocks that appear to be 1/2 and 1/4 as dark as the black blocks,");
  puts("respectively.  If the transition point appears to occur between two");
  puts("characters, enter both characters.");
  puts("");

  g = get_calibration("Gamma?", 1.0f, 4.0f);

  puts("");
  puts("Thank you.  Now insert the page back into the printer and press the");
  puts("ENTER key to print calibration pass #3.");
  puts("");

  safe_gets("Press ENTER to print pass #3 or N to skip...", junk, sizeof(junk));

  if (!junk[0])
  {
    puts("Sending calibration pass #3 for red, green, and blue adjustment...");

    fp = popen(command, "w");
    send_prolog(fp);
    send_pass3(fp, kd, rd, g, yellow);
    fputs("showpage\n", fp);
    pclose(fp);

    puts("Calibration pass #3 sent.");
  }

  puts("");
  puts("Please select the character that corresponds to the correct red,");
  puts("green, and blue colors.  If the transition point appears to occur");
  puts("between two characters, enter both characters.");
  puts("");

  red   = get_calibration("Red color?", 0.35f, -0.40f);
  green = get_calibration("Green color?", 0.35f, -0.40f);
  blue  = get_calibration("Blue color?", 0.35f, -0.40f);

  color = 0.5f * rd / kd - kd;

  cyan    /= kd;
  magenta /= kd;
  yellow  /= kd;

  m[0][0] = cyan;			/* C */
  m[0][1] = cyan * (color + blue);	/* C + M (blue) */
  m[0][2] = cyan * (color - green);	/* C + Y (green) */
  m[1][0] = magenta * (color - blue);	/* M + C (blue) */
  m[1][1] = magenta;			/* M */
  m[1][2] = magenta * (color + red);	/* M + Y (red) */
  m[2][0] = yellow * (color + green);	/* Y + C (green) */
  m[2][1] = yellow * (color - red);	/* Y + M (red) */
  m[2][2] = yellow;			/* Y */

  if (m[0][1] > 0.0f)
  {
    m[1][0] -= m[0][1];
    m[0][1] = 0.0f;
  }
  else if (m[1][0] > 0.0f)
  {
    m[0][1] -= m[1][0];
    m[1][0] = 0.0f;
  }

  if (m[0][2] > 0.0f)
  {
    m[2][0] -= m[0][2];
    m[0][2] = 0.0f;
  }
  else if (m[2][0] > 0.0f)
  {
    m[0][2] -= m[2][0];
    m[2][0] = 0.0f;
  }

  if (m[1][2] > 0.0f)
  {
    m[2][1] -= m[1][2];
    m[1][2] = 0.0f;
  }
  else if (m[2][1] > 0.0f)
  {
    m[1][2] -= m[2][1];
    m[2][1] = 0.0f;
  }

  sprintf(profile, "%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f",
          kd * 1000.0f, g * 1000.0f,
	  m[0][0] * 1000.0f, m[0][1] * 1000.0f, m[0][2] * 1000.0f,
	  m[1][0] * 1000.0f, m[1][1] * 1000.0f, m[1][2] * 1000.0f,
	  m[2][0] * 1000.0f, m[2][1] * 1000.0f, m[2][2] * 1000.0f);

  sprintf(cupsProfile, "    *cupsColorProfile %s/%s: \"%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\"\n",
          resolution[0] ? resolution : "-", mediatype[0] ? mediatype : "-",
	  kd, g, m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2],
	  m[2][0], m[2][1], m[2][2]);

  puts("");
  puts("Thank you.  Now insert the page back into the printer and press the");
  puts("ENTER key to print the final calibration pass.");
  puts("");

  safe_gets("Press ENTER to continue...", junk, sizeof(junk));

  puts("Sending calibration pass #4 for visual confirmation...");

  fp = popen(command, "w");
  send_prolog(fp);
  send_pass4(fp, red, green, blue, cupsProfile);
  fputs("showpage\n", fp);
  pclose(fp);

  puts("Calibration pass #4 sent.");

  puts("");
  puts("The basic color profile for these values is:");
  puts("");
  printf("    %s\n", cupsProfile);
  puts("");
  puts("You can add this to the PPD file for this printer to make this change");
  puts("permanent, or use the following option with a printing command:");
  puts("");
  printf("    -o profile=%s\n", profile);
  puts("");
  puts("to use the profile for this job only.");
  puts("");
  puts("Calibration is complete.");
  puts("");

  if (getuid() == 0)
  {
    do
      safe_gets("Would you like to save the profile as a system-wide default (y/n)? ",
                line, sizeof(line));
    while (tolower(line[0]) != 'n' && tolower(line[0]) != 'y');
  }
  else
    line[0] = 'n';

  if (line[0] == 'n')
  {
    do
      safe_gets("Would you like to save the profile as a personal default (y/n)? ",
                line, sizeof(line));
    while (tolower(line[0]) != 'n' && tolower(line[0]) != 'y');
  }

  puts("");
  if (tolower(line[0]) == 'n')
  {
    puts("Calibration profile NOT saved.");
    return (0);
  }

  strcpy(lpoptionscommand, "lpoptions");
  if (printer[0])
  {
    strcat(lpoptionscommand, " -p ");
    strcat(lpoptionscommand, printer);
  }

  strcat(lpoptionscommand, " -o profile=");

  strcat(lpoptionscommand, profile);

  if (system(lpoptionscommand) == 0)
    puts("Calibration profile successfully saved.");
  else
    puts("An error occured while saving the calibration profile.");

  return (0);
}


float
get_calibration(const char  *prompt,
                float       minval,
		float       maxval)
{
  char	line[4];		/* Input from user */
  int	val1, val2;		/* Calibration values */


  do
  {
    if (safe_gets(prompt, line, sizeof(line)) == NULL)
      return (minval);
  }
  while (!isxdigit(line[0]) || (line[1] && !isxdigit(line[1])));

  if (isdigit(line[0]))
    val1 = line[0] - '0';
  else
    val1 = tolower(line[0]) - 'a' + 10;

  if (!line[1])
    val2 = val1;
  else if (isdigit(line[1]))
    val2 = line[1] - '0';
  else
    val2 = tolower(line[1]) - 'a' + 10;

  return (minval + (maxval - minval) * (val1 + val2) / 30.0f);
}


int
safe_geti(const char *prompt,
          int        defval)
{
  char	line[255];


  do
  {
    safe_gets(prompt, line, sizeof(line));

    if (defval && !line[0])
      return (defval);
  }
  while (!line[0] || !isdigit(line[0]));

  return (atoi(line));
}


/*
 * 'safe_gets()' - Get a string from the user.
 */

char *
safe_gets(const char *prompt,
          char       *s,
          int        size)
{
  printf("%s ", prompt);

  if (fgets(s, size, stdin) == NULL)
    return (NULL);

  if (s[0])
    s[strlen(s) - 1] = '\0';

  return (s);
}


void
send_prolog(FILE *fp)
{
  fputs("%!\n", fp);
  fputs("/Helvetica findfont 12 scalefont setfont\n", fp);
  fputs("/BLOCK {\n"
	"	14.4 mul neg 720 add exch\n"
	"	14.4 mul 72 add exch\n"
	"	14.4 14.4 rectfill\n"
	"} bind def\n", fp);
  fputs("/DIAMOND {\n"
	"	14.4 mul neg 720 add 7.2 add exch\n"
	"	14.4 mul 72 add exch\n"
	"	newpath\n"
	"	moveto\n"
	"	7.2 7.2 rlineto\n"
	"	7.2 -7.2 rlineto\n"
	"	-7.2 -7.2 rlineto\n"
	"	closepath\n"
	"	0 ne { fill } { stroke } ifelse\n"
	"} bind def\n", fp);
  fputs("/PLUS {\n"
	"	14.4 mul neg 720 add 7.2 add exch\n"
	"	14.4 mul 72 add exch\n"
	"	newpath\n"
	"	moveto\n"
	"	14.4 0 rlineto\n"
	"	-7.2 -7.2 rmoveto\n"
	"	0 14.4 rlineto\n"
	"	closepath\n"
	"	fill\n"
	"} bind def\n", fp);
  fputs("/TEXT {\n"
        "	14.4 mul neg 720 add 4 add exch\n"
	"	14.4 mul 72 add 7.2 add exch\n"
	"	moveto\n"
	"	dup stringwidth pop 0.5 mul neg 0 rmoveto\n"
	"	show\n"
	"} bind def\n", fp);
}


void
send_pass1(FILE *fp)
{
  int			i;
  float			p;
  static const char	*hex = "FEDCBA9876543210";


  fputs("0 setgray", fp);
  fputs("(K) 0 1 TEXT\n", fp);
  fputs("(Y) 0 2 TEXT\n", fp);
  fputs("(R) 0 4 TEXT\n", fp);

  for (i = 0; i < 16; i ++)
  {
    fprintf(fp, "(%d) %d 0 TEXT\n", 100 - i * 5, i * 2 + 2);
    fprintf(fp, "(%c) %d 3 TEXT\n", hex[i], i * 2 + 2);
    fprintf(fp, "(%d) %d 5 TEXT\n", 200 - i * 10, i * 2 + 2);
  }

  for (i = 0; i < 16; i ++)
  {
    p = 0.01f * (100 - i * 5);

    fprintf(fp, "%.2f setgray %d 1 BLOCK\n", 1.0 - p, i * 2 + 2);
    fprintf(fp, "0 0 %.2f 0 setcmykcolor %d 2 BLOCK\n", p, i * 2 + 2);
    fprintf(fp, "0 %.2f %.2f 0 setcmykcolor %d 4 BLOCK\n", p, p, i * 2 + 2);
  }
}


void
send_pass2(FILE  *fp,
	   float kd,
	   float rd,
	   float yellow)
{
  int			i;
  float			p;
  float			g;
  static const char	*hex = "FEDCBA9876543210";


  rd *= 0.5f;

  fprintf(fp, "0 0 %.2f 0 setcmykcolor\n", yellow);
  fprintf(fp, "1 %.2f 6 DIAMOND\n", 2.0f + 30.0f * (1.0f - yellow) / 0.75f);

  fprintf(fp, "0 %.2f %.2f 0 setcmykcolor\n", rd, rd);
  fprintf(fp, "%d %.2f 6 DIAMOND\n", rd != yellow,
          2.0f + 30.0f * (1.0f - rd) / 0.75f);

  p = 1.0f - kd;
  fprintf(fp, "%.2f setgray\n", p);

  if (kd == rd && kd == yellow)
    fprintf(fp, "%.2f 6 PLUS\n", 2.0f + 30.0f * (1.0f - kd) / 0.75f);
  else
    fprintf(fp, "%d %.2f 6 DIAMOND\n", kd != yellow && kd != rd,
            2.0f + 30.0f * (1.0f - kd) / 0.75f);

  fputs("(100%) 0 9 TEXT\n", fp);
  fputs("(50%) 0 10 TEXT\n", fp);
  fputs("(25%) 0 11 TEXT\n", fp);

  for (i = 0; i < 16; i ++)
  {
    fprintf(fp, "(%.1f) %d 8 TEXT\n", 0.2f * (20 - i), i * 2 + 2);
    fprintf(fp, "(%c) %d 12 TEXT\n", hex[i], i * 2 + 2);
  }

  for (i = 0; i < 16; i ++)
  {
    g = 0.2f * (20 - i);

    p = 1.0f - kd * (float)pow(1.0f, g);
    fprintf(fp, "%.2f setgray %d 9 BLOCK\n", p, i * 2 + 2);

    p = 1.0f - kd * (float)pow(0.5f, g);
    fprintf(fp, "%.2f setgray %d 10 BLOCK\n", p, i * 2 + 2);

    p = 1.0f - kd * (float)pow(0.25f, g);
    fprintf(fp, "%.2f setgray %d 11 BLOCK\n", p, i * 2 + 2);
  }
}


void
send_pass3(FILE  *fp,
	   float kd,
	   float rd,
	   float g,
	   float yellow)
{
  int	i;
  float p;
  float	color;
  float	c, m, y;
  float	adj;
  static const char	*hex = "FEDCBA9876543210";


  p = 1.0f - kd;
  fprintf(fp, "%.2f setgray\n", p);
  fprintf(fp, "1 %.2f 13 DIAMOND\n", 2.0f + 30.0f * (4.0f - g) / 3.0f);

  color  = 0.5f * rd / kd - kd;
  yellow /= kd;

  fputs("(R) 2 16 TEXT\n", fp);
  fputs("(G) 2 17 TEXT\n", fp);
  fputs("(B) 2 18 TEXT\n", fp);

  for (i = 1; i < 16; i ++)
  {
    fprintf(fp, "(%+d) %d 15 TEXT\n", i * 5 - 40, i * 2 + 2);
    fprintf(fp, "(%c) %d 19 TEXT\n", hex[i], i * 2 + 2);
  }

  for (i = 1; i < 16; i ++)
  {
   /* Adjustment value */
    adj = i * 0.05f - 0.40f;

   /* RED */
    c = 0.0f;
    m = color + adj;
    y = color - adj;
    if (m > 0)
    {
      y -= m;
      m = 0;
    }
    else if (y > 0)
    {
      m -= y;
      y = 0;
    }

    m += 1.0f;
    y = yellow * (1.0f + y);

    if (c <= 0.0f)
      c = 0.0f;
    else if (c >= 1.0f)
      c = 1.0f;
    else
      c = (float)pow(c, g);

    if (m <= 0.0f)
      m = 0.0f;
    else if (m >= 1.0f)
      m = 1.0f;
    else
      m = (float)pow(m, g);

    if (y <= 0.0f)
      y = 0.0f;
    else if (y >= 1.0f)
      y = 1.0f;
    else
      y = (float)pow(y, g);

    fprintf(fp, "%.2f %.2f %.2f 0 setcmykcolor %d 16 BLOCK\n",
            c * kd, m * kd, y * kd, i * 2 + 2);

   /* GREEN */
    c = color - adj;
    m = 0.0f;
    y = color + adj;

    if (c > 0)
    {
      y -= c;
      c = 0;
    }
    else if (y > 0)
    {
      c -= y;
      y = 0;
    }

    c += 1.0f;
    y = yellow * (1.0f + y);

    if (c <= 0.0f)
      c = 0.0f;
    else if (c >= 1.0f)
      c = 1.0f;
    else
      c = (float)pow(c, g);

    if (m <= 0.0f)
      m = 0.0f;
    else if (m >= 1.0f)
      m = 1.0f;
    else
      m = (float)pow(m, g);

    if (y <= 0.0f)
      y = 0.0f;
    else if (y >= 1.0f)
      y = 1.0f;
    else
      y = (float)pow(y, g);

    fprintf(fp, "%.2f %.2f %.2f 0 setcmykcolor %d 17 BLOCK\n",
            c * kd, m * kd, y * kd, i * 2 + 2);

   /* BLUE */
    c = color + adj;
    m = color - adj;
    y = 0.0f;

    if (c > 0)
    {
      m -= c;
      c = 0;
    }
    else if (m > 0)
    {
      c -= m;
      m = 0;
    }

    c += 1.0f;
    m += 1.0f;

    if (c <= 0.0f)
      c = 0.0f;
    else if (c >= 1.0f)
      c = 1.0f;
    else
      c = (float)pow(c, g);

    if (m <= 0.0f)
      m = 0.0f;
    else if (m >= 1.0f)
      m = 1.0f;
    else
      m = (float)pow(m, g);

    if (y <= 0.0f)
      y = 0.0f;
    else if (y >= 1.0f)
      y = 1.0f;
    else
      y = (float)pow(y, g);

    fprintf(fp, "%.2f %.2f %.2f 0 setcmykcolor %d 18 BLOCK\n",
            c * kd, m * kd, y * kd, i * 2 + 2);
  }
}


void
send_pass4(FILE       *fp,
	   float      red,
	   float      green,
	   float      blue,
	   const char *profile)
{
  FILE	*ppm;
  int	x, y, col, width, height;
  int	r, g, b;
  char	line[255];


  fprintf(fp, "0 0 1 setrgbcolor 1 %.2f 20 DIAMOND\n", /* BLUE */
          2.0f + 30.0f * (0.40f + blue) / 0.75f);
  fprintf(fp, "1 0 0 setrgbcolor %d %.2f 20 DIAMOND\n", /* RED */
          red != blue, 2.0f + 30.0f * (0.40f + red) / 0.75f);
  if (green == red && green == blue)
    fprintf(fp, "0 1 0 setrgbcolor %.2f 20 PLUS\n", /* GREEN */
            2.0f + 30.0f * (0.40f + green) / 0.75f);
  else
    fprintf(fp, "0 1 0 setrgbcolor %d %.2f 20 DIAMOND\n", /* GREEN */
            green != red && green != blue,
	    2.0f + 30.0f * (0.40f + green) / 0.75f);

  fputs("0 setgray\n", fp);
  fputs("currentfont 0.8 scalefont setfont\n", fp);

  fprintf(fp, "(%s) 16 22 TEXT\n", profile);

  if ((ppm = fopen(CUPS_DATADIR "/calibrate.ppm", "rb")) == NULL)
    if ((ppm = fopen("calibrate.ppm", "rb")) == NULL)
      return;

  fgets(line, sizeof(line), ppm);
  while (fgets(line, sizeof(line), ppm))
    if (line[0] != '#')
      break;

  sscanf(line, "%d%d", &width, &height);

  fgets(line, sizeof(line), ppm);

  fputs("gsave\n", fp);
  fprintf(fp, "72 %d translate\n", 504 * height / width + 72);
  fprintf(fp, "504 -%d scale\n", 504 * height / width);

  fprintf(fp, "/scanline %d string def\n", width * 3);

  fprintf(fp, "%d %d 8\n", width, height);
  fprintf(fp, "[%d 0 0 %d 0 0]\n", width, height);
  fputs("{ currentfile scanline readhexstring pop } false 3 colorimage\n", fp);

  for (y = 0, col = 0; y < height; y ++)
  {
    printf("Sending scanline %d of %d...\r", y + 1, height);
    fflush(stdout);

    for (x = 0; x < width; x ++)
    {
      r = getc(ppm);
      g = getc(ppm);
      b = getc(ppm);

      fprintf(fp, "%02X%02X%02X", r, g, b);
      col ++;
      if (col >= 12)
      {
        col = 0;
	putc('\n', fp);
      }
    }
  }

  if (col)
    putc('\n', fp);

  printf("                                    \r");

  fclose(ppm);

  fputs("grestore\n", fp);
}
