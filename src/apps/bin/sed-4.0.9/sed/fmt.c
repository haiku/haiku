/* `L' command implementation for GNU sed, based on GNU fmt 1.22.
   Copyright (C) 1994, 1995, 1996, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* GNU fmt was written by Ross Paterson <rap@doc.ic.ac.uk>.  */

#include "sed.h"

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#if HAVE_LIMITS_H
# include <limits.h>
#endif

#ifndef UINT_MAX
# define UINT_MAX ((unsigned int) ~(unsigned int) 0)
#endif

#ifndef INT_MAX
# define INT_MAX ((int) (UINT_MAX >> 1))
#endif

/* The following parameters represent the program's idea of what is
   "best".  Adjust to taste, subject to the caveats given.  */

/* Prefer lines to be LEEWAY % shorter than the maximum width, giving
   room for optimization.  */
#define	LEEWAY	7

/* Costs and bonuses are expressed as the equivalent departure from the
   optimal line length, multiplied by 10.  e.g. assigning something a
   cost of 50 means that it is as bad as a line 5 characters too short
   or too long.  The definition of SHORT_COST(n) should not be changed.
   However, EQUIV(n) may need tuning.  */

typedef long COST;

#define	MAXCOST	(~(((unsigned long) 1) << (8 * sizeof (COST) -1)))

#define	SQR(n)		((n) * (n))
#define	EQUIV(n)	SQR ((COST) (n))

/* Cost of a filled line n chars longer or shorter than best_width.  */
#define	SHORT_COST(n)	EQUIV ((n) * 10)

/* Cost of the difference between adjacent filled lines.  */
#define	RAGGED_COST(n)	(SHORT_COST (n) / 2)

/* Basic cost per line.  */
#define	LINE_COST	EQUIV (70)

/* Cost of breaking a line after the first word of a sentence, where
   the length of the word is N.  */
#define	WIDOW_COST(n)	(EQUIV (200) / ((n) + 2))

/* Cost of breaking a line before the last word of a sentence, where
   the length of the word is N.  */
#define	ORPHAN_COST(n)	(EQUIV (150) / ((n) + 2))

/* Bonus for breaking a line at the end of a sentence.  */
#define	SENTENCE_BONUS	EQUIV (50)

/* Cost of breaking a line after a period not marking end of a sentence.
   With the definition of sentence we are using (borrowed from emacs, see
   get_line()) such a break would then look like a sentence break.  Hence
   we assign a very high cost -- it should be avoided unless things are
   really bad.  */
#define	NOBREAK_COST	EQUIV (600)

/* Bonus for breaking a line before open parenthesis.  */
#define	PAREN_BONUS	EQUIV (40)

/* Bonus for breaking a line after other punctuation.  */
#define	PUNCT_BONUS	EQUIV(40)

/* Credit for breaking a long paragraph one line later.  */
#define	LINE_CREDIT	EQUIV(3)

/* Size of paragraph buffer in words.  Longer paragraphs are handled
   neatly (cf. flush_paragraph()), so there's little to gain by making
   these larger.  */
#define	MAXWORDS	1000

#define GETC()          (parabuf == end_of_parabuf ? EOF : *parabuf++)

/* Extra ctype(3)-style macros.  */

#define	isopen(c)	(strchr ("([`'\"", (c)) != NULL)
#define	isclose(c)	(strchr (")]'\"", (c)) != NULL)
#define	isperiod(c)	(strchr (".?!", (c)) != NULL)

/* Size of a tab stop, for expansion on input and re-introduction on
   output.  */
#define	TABWIDTH	8

/* Word descriptor structure.  */

typedef struct Word WORD;

struct Word
  {

    /* Static attributes determined during input.  */

    const char *text;		/* the text of the word */
    short length;		/* length of this word */
    short space;		/* the size of the following space */
    unsigned paren:1;		/* starts with open paren */
    unsigned period:1;		/* ends in [.?!])* */
    unsigned punct:1;		/* ends in punctuation */
    unsigned final:1;		/* end of sentence */

    /* The remaining fields are computed during the optimization.  */

    short line_length;		/* length of the best line starting here */
    COST best_cost;		/* cost of best paragraph starting here */
    WORD *next_break;		/* break which achieves best_cost */
  };

/* Forward declarations.  */

extern void fmt P_ ((char *line, char *line_end, int max_length, FILE *output_file));
static flagT get_paragraph P_ ((void));
static int get_line P_ ((int c));
static int get_space P_ ((int c));
static int copy_rest P_ ((int c));
static flagT same_para P_ ((int c));
static void flush_paragraph P_ ((void));
static void fmt_paragraph P_ ((void));
static void check_punctuation P_ ((WORD *w));
static COST base_cost P_ ((WORD *this));
static COST line_cost P_ ((WORD *next, int len));
static void put_paragraph P_ ((WORD *finish));
static void put_line P_ ((WORD *w, int indent));
static void put_word P_ ((WORD *w));
static void put_space P_ ((int space));

/* Option values.  */

/* User-supplied maximum line width (default WIDTH).  The only output
   lines
   longer than this will each comprise a single word.  */
