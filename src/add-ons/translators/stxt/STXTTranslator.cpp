/*
 * Copyright 2002-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "STXTTranslator.h"
#include "STXTView.h"

#include <Catalog.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <MimeType.h>
#include <String.h>
#include <UTF8.h>

#include <algorithm>
#include <new>
#include <string.h>
#include <stdio.h>
#include <stdint.h>


using namespace BPrivate;
using namespace std;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "STXTTranslator"

#define READ_BUFFER_SIZE 32768
#define DATA_BUFFER_SIZE 256

// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
	{
		B_TRANSLATOR_TEXT,
		B_TRANSLATOR_TEXT,
		TEXT_IN_QUALITY,
		TEXT_IN_CAPABILITY,
		"text/plain",
		"Plain text file"
	},
	{
		B_STYLED_TEXT_FORMAT,
		B_TRANSLATOR_TEXT,
		STXT_IN_QUALITY,
		STXT_IN_CAPABILITY,
		"text/x-vnd.Be-stxt",
		"Be styled text file"
	}
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
	{
		B_TRANSLATOR_TEXT,
		B_TRANSLATOR_TEXT,
		TEXT_OUT_QUALITY,
		TEXT_OUT_CAPABILITY,
		"text/plain",
		"Plain text file"
	},
	{
		B_STYLED_TEXT_FORMAT,
		B_TRANSLATOR_TEXT,
		STXT_OUT_QUALITY,
		STXT_OUT_CAPABILITY,
		"text/x-vnd.Be-stxt",
		"Be styled text file"
	}
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
	{B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false},
	{B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false}
};

const uint32 kNumInputFormats = sizeof(sInputFormats) / sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) / sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) / sizeof(TranSetting);

// ---------------------------------------------------------------
// make_nth_translator
//
// Creates a STXTTranslator object to be used by BTranslatorRoster
//
// Preconditions:
//
// Parameters: n,		The translator to return. Since
//						STXTTranslator only publishes one
//						translator, it only returns a
//						STXTTranslator if n == 0
//
//             you, 	The image_id of the add-on that
//						contains code (not used).
//
//             flags,	Has no meaning yet, should be 0.
//
// Postconditions:
//
// Returns: NULL if n is not zero,
//          a new STXTTranslator if n is zero
// ---------------------------------------------------------------
BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (!n)
		return new (std::nothrow) STXTTranslator();

	return NULL;
}


// #pragma mark - ascmagic.c from the BSD file tool
/*
 * The following code has been taken from version 4.17 of the BSD file tool,
 * file ascmagic.c, modified for our purpose.
 */

/*
 * Copyright (c) Ian F. Darwin 1986-1995.
 * Software written by Ian F. Darwin and others;
 * maintained 1995-present by Christos Zoulas and others.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * ASCII magic -- file types that we know based on keywords
 * that can appear anywhere in the file.
 *		bool found = false;
		if (subtypeMimeSpecific != NULL) {
			mimeType->SetTo(subtypeMimeSpecific);
			if (mimeType->IsInstalled())
				found = true;
		}
		if (!found && subtypeMimeGeneric != NULL) {
			mimeType->SetTo(subtypeMimeGeneric);
			if (mimeType->IsInstalled())
				found = true;
		}
		if (!found)
			mimeType->SetTo("text/plain");

 * Extensively modified by Eric Fischer <enf@pobox.com> in July, 2000,
 * to handle character codes other than ASCII on a unified basis.
 *
 * Joerg Wunsch <joerg@freebsd.org> wrote the original support for 8-bit
 * international characters, now subsumed into this file.
 */

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "names.h"

typedef unsigned long my_unichar;

#define MAXLINELEN 300	/* longest sane line length */
#define ISSPC(x) ((x) == ' ' || (x) == '\t' || (x) == '\r' || (x) == '\n' \
		  || (x) == 0x85 || (x) == '\f')

static int looks_ascii(const unsigned char *, size_t, my_unichar *, size_t *);
static int looks_utf8(const unsigned char *, size_t, my_unichar *, size_t *);
static int looks_unicode(const unsigned char *, size_t, my_unichar *, size_t *);
static int looks_latin1(const unsigned char *, size_t, my_unichar *, size_t *);
static int looks_extended(const unsigned char *, size_t, my_unichar *, size_t *);
static void from_ebcdic(const unsigned char *, size_t, unsigned char *);
static int ascmatch(const unsigned char *, const my_unichar *, size_t);


