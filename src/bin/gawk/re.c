/*
 * re.c - compile regular expressions.
 */

/* 
 * Copyright (C) 1991-2003 the Free Software Foundation, Inc.
 * 
 * This file is part of GAWK, the GNU implementation of the
 * AWK Programming Language.
 * 
 * GAWK is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * GAWK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#include "awk.h"

static reg_syntax_t syn;

/* make_regexp --- generate compiled regular expressions */

Regexp *
make_regexp(const char *s, size_t len, int ignorecase)
{
	Regexp *rp;
	const char *rerr;
	const char *src = s;
	char *temp;
	const char *end = s + len;
	register char *dest;
	register int c, c2;
#ifdef MBS_SUPPORT
	/* The number of bytes in the current multbyte character.
	   It is 0, when the current character is a singlebyte character.  */
	size_t is_multibyte = 0;
	mbstate_t mbs;

	if (gawk_mb_cur_max > 1)
		memset(&mbs, 0, sizeof(mbstate_t)); /* Initialize.  */
#endif

	/* Handle escaped characters first. */

	/*
	 * Build a copy of the string (in dest) with the
	 * escaped characters translated, and generate the regex
	 * from that.  
	 */
	emalloc(dest, char *, len + 2, "make_regexp");
	temp = dest;

	while (src < end) {
#ifdef MBS_SUPPORT
		if (gawk_mb_cur_max > 1 && !is_multibyte) {
			/* The previous byte is a singlebyte character, or last byte
			   of a multibyte character.  We check the next character.  */
			is_multibyte = mbrlen(src, end - src, &mbs);
			if ((is_multibyte == 1) || (is_multibyte == (size_t) -1)
				|| (is_multibyte == (size_t) -2 || (is_multibyte == 0))) {
				/* We treat it as a singlebyte character.  */
				is_multibyte = 0;
			}
		}
#endif

		if (
#ifdef MBS_SUPPORT
		/* We skip multibyte character, since it must not be a special
		   character.  */
		    (gawk_mb_cur_max == 1 || ! is_multibyte) &&
#endif
		    (*src == '\\')) {
			c = *++src;
			switch (c) {
			case 'a':
			case 'b':
			case 'f':
			case 'n':
			case 'r':
			case 't':
			case 'v':
			case 'x':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				c2 = parse_escape(&src);
				if (c2 < 0)
					cant_happen();
				/*
				 * Unix awk treats octal (and hex?) chars
				 * literally in re's, so escape regexp
				 * metacharacters.
				 */
				if (do_traditional && ! do_posix && (ISDIGIT(c) || c == 'x')
				    && strchr("()|*+?.^$\\[]", c2) != NULL)
					*dest++ = '\\';
				*dest++ = (char) c2;
				break;
			case '8':
			case '9':	/* a\9b not valid */
				*dest++ = c;
				src++;
				break;
			case 'y':	/* normally \b */
				/* gnu regex op */
				if (! do_traditional) {
					*dest++ = '\\';
					*dest++ = 'b';
					src++;
					break;
				}
				/* else, fall through */
			default:
				*dest++ = '\\';
				*dest++ = (char) c;
				src++;
				break;
			} /* switch */
		} else
			*dest++ = *src++;	/* not '\\' */
#ifdef MBS_SUPPORT
		if (gawk_mb_cur_max > 1 && is_multibyte)
			is_multibyte--;
#endif
	} /* while */

	*dest = '\0' ;	/* Only necessary if we print dest ? */
	emalloc(rp, Regexp *, sizeof(*rp), "make_regexp");
	memset((char *) rp, 0, sizeof(*rp));
	rp->pat.allocated = 0;	/* regex will allocate the buffer */
	emalloc(rp->pat.fastmap, char *, 256, "make_regexp");

	if (ignorecase)
		rp->pat.translate = casetable;
	else
		rp->pat.translate = NULL;
	len = dest - temp;
	if ((rerr = re_compile_pattern(temp, len, &(rp->pat))) != NULL)
		fatal("%s: /%s/", rerr, temp);	/* rerr already gettextized inside regex routines */

	/* gack. this must be done *after* re_compile_pattern */
	rp->pat.newline_anchor = FALSE; /* don't get \n in middle of string */

	free(temp);
	return rp;
}

/* research --- do a regexp search */

int
research(Regexp *rp, register const char *str, int start,
	register size_t len, int need_start)
{
	const char *ret = str;

	if (ret) {
		/*
		 * Passing NULL as last arg speeds up search for cases
		 * where we don't need the start/end info.
		 */
		int res = re_search(&(rp->pat), str, start+len,
				start, len, need_start ? &(rp->regs) : NULL);

		/*
		 * A return of -2 indicates that a heuristic in
		 * regex decided it might allocate too much memory
		 * on the C stack. This doesn't apply to gawk, which
		 * uses REGEX_MALLOC. This is dealt with by the
		 * assignment to re_max_failures in resetup().
		 * Naetheless, we keep this code here as a fallback.
		 *
		 * XXX: The above comment is obsolete; the new regex
		 * doesn't have an re_max_failures variable. But we
		 * keep the code here just in case.
		 */
		if (res == -2) {
			/* the 10 here is arbitrary */
			fatal(_("regex match failed, not enough memory to match string \"%.*s%s\""),
					(int) (len > 10 ? 10 : len), str + start,
					len > 10 ? "..." : "");
		}
		return res;
	} else
		return -1;
}

/* refree --- free up the dynamic memory used by a compiled regexp */