static int max_width;

/* Space for the paragraph text.  */
static char *parabuf;

/* End of space for the paragraph text.  */
static char *end_of_parabuf;

/* The file on which we output */
static FILE *outfile;

/* Values derived from the option values.  */

/* The preferred width of text lines, set to LEEWAY % less than max_width.  */
static int best_width;

/* Dynamic variables.  */

/* Start column of the character most recently read from the input file.  */
static int in_column;

/* Start column of the next character to be written to stdout.  */
static int out_column;

/* The words of a paragraph -- longer paragraphs are handled neatly
   (cf. flush_paragraph()).  */
static WORD words[MAXWORDS];

/* A pointer into the above word array, indicating the first position
   after the last complete word.  Sometimes it will point at an incomplete
   word.  */
static WORD *word_limit;

/* Indentation of the first line of the current paragraph.  */
static int first_indent;

/* Indentation of other lines of the current paragraph */
static int other_indent;

/* The last character read from the input file.  */
static int next_char;

/* If nonzero, the length of the last line output in the current
   paragraph, used to charge for raggedness at the split point for long
   paragraphs chosen by fmt_paragraph().  */
static int last_line_length;

/* read file F and send formatted output to stdout.  */

void
fmt (char *line, char *line_end, int max_length, FILE *output_file)
{
  parabuf = line;
  end_of_parabuf = line_end;
  outfile = output_file;

  max_width = max_length;
  best_width = max_width * (201 - 2 * LEEWAY) / 200;

  in_column = 0;
  other_indent = 0;
  next_char = GETC();
  while (get_paragraph ())
    {
      fmt_paragraph ();
      put_paragraph (word_limit);
    }
}

/* Read a paragraph from input file F.  A paragraph consists of a
   maximal number of non-blank (excluding any prefix) lines
   with the same indent.

   Return FALSE if end-of-file was encountered before the start of a
   paragraph, else TRUE.  */

static flagT
get_paragraph ()
{
  register int c;

  last_line_length = 0;
  c = next_char;

  /* Scan (and copy) blank lines, and lines not introduced by the prefix.  */

  while (c == '\n' || c == EOF)
    {
      c = copy_rest (c);
      if (c == EOF)
	{
	  next_char = EOF;
	  return FALSE;
	}
      putc ('\n', outfile);
      c = GETC();
    }

  /* Got a suitable first line for a paragraph.  */

  first_indent = in_column;
  word_limit = words;
  c = get_line (c);

  /* Read rest of paragraph.  */

  other_indent = in_column;
  while (same_para (c) && in_column == other_indent)
    c = get_line (c);

  (word_limit - 1)->period = (word_limit - 1)->final = TRUE;
  next_char = c;
  return TRUE;
}

/* Copy to the output a blank line.  In the latter, C is \n or EOF.
   Return the character (\n or EOF) ending the line.  */

static int
copy_rest (register int c)
{
  out_column = 0;
  while (c != '\n' && c != EOF)
    {
      putc (c, outfile);
      c = GETC();
    }
  return c;
}

/* Return TRUE if a line whose first non-blank character after the
   prefix (if any) is C could belong to the current paragraph,
   otherwise FALSE.  */

static flagT
same_para (register int c)
{
  return (c != '\n' && c != EOF);
}

/* Read a line from the input data given first non-blank character C
   after the prefix, and the following indent, and break it into words.
   A word is a maximal non-empty string of non-white characters.  A word
   ending in [.?!]["')\]]* and followed by end-of-line or at least two
   spaces ends a sentence, as in emacs.

   Return the first non-blank character of the next line.  */

static int
get_line (register int c)
{
  int start;
  register WORD *end_of_word;

  end_of_word = &words[MAXWORDS - 2];

  do
    {				/* for each word in a line */

      /* Scan word.  */

      word_limit->text = parabuf - 1;
      do
	c = GETC();
      while (c != EOF && !ISSPACE (c));
      word_limit->length = parabuf - word_limit->text - (c != EOF);
      in_column += word_limit->length;

      check_punctuation (word_limit);

      /* Scan inter-word space.  */

      start = in_column;
      c = get_space (c);
      word_limit->space = in_column - start;
      word_limit->final = (c == EOF
			   || (word_limit->period
			       && (c == '\n' || word_limit->space > 1)));
      if (c == '\n' || c == EOF)
	word_limit->space = word_limit->final ? 2 : 1;
      if (word_limit == end_of_word)
	flush_paragraph ();
      word_limit++;
      if (c == EOF)
	{
	  in_column = first_indent;
	  return EOF;
	}
    }
  while (c != '\n');

  in_column = 0;
  c = GETC();
  return get_space (c);
}

/* Read blank characters from the input data, starting with C, and keeping
   in_column up-to-date.  Return first non-blank character.  */

static int
get_space (register int c)
{
  for (;;)
    {
      if (c == ' ')
	in_column++;
      else if (c == '\t')
	in_column = (in_column / TABWIDTH + 1) * TABWIDTH;
      else
	return c;
      c = GETC();
    }
}