static int
file_ascmagic(const unsigned char *buf, size_t nbytes, BMimeType* mimeType,
	const char*& encoding)
{
	size_t i;
	unsigned char *nbuf = NULL;
	my_unichar *ubuf = NULL;
	size_t ulen;
	struct names *p;
	int rv = -1;

	const char *code = NULL;
	encoding = NULL;
	const char *type = NULL;
	const char *subtype = NULL;
	const char *subtypeMimeGeneric = NULL;
	const char *subtypeMimeSpecific = NULL;

	int has_escapes = 0;
	int has_backspace = 0;
	int seen_cr = 0;

	int n_crlf = 0;
	int n_lf = 0;
	int n_cr = 0;
	int n_nel = 0;

	int last_line_end = -1;
	int has_long_lines = 0;

	if ((nbuf = (unsigned char*)malloc((nbytes + 1) * sizeof(nbuf[0]))) == NULL)
		goto done;
	if ((ubuf = (my_unichar*)malloc((nbytes + 1) * sizeof(ubuf[0]))) == NULL)
		goto done;

	/*
	 * Then try to determine whether it's any character code we can
	 * identify.  Each of these tests, if it succeeds, will leave
	 * the text converted into one-my_unichar-per-character Unicode in
	 * ubuf, and the number of characters converted in ulen.
	 */
	if (nbytes == 0) {
		code = "UTF-8 Unicode";
		encoding = NULL; // "UTF-8";
		type = "text";
		rv = 1;
	} else if (looks_ascii(buf, nbytes, ubuf, &ulen)) {
		code = "ASCII";
		encoding = NULL; //"us-ascii";
		type = "text";
		if (nbytes == 1) {
			// no further tests
			rv = 1;
		}
	} else if (looks_utf8(buf, nbytes, ubuf, &ulen)) {
		code = "UTF-8 Unicode";
		encoding = NULL; // "UTF-8";
		type = "text";
	} else if ((i = looks_unicode(buf, nbytes, ubuf, &ulen)) != 0) {
		if (i == 1) {
			code = "Little-endian UTF-16 Unicode";
			encoding = "UTF-16";
		} else {
			code = "Big-endian UTF-16 Unicode";
			encoding = "UTF-16";
		}

		type = "character data";
	} else if (looks_latin1(buf, nbytes, ubuf, &ulen)) {
		code = "ISO-8859";
		type = "text";
		encoding = "iso-8859-1";
	} else if (looks_extended(buf, nbytes, ubuf, &ulen)) {
		code = "Non-ISO extended-ASCII";
		type = "text";
		encoding = "unknown";
	} else {
		from_ebcdic(buf, nbytes, nbuf);

		if (looks_ascii(nbuf, nbytes, ubuf, &ulen)) {
			code = "EBCDIC";
			type = "character data";
			encoding = "ebcdic";
		} else if (looks_latin1(nbuf, nbytes, ubuf, &ulen)) {
			code = "International EBCDIC";
			type = "character data";
			encoding = "ebcdic";
		} else {
			rv = 0;
			goto done;  /* doesn't look like text at all */
		}
	}

	if (nbytes <= 1) {
		if (rv == -1)
			rv = 0;
		goto done;
	}

	/*
	 * for troff, look for . + letter + letter or .\";
	 * this must be done to disambiguate tar archives' ./file
	 * and other trash from real troff input.
	 *
	 * I believe Plan 9 troff allows non-ASCII characters in the names
	 * of macros, so this test might possibly fail on such a file.
	 */
	if (*ubuf == '.') {
		my_unichar *tp = ubuf + 1;

		while (ISSPC(*tp))
			++tp;	/* skip leading whitespace */
		if ((tp[0] == '\\' && tp[1] == '\"') ||
		    (isascii((unsigned char)tp[0]) &&
		     isalnum((unsigned char)tp[0]) &&
		     isascii((unsigned char)tp[1]) &&
		     isalnum((unsigned char)tp[1]) &&
		     ISSPC(tp[2]))) {
		    subtypeMimeGeneric = "text/x-source-code";
			subtypeMimeSpecific = "text/troff";
			subtype = "troff or preprocessor input";
			goto subtype_identified;
		}
	}

	if ((*buf == 'c' || *buf == 'C') && ISSPC(buf[1])) {
		subtypeMimeGeneric = "text/x-source-code";
		subtypeMimeSpecific = "text/fortran";
		subtype = "fortran program";
		goto subtype_identified;
	}

	/* look for tokens from names.h - this is expensive! */

	i = 0;
	while (i < ulen) {
		size_t end;

		/*
		 * skip past any leading space
		 */
		while (i < ulen && ISSPC(ubuf[i]))
			i++;
		if (i >= ulen)
			break;

		/*
		 * find the next whitespace
		 */
		for (end = i + 1; end < nbytes; end++)
			if (ISSPC(ubuf[end]))
				break;

		/*
		 * compare the word thus isolated against the token list
		 */
		for (p = names; p < names + NNAMES; p++) {
			if (ascmatch((const unsigned char *)p->name, ubuf + i,
			    end - i)) {
				subtype = types[p->type].human;
				subtypeMimeGeneric = types[p->type].generic_mime;
				subtypeMimeSpecific = types[p->type].specific_mime;
				goto subtype_identified;
			}
		}

		i = end;
	}

subtype_identified:

	/*
	 * Now try to discover other details about the file.
	 */
	for (i = 0; i < ulen; i++) {
		if (ubuf[i] == '\n') {
			if (seen_cr)
				n_crlf++;
			else
				n_lf++;
			last_line_end = i;
		} else if (seen_cr)
			n_cr++;

		seen_cr = (ubuf[i] == '\r');
		if (seen_cr)
			last_line_end = i;

		if (ubuf[i] == 0x85) { /* X3.64/ECMA-43 "next line" character */
			n_nel++;
			last_line_end = i;
		}

		/* If this line is _longer_ than MAXLINELEN, remember it. */
		if ((int)i > last_line_end + MAXLINELEN)
			has_long_lines = 1;

		if (ubuf[i] == '\033')
			has_escapes = 1;
		if (ubuf[i] == '\b')
			has_backspace = 1;
	}

	rv = 1;
done:
	if (nbuf)
		free(nbuf);
	if (ubuf)
		free(ubuf);

	if (rv) {
		// If we have identified the subtype, return it, otherwise just
		// text/plain.

		bool found = false;
		if (subtypeMimeSpecific != NULL) {
			mimeType->SetTo(subtypeMimeSpecific);
			if (mimeType->IsInstalled())
				found = true;
		}
		if (!found && subtypeMimeGeneric != NULL) {
			mimeType->SetTo(subtypeMimeGeneric);
			if (mimeType->IsInstalled())
				found = true;
		}
		if (!found)
			mimeType->SetTo("text/plain");
	}

	return rv;
}

static int
ascmatch(const unsigned char *s, const my_unichar *us, size_t ulen)
{
	size_t i;

	for (i = 0; i < ulen; i++) {
		if (s[i] != us[i])
			return 0;
	}

	if (s[i])
		return 0;
	else
		return 1;
}