void
refree(Regexp *rp)
{
	/*
	 * This isn't malloced, don't let regfree free it.
	 * (This is strictly necessary only for the old
	 * version of regex, but it's a good idea to keep it
	 * here in case regex internals change in the future.)
	 */
	rp->pat.translate = NULL;

	regfree(& rp->pat);
	if (rp->regs.start)
		free(rp->regs.start);
	if (rp->regs.end)
		free(rp->regs.end);
	free(rp);
}

/* re_update --- recompile a dynamic regexp */

Regexp *
re_update(NODE *t)
{
	NODE *t1;

	if ((t->re_flags & CASE) == IGNORECASE) {
		if ((t->re_flags & CONST) != 0) {
			assert(t->type == Node_regex);
			return t->re_reg;
		}
		t1 = force_string(tree_eval(t->re_exp));
		if (t->re_text != NULL) {
			if (cmp_nodes(t->re_text, t1) == 0) {
				free_temp(t1);
				return t->re_reg;
			}
			unref(t->re_text);
		}
		t->re_text = dupnode(t1);
		free_temp(t1);
	}
	if (t->re_reg != NULL)
		refree(t->re_reg);
	if (t->re_text == NULL || (t->re_flags & CASE) != IGNORECASE) {
		t1 = force_string(tree_eval(t->re_exp));
		unref(t->re_text);
		t->re_text = dupnode(t1);
		free_temp(t1);
	}
	t->re_reg = make_regexp(t->re_text->stptr, t->re_text->stlen,
				IGNORECASE);
	t->re_flags &= ~CASE;
	t->re_flags |= IGNORECASE;
	return t->re_reg;
}

/* resetup --- choose what kind of regexps we match */

void
resetup()
{
	if (do_posix)
		syn = RE_SYNTAX_POSIX_AWK;	/* strict POSIX re's */
	else if (do_traditional)
		syn = RE_SYNTAX_AWK;		/* traditional Unix awk re's */
	else
		syn = RE_SYNTAX_GNU_AWK;	/* POSIX re's + GNU ops */

	/*
	 * Interval expressions are off by default, since it's likely to
	 * break too many old programs to have them on.
	 */
	if (do_intervals)
		syn |= RE_INTERVALS;

	(void) re_set_syntax(syn);
}

/* reisstring --- return TRUE if the RE match is a simple string match */

int
reisstring(const char *text, size_t len, Regexp *re, const char *buf)
{
	static char metas[] = ".*+(){}[]|?^$\\";
	int i;
	int res;
	const char *matched;

	/* simple checking for has meta characters in re */
	for (i = 0; i < len; i++) {
		if (strchr(metas, text[i]) != NULL) {
			return FALSE;	/* give up early, can't be string match */
		}
	}

	/* make accessable to gdb */
	matched = &buf[RESTART(re, buf)];

	res = STREQN(text, matched, len);

	return res;
}

/* remaybelong --- return TRUE if the RE contains * ? | + */

int
remaybelong(const char *text, size_t len)
{
	while (len--) {
		if (strchr("*+|?", *text++) != NULL) {
			return TRUE;
		}
	}

	return FALSE;
}

/* reflags2str --- make a regex flags value readable */

const char *
reflags2str(int flagval)
{
	static const struct flagtab values[] = {
		{ RE_BACKSLASH_ESCAPE_IN_LISTS, "RE_BACKSLASH_ESCAPE_IN_LISTS" },
		{ RE_BK_PLUS_QM, "RE_BK_PLUS_QM" },
		{ RE_CHAR_CLASSES, "RE_CHAR_CLASSES" },
		{ RE_CONTEXT_INDEP_ANCHORS, "RE_CONTEXT_INDEP_ANCHORS" },
		{ RE_CONTEXT_INDEP_OPS, "RE_CONTEXT_INDEP_OPS" },
		{ RE_CONTEXT_INVALID_OPS, "RE_CONTEXT_INVALID_OPS" },
		{ RE_DOT_NEWLINE, "RE_DOT_NEWLINE" },
		{ RE_DOT_NOT_NULL, "RE_DOT_NOT_NULL" },
		{ RE_HAT_LISTS_NOT_NEWLINE, "RE_HAT_LISTS_NOT_NEWLINE" },
		{ RE_INTERVALS, "RE_INTERVALS" },
		{ RE_LIMITED_OPS, "RE_LIMITED_OPS" },
		{ RE_NEWLINE_ALT, "RE_NEWLINE_ALT" },
		{ RE_NO_BK_BRACES, "RE_NO_BK_BRACES" },
		{ RE_NO_BK_PARENS, "RE_NO_BK_PARENS" },
		{ RE_NO_BK_REFS, "RE_NO_BK_REFS" },
		{ RE_NO_BK_VBAR, "RE_NO_BK_VBAR" },
		{ RE_NO_EMPTY_RANGES, "RE_NO_EMPTY_RANGES" },
		{ RE_UNMATCHED_RIGHT_PAREN_ORD, "RE_UNMATCHED_RIGHT_PAREN_ORD" },
		{ RE_NO_POSIX_BACKTRACKING, "RE_NO_POSIX_BACKTRACKING" },
		{ RE_NO_GNU_OPS, "RE_NO_GNU_OPS" },
		{ RE_DEBUG, "RE_DEBUG" },
		{ RE_INVALID_INTERVAL_ORD, "RE_INVALID_INTERVAL_ORD" },
		{ RE_ICASE, "RE_ICASE" },
		{ 0,	NULL },
	};

	return genflags2str(flagval, values);
}