/* Set extra fields in word W describing any attached punctuation.  */

static void
check_punctuation (register WORD *w)
{
  register const char *start, *finish;

  start = w->text;
  finish = start + (w->length - 1);
  w->paren = isopen (*start);
  w->punct = ISPUNCT (*finish);
  while (isclose (*finish) && finish > start)
    finish--;
  w->period = isperiod (*finish);
}

/* Flush part of the paragraph to make room.  This function is called on
   hitting the limit on the number of words or characters.  */

static void
flush_paragraph (void)
{
  WORD *split_point;
  register WORD *w;
  COST best_break;

  /* - format what you have so far as a paragraph,
     - find a low-cost line break near the end,
     - output to there,
     - make that the start of the paragraph.  */

  fmt_paragraph ();

  /* Choose a good split point.  */

  split_point = word_limit;
  best_break = MAXCOST;
  for (w = words->next_break; w != word_limit; w = w->next_break)
    {
      if (w->best_cost - w->next_break->best_cost < best_break)
	{
	  split_point = w;
	  best_break = w->best_cost - w->next_break->best_cost;
	}
      if (best_break <= MAXCOST - LINE_CREDIT)
	best_break += LINE_CREDIT;
    }
  put_paragraph (split_point);

  /* Copy words from split_point down to word -- we use memmove because
     the source and target may overlap.  */

  memmove ((char *) words, (char *) split_point,
	 (word_limit - split_point + 1) * sizeof (WORD));
  word_limit -= split_point - words;
}

/* Compute the optimal formatting for the whole paragraph by computing
   and remembering the optimal formatting for each suffix from the empty
   one to the whole paragraph.  */

static void
fmt_paragraph (void)
{
  register WORD *start, *w;
  register int len;
  register COST wcost, best;
  int saved_length;

  word_limit->best_cost = 0;
  saved_length = word_limit->length;
  word_limit->length = max_width;	/* sentinel */

  for (start = word_limit - 1; start >= words; start--)
    {
      best = MAXCOST;
      len = start == words ? first_indent : other_indent;

      /* At least one word, however long, in the line.  */

      w = start;
      len += w->length;
      do
	{
	  w++;

	  /* Consider breaking before w.  */

	  wcost = line_cost (w, len) + w->best_cost;
	  if (start == words && last_line_length > 0)
	    wcost += RAGGED_COST (len - last_line_length);
	  if (wcost < best)
	    {
	      best = wcost;
	      start->next_break = w;
	      start->line_length = len;
	    }
	  len += (w - 1)->space + w->length;	/* w > start >= words */
	}
      while (len < max_width);
      start->best_cost = best + base_cost (start);
    }

  word_limit->length = saved_length;
}

/* Return the constant component of the cost of breaking before the
   word THIS.  */

static COST
base_cost (register WORD *this)
{
  register COST cost;

  cost = LINE_COST;

  if (this > words)
    {
      if ((this - 1)->period)
	{
	  if ((this - 1)->final)
	    cost -= SENTENCE_BONUS;
	  else
	    cost += NOBREAK_COST;
	}
      else if ((this - 1)->punct)
	cost -= PUNCT_BONUS;
      else if (this > words + 1 && (this - 2)->final)
	cost += WIDOW_COST ((this - 1)->length);
    }

  if (this->paren)
    cost -= PAREN_BONUS;
  else if (this->final)
    cost += ORPHAN_COST (this->length);

  return cost;
}

/* Return the component of the cost of breaking before word NEXT that
   depends on LEN, the length of the line beginning there.  */

static COST
line_cost (register WORD *next, register int len)
{
  register int n;
  register COST cost;

  if (next == word_limit)
    return 0;
  n = best_width - len;
  cost = SHORT_COST (n);
  if (next->next_break != word_limit)
    {
      n = len - next->line_length;
      cost += RAGGED_COST (n);
    }
  return cost;
}

/* Output to stdout a paragraph from word up to (but not including)
   FINISH, which must be in the next_break chain from word.  */

static void
put_paragraph (register WORD *finish)
{
  register WORD *w;

  put_line (words, first_indent);
  for (w = words->next_break; w != finish; w = w->next_break)
    put_line (w, other_indent);
}

/* Output to stdout the line beginning with word W, beginning in column
   INDENT, including the prefix (if any).  */

static void
put_line (register WORD *w, int indent)
{
  register WORD *endline;
  out_column = 0;
  put_space (indent);

  endline = w->next_break - 1;
  for (; w != endline; w++)
    {
      put_word (w);
      put_space (w->space);
    }
  put_word (w);
  last_line_length = out_column;
  putc ('\n', outfile);
}

/* Output to stdout the word W.  */

static void
put_word (register WORD *w)
{
  register const char *s;
  register int n;

  s = w->text;
  for (n = w->length; n != 0; n--)
    putc (*s++, outfile);
  out_column += w->length;
}

/* Output to stdout SPACE spaces, or equivalent tabs.  */

static void
put_space (int space)
{
  out_column += space;
  while (space--)
    putc (' ', outfile);
}