/*
 * This table reflects a particular philosophy about what constitutes
 * "text," and there is room for disagreement about it.
 *
 * Version 3.31 of the file command considered a file to be ASCII if
 * each of its characters was approved by either the isascii() or
 * isalpha() function.  On most systems, this would mean that any
 * file consisting only of characters in the range 0x00 ... 0x7F
 * would be called ASCII text, but many systems might reasonably
 * consider some characters outside this range to be alphabetic,
 * so the file command would call such characters ASCII.  It might
 * have been more accurate to call this "considered textual on the
 * local system" than "ASCII."
 *
 * It considered a file to be "International language text" if each
 * of its characters was either an ASCII printing character (according
 * to the real ASCII standard, not the above test), a character in
 * the range 0x80 ... 0xFF, or one of the following control characters:
 * backspace, tab, line feed, vertical tab, form feed, carriage return,
 * escape.  No attempt was made to determine the language in which files
 * of this type were written.
 *
 *
 * The table below considers a file to be ASCII if all of its characters
 * are either ASCII printing characters (again, according to the X3.4
 * standard, not isascii()) or any of the following controls: bell,
 * backspace, tab, line feed, form feed, carriage return, esc, nextline.
 *
 * I include bell because some programs (particularly shell scripts)
 * use it literally, even though it is rare in normal text.  I exclude
 * vertical tab because it never seems to be used in real text.  I also
 * include, with hesitation, the X3.64/ECMA-43 control nextline (0x85),
 * because that's what the dd EBCDIC->ASCII table maps the EBCDIC newline
 * character to.  It might be more appropriate to include it in the 8859
 * set instead of the ASCII set, but it's got to be included in *something*
 * we recognize or EBCDIC files aren't going to be considered textual.
 * Some old Unix source files use SO/SI (^N/^O) to shift between Greek
 * and Latin characters, so these should possibly be allowed.  But they
 * make a real mess on VT100-style displays if they're not paired properly,
 * so we are probably better off not calling them text.
 *
 * A file is considered to be ISO-8859 text if its characters are all
 * either ASCII, according to the above definition, or printing characters
 * from the ISO-8859 8-bit extension, characters 0xA0 ... 0xFF.
 *
 * Finally, a file is considered to be international text from some other
 * character code if its characters are all either ISO-8859 (according to
 * the above definition) or characters in the range 0x80 ... 0x9F, which
 * ISO-8859 considers to be control characters but the IBM PC and Macintosh
 * consider to be printing characters.
 */

#define F 0   /* character never appears in text */
#define T 1   /* character appears in plain ASCII text */
#define I 2   /* character appears in ISO-8859 text */
#define X 3   /* character appears in non-ISO extended ASCII (Mac, IBM PC) */

static char text_chars[256] = {
	/*                  BEL BS HT LF    FF CR    */
	F, F, F, F, F, F, F, T, T, T, T, F, T, T, F, F,  /* 0x0X */
        /*                              ESC          */
	F, F, F, F, F, F, F, F, F, F, F, T, F, F, F, F,  /* 0x1X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x2X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x3X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x4X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x5X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x6X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, F,  /* 0x7X */
	/*            NEL                            */
	X, X, X, X, X, T, X, X, X, X, X, X, X, X, X, X,  /* 0x8X */
	X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,  /* 0x9X */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xaX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xbX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xcX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xdX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xeX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I   /* 0xfX */
};

static int
looks_ascii(const unsigned char *buf, size_t nbytes, my_unichar *ubuf,
    size_t *ulen)
{
	int i;

	*ulen = 0;

	for (i = 0; i < (int)nbytes; i++) {
		int t = text_chars[buf[i]];

		if (t != T)
			return 0;

		ubuf[(*ulen)++] = buf[i];
	}

	return 1;
}

static int
looks_latin1(const unsigned char *buf, size_t nbytes, my_unichar *ubuf, size_t *ulen)
{
	int i;

	*ulen = 0;

	for (i = 0; i < (int)nbytes; i++) {
		int t = text_chars[buf[i]];

		if (t != T && t != I)
			return 0;

		ubuf[(*ulen)++] = buf[i];
	}

	return 1;
}

static int
looks_extended(const unsigned char *buf, size_t nbytes, my_unichar *ubuf,
    size_t *ulen)
{
	int i;

	*ulen = 0;

	for (i = 0; i < (int)nbytes; i++) {
		int t = text_chars[buf[i]];

		if (t != T && t != I && t != X)
			return 0;

		ubuf[(*ulen)++] = buf[i];
	}

	return 1;
}

static int
looks_utf8(const unsigned char *buf, size_t nbytes, my_unichar *ubuf, size_t *ulen)
{
	int i, n;
	my_unichar c;
	int gotone = 0;

	*ulen = 0;

	for (i = 0; i < (int)nbytes; i++) {
		if ((buf[i] & 0x80) == 0) {	   /* 0xxxxxxx is plain ASCII */
			/*
			 * Even if the whole file is valid UTF-8 sequences,
			 * still reject it if it uses weird control characters.
			 */

			if (text_chars[buf[i]] != T)
				return 0;

			ubuf[(*ulen)++] = buf[i];
		} else if ((buf[i] & 0x40) == 0) { /* 10xxxxxx never 1st byte */
			return 0;
		} else {			   /* 11xxxxxx begins UTF-8 */
			int following;

			if ((buf[i] & 0x20) == 0) {		/* 110xxxxx */
				c = buf[i] & 0x1f;
				following = 1;
			} else if ((buf[i] & 0x10) == 0) {	/* 1110xxxx */
				c = buf[i] & 0x0f;
				following = 2;
			} else if ((buf[i] & 0x08) == 0) {	/* 11110xxx */
				c = buf[i] & 0x07;
				following = 3;
			} else if ((buf[i] & 0x04) == 0) {	/* 111110xx */
				c = buf[i] & 0x03;
				following = 4;
			} else if ((buf[i] & 0x02) == 0) {	/* 1111110x */
				c = buf[i] & 0x01;
				following = 5;
			} else
				return 0;

			for (n = 0; n < following; n++) {
				i++;
				if (i >= (int)nbytes)
					goto done;

				if ((buf[i] & 0x80) == 0 || (buf[i] & 0x40))
					return 0;

				c = (c << 6) + (buf[i] & 0x3f);
			}

			ubuf[(*ulen)++] = c;
			gotone = 1;
		}
	}
done:
	return gotone;   /* don't claim it's UTF-8 if it's all 7-bit */
}

