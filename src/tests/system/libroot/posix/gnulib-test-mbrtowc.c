/* Test of conversion of multibyte character to wide character.
   Copyright (C) 2008-2011 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2008.  */

#undef NDEBUG
#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <Debug.h>

int
main (int argc, char *argv[])
{
  mbstate_t state;
  wchar_t wc;
  size_t ret;
  int i;

  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL) {
	fprintf(stderr, "unable to set standard locale\n");
    return 1;
  }

  /* Test zero-length input.  */
  printf("zero-length input ...\n");
  {
    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, "x", 0, &state);
    /* gnulib's implementation returns (size_t)(-2).
       The AIX 5.1 implementation returns (size_t)(-1).
       glibc's implementation returns 0.  */
    assert (ret == (size_t)(-2) || ret == (size_t)(-1) || ret == 0);
    assert (mbsinit (&state));
  }

  /* Test NUL byte input.  */
  printf("NUL byte input ...\n");
  {
    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, "", 1, &state);
    assert (ret == 0);
    assert (wc == 0);
    assert (mbsinit (&state));
    ret = mbrtowc (NULL, "", 1, &state);
    assert (ret == 0);
    assert (mbsinit (&state));
  }

  /* Test single-byte input.  */
  printf("single-byte input ...\n");
  {
    char buf[1];
    int c;

    memset (&state, '\0', sizeof (mbstate_t));
    for (c = 0; c < 0x100; c++)
      switch (c)
        {
        case '\t': case '\v': case '\f':
        case ' ': case '!': case '"': case '#': case '%':
        case '&': case '\'': case '(': case ')': case '*':
        case '+': case ',': case '-': case '.': case '/':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case ':': case ';': case '<': case '=': case '>':
        case '?':
        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y':
        case 'Z':
        case '[': case '\\': case ']': case '^': case '_':
        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y':
        case 'z': case '{': case '|': case '}': case '~':
          /* c is in the ISO C "basic character set".  */
          buf[0] = c;
          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, buf, 1, &state);
          assert (ret == 1);
          assert (wc == c);
          assert (mbsinit (&state));
          ret = mbrtowc (NULL, buf, 1, &state);
          assert (ret == 1);
          assert (mbsinit (&state));
          break;
        }
  }

  /* Test special calling convention, passing a NULL pointer.  */
  printf("special calling convention, passing NULL ...\n");
  {
    memset (&state, '\0', sizeof (mbstate_t));
    wc = (wchar_t) 0xBADFACE;
    ret = mbrtowc (&wc, NULL, 5, &state);
    assert (ret == 0);
    assert (wc == (wchar_t) 0xBADFACE);
    assert (mbsinit (&state));
  }

  for (i = '1'; i <= '4'; ++i) {
    switch (i)
      {
      case '1':
        /* Locale encoding is ISO-8859-1 or ISO-8859-15.  */
    	printf("ISO8859-1 ...\n");
        {
          char input[] = "B\374\337er"; /* "Büßer" */
          memset (&state, '\0', sizeof (mbstate_t));

       	  if (setlocale (LC_ALL, "en_US.ISO8859-1") == NULL) {
       		  fprintf(stderr, "unable to set ISO8859-1 locale, skipping\n");
       		  break;
       	  }

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input, 1, &state);
          assert (ret == 1);
          assert (wc == 'B');
          assert (mbsinit (&state));
          input[0] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 1, 1, &state);
          assert (ret == 1);
          assert (wctob (wc) == (unsigned char) '\374');
          assert (mbsinit (&state));
          input[1] = '\0';

          /* Test support of NULL first argument.  */
          ret = mbrtowc (NULL, input + 2, 3, &state);
          assert (ret == 1);
          assert (mbsinit (&state));

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 2, 3, &state);
          assert (ret == 1);
          assert (wctob (wc) == (unsigned char) '\337');
          assert (mbsinit (&state));
          input[2] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 3, 2, &state);
          assert (ret == 1);
          assert (wc == 'e');
          assert (mbsinit (&state));
          input[3] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 4, 1, &state);
          assert (ret == 1);
          assert (wc == 'r');
          assert (mbsinit (&state));
        }
        break;

      case '2':
        /* Locale encoding is UTF-8.  */
      	printf("UTF-8 ...\n");
        {
          char input[] = "B\303\274\303\237er"; /* "Büßer" */
          memset (&state, '\0', sizeof (mbstate_t));

		  if (setlocale (LC_ALL, "en_US.UTF-8") == NULL) {
			  fprintf(stderr, "unable to set UTF-8 locale, skipping\n");
			  break;
		  }

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input, 1, &state);
          assert (ret == 1);
          assert (wc == 'B');
          assert (mbsinit (&state));
          input[0] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 1, 1, &state);
          assert (ret == (size_t)(-2));
          assert (wc == (wchar_t) 0xBADFACE);
          assert (!mbsinit (&state));
          input[1] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 2, 5, &state);
          assert (ret == 1);
          assert (wctob (wc) == EOF);
          assert (mbsinit (&state));
          input[2] = '\0';

          /* Test support of NULL first argument.  */
          ret = mbrtowc (NULL, input + 3, 4, &state);
          assert (ret == 2);
          assert (mbsinit (&state));

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 3, 4, &state);
          assert (ret == 2);
          assert (wctob (wc) == EOF);
          assert (mbsinit (&state));
          input[3] = '\0';
          input[4] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 5, 2, &state);
          assert (ret == 1);
          assert (wc == 'e');
          assert (mbsinit (&state));
          input[5] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 6, 1, &state);
          assert (ret == 1);
          assert (wc == 'r');
          assert (mbsinit (&state));
        }
        break;

      case '3':
        /* Locale encoding is EUC-JP.  */
       	printf("EUC-JP ...\n");
        {
          char input[] = "<\306\374\313\334\270\354>"; /* "<日本語>" */
          memset (&state, '\0', sizeof (mbstate_t));

		  if (setlocale (LC_ALL, "en_US.EUC-JP") == NULL) {
			  fprintf(stderr, "unable to set EUC-JP locale, skipping\n");
			  break;
		  }

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input, 1, &state);
          assert (ret == 1);
          assert (wc == '<');
          assert (mbsinit (&state));
          input[0] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 1, 2, &state);
          assert (ret == 2);
          assert (wctob (wc) == EOF);
          assert (mbsinit (&state));
          input[1] = '\0';
          input[2] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 3, 1, &state);
          assert (ret == (size_t)(-2));
          assert (wc == (wchar_t) 0xBADFACE);
          assert (!mbsinit (&state));
          input[3] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 4, 4, &state);
          assert (ret == 1);
          assert (wctob (wc) == EOF);
          assert (mbsinit (&state));
          input[4] = '\0';

          /* Test support of NULL first argument.  */
          ret = mbrtowc (NULL, input + 5, 3, &state);
          assert (ret == 2);
          assert (mbsinit (&state));

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 5, 3, &state);
          assert (ret == 2);
          assert (wctob (wc) == EOF);
          assert (mbsinit (&state));
          input[5] = '\0';
          input[6] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 7, 1, &state);
          assert (ret == 1);
          assert (wc == '>');
          assert (mbsinit (&state));
        }
        break;

      case '4':
        /* Locale encoding is GB18030.  */
       	printf("GB18030 ...\n");
        {
          char input[] = "B\250\271\201\060\211\070er"; /* "Büßer" */
          memset (&state, '\0', sizeof (mbstate_t));

		  if (setlocale (LC_ALL, "en_US.GB18030") == NULL) {
			  fprintf(stderr, "unable to set GB18030 locale, skipping\n");
			  break;
		  }

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input, 1, &state);
          assert (ret == 1);
          assert (wc == 'B');
          assert (mbsinit (&state));
          input[0] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 1, 1, &state);
          assert (ret == (size_t)(-2));
          assert (wc == (wchar_t) 0xBADFACE);
          assert (!mbsinit (&state));
          input[1] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 2, 7, &state);
          assert (ret == 1);
          assert (wctob (wc) == EOF);
          assert (mbsinit (&state));
          input[2] = '\0';

          /* Test support of NULL first argument.  */
          ret = mbrtowc (NULL, input + 3, 6, &state);
          assert (ret == 4);
          assert (mbsinit (&state));

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 3, 6, &state);
          assert (ret == 4);
          assert (wctob (wc) == EOF);
          assert (mbsinit (&state));
          input[3] = '\0';
          input[4] = '\0';
          input[5] = '\0';
          input[6] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 7, 2, &state);
          assert (ret == 1);
          assert (wc == 'e');
          assert (mbsinit (&state));
          input[5] = '\0';

          wc = (wchar_t) 0xBADFACE;
          ret = mbrtowc (&wc, input + 8, 1, &state);
          assert (ret == 1);
          assert (wc == 'r');
          assert (mbsinit (&state));
        }
        break;
      }
  }

  return 0;
}