static int
looks_unicode(const unsigned char *buf, size_t nbytes, my_unichar *ubuf,
    size_t *ulen)
{
	int bigend;
	int i;

	if (nbytes < 2)
		return 0;

	if (buf[0] == 0xff && buf[1] == 0xfe)
		bigend = 0;
	else if (buf[0] == 0xfe && buf[1] == 0xff)
		bigend = 1;
	else
		return 0;

	*ulen = 0;

	for (i = 2; i + 1 < (int)nbytes; i += 2) {
		/* XXX fix to properly handle chars > 65536 */

		if (bigend)
			ubuf[(*ulen)++] = buf[i + 1] + 256 * buf[i];
		else
			ubuf[(*ulen)++] = buf[i] + 256 * buf[i + 1];

		if (ubuf[*ulen - 1] == 0xfffe)
			return 0;
		if (ubuf[*ulen - 1] < 128 &&
		    text_chars[(size_t)ubuf[*ulen - 1]] != T)
			return 0;
	}

	return 1 + bigend;
}

#undef F
#undef T
#undef I
#undef X

/*
 * This table maps each EBCDIC character to an (8-bit extended) ASCII
 * character, as specified in the rationale for the dd(1) command in
 * draft 11.2 (September, 1991) of the POSIX P1003.2 standard.
 *
 * Unfortunately it does not seem to correspond exactly to any of the
 * five variants of EBCDIC documented in IBM's _Enterprise Systems
 * Architecture/390: Principles of Operation_, SA22-7201-06, Seventh
 * Edition, July, 1999, pp. I-1 - I-4.
 *
 * Fortunately, though, all versions of EBCDIC, including this one, agree
 * on most of the printing characters that also appear in (7-bit) ASCII.
 * Of these, only '|', '!', '~', '^', '[', and ']' are in question at all.
 *
 * Fortunately too, there is general agreement that codes 0x00 through
 * 0x3F represent control characters, 0x41 a nonbreaking space, and the
 * remainder printing characters.
 *
 * This is sufficient to allow us to identify EBCDIC text and to distinguish
 * between old-style and internationalized examples of text.
 */

static unsigned char ebcdic_to_ascii[] = {
  0,   1,   2,   3, 156,   9, 134, 127, 151, 141, 142,  11,  12,  13,  14,  15,
 16,  17,  18,  19, 157, 133,   8, 135,  24,  25, 146, 143,  28,  29,  30,  31,
128, 129, 130, 131, 132,  10,  23,  27, 136, 137, 138, 139, 140,   5,   6,   7,
144, 145,  22, 147, 148, 149, 150,   4, 152, 153, 154, 155,  20,  21, 158,  26,
' ', 160, 161, 162, 163, 164, 165, 166, 167, 168, 213, '.', '<', '(', '+', '|',
'&', 169, 170, 171, 172, 173, 174, 175, 176, 177, '!', '$', '*', ')', ';', '~',
'-', '/', 178, 179, 180, 181, 182, 183, 184, 185, 203, ',', '%', '_', '>', '?',
186, 187, 188, 189, 190, 191, 192, 193, 194, '`', ':', '#', '@', '\'','=', '"',
195, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 196, 197, 198, 199, 200, 201,
202, 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', '^', 204, 205, 206, 207, 208,
209, 229, 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 210, 211, 212, '[', 214, 215,
216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, ']', 230, 231,
'{', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 232, 233, 234, 235, 236, 237,
'}', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 238, 239, 240, 241, 242, 243,
'\\',159, 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 244, 245, 246, 247, 248, 249,
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 250, 251, 252, 253, 254, 255
};

#ifdef notdef
/*
 * The following EBCDIC-to-ASCII table may relate more closely to reality,
 * or at least to modern reality.  It comes from
 *
 *   http://ftp.s390.ibm.com/products/oe/bpxqp9.html
 *
 * and maps the characters of EBCDIC code page 1047 (the code used for
 * Unix-derived software on IBM's 390 systems) to the corresponding
 * characters from ISO 8859-1.
 *
 * If this table is used instead of the above one, some of the special
 * cases for the NEL character can be taken out of the code.
 */

static unsigned char ebcdic_1047_to_8859[] = {
0x00,0x01,0x02,0x03,0x9C,0x09,0x86,0x7F,0x97,0x8D,0x8E,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x9D,0x0A,0x08,0x87,0x18,0x19,0x92,0x8F,0x1C,0x1D,0x1E,0x1F,
0x80,0x81,0x82,0x83,0x84,0x85,0x17,0x1B,0x88,0x89,0x8A,0x8B,0x8C,0x05,0x06,0x07,
0x90,0x91,0x16,0x93,0x94,0x95,0x96,0x04,0x98,0x99,0x9A,0x9B,0x14,0x15,0x9E,0x1A,
0x20,0xA0,0xE2,0xE4,0xE0,0xE1,0xE3,0xE5,0xE7,0xF1,0xA2,0x2E,0x3C,0x28,0x2B,0x7C,
0x26,0xE9,0xEA,0xEB,0xE8,0xED,0xEE,0xEF,0xEC,0xDF,0x21,0x24,0x2A,0x29,0x3B,0x5E,
0x2D,0x2F,0xC2,0xC4,0xC0,0xC1,0xC3,0xC5,0xC7,0xD1,0xA6,0x2C,0x25,0x5F,0x3E,0x3F,
0xF8,0xC9,0xCA,0xCB,0xC8,0xCD,0xCE,0xCF,0xCC,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
0xD8,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0xAB,0xBB,0xF0,0xFD,0xFE,0xB1,
0xB0,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0xAA,0xBA,0xE6,0xB8,0xC6,0xA4,
0xB5,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0xA1,0xBF,0xD0,0x5B,0xDE,0xAE,
0xAC,0xA3,0xA5,0xB7,0xA9,0xA7,0xB6,0xBC,0xBD,0xBE,0xDD,0xA8,0xAF,0x5D,0xB4,0xD7,
0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0xAD,0xF4,0xF6,0xF2,0xF3,0xF5,
0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0xB9,0xFB,0xFC,0xF9,0xFA,0xFF,
0x5C,0xF7,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0xB2,0xD4,0xD6,0xD2,0xD3,0xD5,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xB3,0xDB,0xDC,0xD9,0xDA,0x9F
};
#endif

/*
 * Copy buf[0 ... nbytes-1] into out[], translating EBCDIC to ASCII.
 */
static void
from_ebcdic(const unsigned char *buf, size_t nbytes, unsigned char *out)
{
	int i;

	for (i = 0; i < (int)nbytes; i++) {
		out[i] = ebcdic_to_ascii[buf[i]];
	}
}


//	#pragma mark -


/*!
	Determines if the data in inSource is of the STXT format.

	\param header the STXT stream header read in by Identify() or Translate()
	\param inSource the stream with the STXT data
	\param outInfo information about the type of data from inSource is stored here
	\param outType the desired output type for the data in inSource
	\param ptxtheader if this is not NULL, the TEXT header from
		inSource is copied to it
*/
status_t
identify_stxt_header(const TranslatorStyledTextStreamHeader &header,
	BPositionIO *inSource, translator_info *outInfo, uint32 outType,
	TranslatorStyledTextTextHeader *ptxtheader = NULL)
{
	const ssize_t ktxtsize = sizeof(TranslatorStyledTextTextHeader);
	const ssize_t kstylsize = sizeof(TranslatorStyledTextStyleHeader);

	uint8 buffer[max(ktxtsize, kstylsize)];

	// Check the TEXT header
	TranslatorStyledTextTextHeader txtheader;
	if (inSource->Read(buffer, ktxtsize) != ktxtsize)
		return B_NO_TRANSLATOR;

	memcpy(&txtheader, buffer, ktxtsize);
	if (swap_data(B_UINT32_TYPE, &txtheader, ktxtsize,
		B_SWAP_BENDIAN_TO_HOST) != B_OK)
		return B_ERROR;

	if (txtheader.header.magic != 'TEXT'
		|| txtheader.header.header_size != sizeof(TranslatorStyledTextTextHeader)
		|| txtheader.charset != B_UNICODE_UTF8)
		return B_NO_TRANSLATOR;

	// skip the text data
	off_t seekresult, pos;
	pos = header.header.header_size + txtheader.header.header_size
		+ txtheader.header.data_size;
	seekresult = inSource->Seek(txtheader.header.data_size,
		SEEK_CUR);
	if (seekresult < pos)
		return B_NO_TRANSLATOR;
	if (seekresult > pos)
		return B_ERROR;

	// check the STYL header (not all STXT files have this)
	ssize_t read = 0;
	TranslatorStyledTextStyleHeader stylheader;
	read = inSource->Read(buffer, kstylsize);
	if (read < 0)
		return read;
	if (read != kstylsize && read != 0)
		return B_NO_TRANSLATOR;

	// If there is a STYL header
	if (read == kstylsize) {
		memcpy(&stylheader, buffer, kstylsize);
		if (swap_data(B_UINT32_TYPE, &stylheader, kstylsize,
			B_SWAP_BENDIAN_TO_HOST) != B_OK)
			return B_ERROR;

		if (stylheader.header.magic != 'STYL'
			|| stylheader.header.header_size !=
				sizeof(TranslatorStyledTextStyleHeader))
			return B_NO_TRANSLATOR;
	}

	// if output TEXT header is supplied, fill it with data
	if (ptxtheader) {
		ptxtheader->header.magic = txtheader.header.magic;
		ptxtheader->header.header_size = txtheader.header.header_size;
		ptxtheader->header.data_size = txtheader.header.data_size;
		ptxtheader->charset = txtheader.charset;
	}

	// return information about the data in the stream
	outInfo->type = B_STYLED_TEXT_FORMAT;
	outInfo->group = B_TRANSLATOR_TEXT;
	outInfo->quality = STXT_IN_QUALITY;
	outInfo->capability = STXT_IN_CAPABILITY;
	strlcpy(outInfo->name, B_TRANSLATE("Be styled text file"),
		sizeof(outInfo->name));
	strcpy(outInfo->MIME, "text/x-vnd.Be-stxt");

	return B_OK;
}


/*!
	Determines if the data in \a inSource is of the UTF8 plain

	\param data buffer containing data already read (must be at
		least DATA_BUFFER_SIZE bytes large)
	\param nread number of bytes that have already been read from the stream
	\param header the STXT stream header read in by Identify() or Translate()
	\param inSource the stream with the STXT data
	\param outInfo information about the type of data from inSource is stored here
	\param outType the desired output type for the data in inSource
*/
status_t
identify_text(uint8* data, int32 bytesRead, BPositionIO* source,
	translator_info* outInfo, uint32 outType, const char*& encoding)
{
	ssize_t readLater = source->Read(data + bytesRead, DATA_BUFFER_SIZE - bytesRead);
	if (readLater < B_OK)
		return B_NO_TRANSLATOR;

	bytesRead += readLater;

	// TODO: identify encoding as possible!
	BMimeType type;
	if (!file_ascmagic((const unsigned char*)data, bytesRead, &type, encoding))
		return B_NO_TRANSLATOR;

	float capability = TEXT_IN_CAPABILITY;
	if (bytesRead < 20)
		capability = .1f;

	// return information about the data in the stream
	outInfo->type = B_TRANSLATOR_TEXT;
	outInfo->group = B_TRANSLATOR_TEXT;
	outInfo->quality = TEXT_IN_QUALITY;
	outInfo->capability = capability;

	char description[B_MIME_TYPE_LENGTH];
	if (type.GetLongDescription(description) == B_OK)
		strlcpy(outInfo->name, description, sizeof(outInfo->name));
	else
		strlcpy(outInfo->name, B_TRANSLATE("Plain text file"), 
			sizeof(outInfo->name));

	//strlcpy(outInfo->MIME, type.Type(), sizeof(outInfo->MIME));
	strcpy(outInfo->MIME, "text/plain");
	return B_OK;
}


// ---------------------------------------------------------------
// translate_from_stxt
//
// Translates the data in inSource to the type outType and stores
// the translated data in outDestination.
//
// Preconditions:
//
// Parameters:	inSource,	the data to be translated
//
//				outDestination,	where the translated data is
//								put
//
//				outType,	the type to convert inSource to
//
//				txtheader, 	the TEXT header from inSource
//
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if outType is invalid
//
// B_NO_TRANSLATOR, if this translator doesn't understand the data
//
// B_ERROR, if there was an error allocating memory or converting
//          data
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
translate_from_stxt(BPositionIO *inSource, BPositionIO *outDestination,
		uint32 outType, const TranslatorStyledTextTextHeader &txtheader)
{
	if (inSource->Seek(0, SEEK_SET) != 0)
		return B_ERROR;

	const ssize_t kstxtsize = sizeof(TranslatorStyledTextStreamHeader);
	const ssize_t ktxtsize = sizeof(TranslatorStyledTextTextHeader);

	bool btoplain;
	if (outType == B_TRANSLATOR_TEXT)
		btoplain = true;
	else if (outType == B_STYLED_TEXT_FORMAT)
		btoplain = false;
	else
		return B_BAD_VALUE;

	uint8 buffer[READ_BUFFER_SIZE];
	ssize_t nread = 0, nwritten = 0, nreed = 0, ntotalread = 0;

	// skip to the actual text data when outputting a
	// plain text file
	if (btoplain) {
		if (inSource->Seek(kstxtsize + ktxtsize, SEEK_CUR) !=
			kstxtsize + ktxtsize)
			return B_ERROR;
	}

	// Read data from inSource
	// When outputing B_TRANSLATOR_TEXT, the loop stops when all of
	// the text data has been read and written.
	// When outputting B_STYLED_TEXT_FORMAT, the loop stops when all
	// of the data from inSource has been read and written.
	if (btoplain)
		nreed = min((size_t)READ_BUFFER_SIZE,
			txtheader.header.data_size - ntotalread);
	else
		nreed = READ_BUFFER_SIZE;
	nread = inSource->Read(buffer, nreed);
	while (nread > 0) {
		nwritten = outDestination->Write(buffer, nread);
		if (nwritten != nread)
			return B_ERROR;

		if (btoplain) {
			ntotalread += nread;
			nreed = min((size_t)READ_BUFFER_SIZE,
				txtheader.header.data_size - ntotalread);
		} else
			nreed = READ_BUFFER_SIZE;
		nread = inSource->Read(buffer, nreed);
	}

	if (btoplain && static_cast<ssize_t>(txtheader.header.data_size) !=
		ntotalread)
		// If not all of the text data was able to be read...
		return B_NO_TRANSLATOR;
	else
		return B_OK;
}

// ---------------------------------------------------------------
// output_headers
//
// Outputs the Stream and Text headers from the B_STYLED_TEXT_FORMAT
// to outDestination, setting the data_size member of the text header
// to text_data_size
//
// Preconditions:
//
// Parameters:	outDestination,	where the translated data is
//								put
//
//				text_data_size, number of bytes in data section
//							    of the TEXT header
//
//
// Postconditions:
//
// Returns:
//
// B_ERROR, if there was an error writing to outDestination or
// 	an error with converting the byte order
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
output_headers(BPositionIO *outDestination, uint32 text_data_size)
{
	const int32 kHeadersSize = sizeof(TranslatorStyledTextStreamHeader) +
		sizeof(TranslatorStyledTextTextHeader);
	status_t result;
	TranslatorStyledTextStreamHeader stxtheader;
	TranslatorStyledTextTextHeader txtheader;

	uint8 buffer[kHeadersSize];

	stxtheader.header.magic = 'STXT';
	stxtheader.header.header_size = sizeof(TranslatorStyledTextStreamHeader);
	stxtheader.header.data_size = 0;
	stxtheader.version = 100;
	memcpy(buffer, &stxtheader, stxtheader.header.header_size);

	txtheader.header.magic = 'TEXT';
	txtheader.header.header_size = sizeof(TranslatorStyledTextTextHeader);
	txtheader.header.data_size = text_data_size;
	txtheader.charset = B_UNICODE_UTF8;
	memcpy(buffer + stxtheader.header.header_size, &txtheader,
		txtheader.header.header_size);

	// write out headers in Big Endian byte order
	result = swap_data(B_UINT32_TYPE, buffer, kHeadersSize,
		B_SWAP_HOST_TO_BENDIAN);
	if (result == B_OK) {
		ssize_t nwritten = 0;
		nwritten = outDestination->Write(buffer, kHeadersSize);
		if (nwritten != kHeadersSize)
			return B_ERROR;
		else
			return B_OK;
	}

	return result;
}

// ---------------------------------------------------------------
// output_styles
//
// Writes out the actual style information into outDestination
// using the data from pflatRunArray
//
// Preconditions:
//
// Parameters:	outDestination,	where the translated data is
//								put
//
//				text_size,		size in bytes of the text in
//								outDestination
//
//				data_size,		size of pflatRunArray
//
// Postconditions:
//
// Returns:
//
// B_ERROR, if there was an error writing to outDestination or
// 	an error with converting the byte order
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
output_styles(BPositionIO *outDestination, uint32 text_size,
	uint8 *pflatRunArray, ssize_t data_size)
{
	const ssize_t kstylsize = sizeof(TranslatorStyledTextStyleHeader);

	uint8 buffer[kstylsize];

	// output STYL header
	TranslatorStyledTextStyleHeader stylheader;
	stylheader.header.magic = 'STYL';
	stylheader.header.header_size =
		sizeof(TranslatorStyledTextStyleHeader);
	stylheader.header.data_size = data_size;
	stylheader.apply_offset = 0;
	stylheader.apply_length = text_size;

	memcpy(buffer, &stylheader, kstylsize);
	if (swap_data(B_UINT32_TYPE, buffer, kstylsize,
		B_SWAP_HOST_TO_BENDIAN) != B_OK)
		return B_ERROR;
	if (outDestination->Write(buffer, kstylsize) != kstylsize)
		return B_ERROR;

	// output actual style information
	if (outDestination->Write(pflatRunArray,
		data_size) != data_size)
		return B_ERROR;

	return B_OK;
}


/*!
	Convert the plain text (UTF8) from inSource to plain or
	styled text in outDestination
*/
status_t
translate_from_text(BPositionIO* source, const char* encoding, bool forceEncoding,
	BPositionIO* destination, uint32 outType)
{
	if (outType != B_TRANSLATOR_TEXT && outType != B_STYLED_TEXT_FORMAT)
		return B_BAD_VALUE;

	// find the length of the text
	off_t size = source->Seek(0, SEEK_END);
	if (size < 0)
		return (status_t)size;
	if (size > UINT32_MAX && outType == B_STYLED_TEXT_FORMAT)
		return B_NOT_SUPPORTED;

	status_t status = source->Seek(0, SEEK_SET);
	if (status < B_OK)
		return status;

	if (outType == B_STYLED_TEXT_FORMAT) {
		// output styled text headers
		status = output_headers(destination, (uint32)size);
		if (status != B_OK)
			return status;
	}

	class MallocBuffer {
		public:
			MallocBuffer() : fBuffer(NULL), fSize(0) {}
			~MallocBuffer() { free(fBuffer); }

			void* Buffer() { return fBuffer; }
			size_t Size() const { return fSize; }

			status_t
			Allocate(size_t size)
			{
				fBuffer = malloc(size);
				if (fBuffer != NULL) {
					fSize = size;
					return B_OK;
				}
				return B_NO_MEMORY;
			}

		private:
			void*	fBuffer;
			size_t	fSize;
	} encodingBuffer;
	BMallocIO encodingIO;
	uint32 encodingID = 0;
		// defaults to UTF-8 or no encoding

	BNode* node = dynamic_cast<BNode*>(source);
	if (node != NULL) {
		// determine encoding, if available
		const BCharacterSet* characterSet = NULL;
		bool hasAttribute = false;
		if (encoding != NULL && !forceEncoding) {
			BString name;
			if (node->ReadAttrString("be:encoding", &name) == B_OK) {
				encoding = name.String();
				hasAttribute = true;
			} else {
				int32 value;
				ssize_t bytesRead = node->ReadAttr("be:encoding", B_INT32_TYPE, 0,
					&value, sizeof(value));
				if (bytesRead == (ssize_t)sizeof(value)) {
					hasAttribute = true;
					if (value != 65535)
						characterSet = BCharacterSetRoster::GetCharacterSetByConversionID(value);
				}
			}
		} else {
			hasAttribute = true;
				// we don't write the encoding in this case
		}
		if (characterSet == NULL && encoding != NULL)
			characterSet = BCharacterSetRoster::FindCharacterSetByName(encoding);

		if (characterSet != NULL) {
			encodingID = characterSet->GetConversionID();
			encodingBuffer.Allocate(READ_BUFFER_SIZE * 4);
		}

		if (!hasAttribute && encoding != NULL) {
			// add encoding attribute, so that someone opening the file can
			// retrieve it for persistance
			node->WriteAttr("be:encoding", B_STRING_TYPE, 0, encoding,
				strlen(encoding));
		}
	}

	off_t outputSize = 0;
	ssize_t bytesRead;
	int32 state = 0;

	// output the actual text part of the data
	do {
		uint8 buffer[READ_BUFFER_SIZE];
		bytesRead = source->Read(buffer, READ_BUFFER_SIZE);
		if (bytesRead < B_OK)
			return bytesRead;
		if (bytesRead == 0)
			break;

		if (encodingBuffer.Size() == 0) {
			// default, no encoding
			ssize_t bytesWritten = destination->Write(buffer, bytesRead);
			if (bytesWritten != bytesRead) {
				if (bytesWritten < B_OK)
					return bytesWritten;

				return B_ERROR;
			}

			outputSize += bytesRead;
		} else {
			// decode text file to UTF-8
			char* pos = (char*)buffer;
			int32 encodingLength = encodingIO.BufferLength();
			int32 bytesLeft = bytesRead;
			int32 bytes;
			do {
				encodingLength = READ_BUFFER_SIZE * 4;
				bytes = bytesLeft;

				status = convert_to_utf8(encodingID, pos, &bytes,
					(char*)encodingBuffer.Buffer(), &encodingLength, &state);
				if (status < B_OK)
					return status;

				ssize_t bytesWritten = destination->Write(encodingBuffer.Buffer(),
					encodingLength);
				if (bytesWritten < encodingLength) {
					if (bytesWritten < B_OK)
						return bytesWritten;

					return B_ERROR;
				}

				pos += bytes;
				bytesLeft -= bytes;
				outputSize += encodingLength;
			} while (encodingLength > 0 && bytesLeft > 0);
		}
	} while (bytesRead > 0);

	if (outType != B_STYLED_TEXT_FORMAT)
		return B_OK;

	if (encodingBuffer.Size() != 0 && size != outputSize) {
		if (outputSize > UINT32_MAX)
			return B_NOT_SUPPORTED;

		// we need to update the header as the decoded text size has changed
		status = destination->Seek(0, SEEK_SET);
		if (status == B_OK)
			status = output_headers(destination, (uint32)outputSize);
		if (status == B_OK)
			status = destination->Seek(0, SEEK_END);

		if (status < B_OK)
			return status;
	}

	// Read file attributes if outputting styled data
	// and source is a BNode object

	if (node == NULL)
		return B_OK;

	// Try to read styles - we only propagate an error if the actual on-disk
	// data is likely to be okay

	const char *kAttrName = "styles";
	attr_info info;
	if (node->GetAttrInfo(kAttrName, &info) != B_OK)
		return B_OK;

	if (info.type != B_RAW_TYPE || info.size < 160) {
		// styles seem to be broken, but since we got the text,
		// we don't propagate the error
		return B_OK;
	}

	uint8* flatRunArray = new (std::nothrow) uint8[info.size];
	if (flatRunArray == NULL)
		return B_NO_MEMORY;

	bytesRead = node->ReadAttr(kAttrName, B_RAW_TYPE, 0, flatRunArray, info.size);
	if (bytesRead != info.size)
		return B_OK;

	output_styles(destination, size, flatRunArray, info.size);

	delete[] flatRunArray;
	return B_OK;
}


//	#pragma mark -


STXTTranslator::STXTTranslator()
	: BaseTranslator(B_TRANSLATE("StyledEdit files"), 
		B_TRANSLATE("StyledEdit files translator"),
		STXT_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"STXTTranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_TEXT, B_STYLED_TEXT_FORMAT)
{
}


STXTTranslator::~STXTTranslator()
{
}


status_t
STXTTranslator::Identify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_TEXT;
	if (outType != B_TRANSLATOR_TEXT && outType != B_STYLED_TEXT_FORMAT)
		return B_NO_TRANSLATOR;

	const ssize_t kstxtsize = sizeof(TranslatorStyledTextStreamHeader);

	uint8 buffer[DATA_BUFFER_SIZE];
	status_t nread = 0;
	// Read in the header to determine
	// if the data is supported
	nread = inSource->Read(buffer, kstxtsize);
	if (nread < 0)
		return nread;

	// read in enough data to fill the stream header
	if (nread == kstxtsize) {
		TranslatorStyledTextStreamHeader header;
		memcpy(&header, buffer, kstxtsize);
		if (swap_data(B_UINT32_TYPE, &header, kstxtsize,
				B_SWAP_BENDIAN_TO_HOST) != B_OK)
			return B_ERROR;

		if (header.header.magic == B_STYLED_TEXT_FORMAT
			&& header.header.header_size == (int32)kstxtsize
			&& header.header.data_size == 0
			&& header.version == 100)
			return identify_stxt_header(header, inSource, outInfo, outType);
	}

	// if the data is not styled text, check if it is plain text
	const char* encoding;
	return identify_text(buffer, nread, inSource, outInfo, outType, encoding);
}


status_t
STXTTranslator::Translate(BPositionIO* source, const translator_info* info,
	BMessage* ioExtension, uint32 outType, BPositionIO* outDestination)
{
	if (!outType)
		outType = B_TRANSLATOR_TEXT;
	if (outType != B_TRANSLATOR_TEXT && outType != B_STYLED_TEXT_FORMAT)
		return B_NO_TRANSLATOR;

	const ssize_t headerSize = sizeof(TranslatorStyledTextStreamHeader);
	uint8 buffer[DATA_BUFFER_SIZE];
	status_t result;
	translator_info outInfo;
	// Read in the header to determine
	// if the data is supported
	ssize_t bytesRead = source->Read(buffer, headerSize);
	if (bytesRead < 0)
		return bytesRead;

	// read in enough data to fill the stream header
	if (bytesRead == headerSize) {
		TranslatorStyledTextStreamHeader header;
		memcpy(&header, buffer, headerSize);
		if (swap_data(B_UINT32_TYPE, &header, headerSize,
				B_SWAP_BENDIAN_TO_HOST) != B_OK)
			return B_ERROR;

		if (header.header.magic == B_STYLED_TEXT_FORMAT
			&& header.header.header_size == sizeof(TranslatorStyledTextStreamHeader)
			&& header.header.data_size == 0
			&& header.version == 100) {
			TranslatorStyledTextTextHeader textHeader;
			result = identify_stxt_header(header, source, &outInfo, outType,
				&textHeader);
			if (result != B_OK)
				return result;

			return translate_from_stxt(source, outDestination, outType, textHeader);
		}
	}

	// if the data is not styled text, check if it is ASCII text
	bool forceEncoding = false;
	const char* encoding = NULL;
	result = identify_text(buffer, bytesRead, source, &outInfo, outType, encoding);
	if (result != B_OK)
		return result;

	if (ioExtension != NULL) {
		const char* value;
		if (ioExtension->FindString("be:encoding", &value) == B_OK
			&& value[0]) {
			// override encoding
			encoding = value;
			forceEncoding = true;
		}
	}

	return translate_from_text(source, encoding, forceEncoding, outDestination, outType);
}


BView *
STXTTranslator::NewConfigView(TranslatorSettings *settings)
{
	return new STXTView(BRect(0, 0, 225, 175), 
		B_TRANSLATE("STXTTranslator Settings"),
		B_FOLLOW_ALL, B_WILL_DRAW, settings);
}

