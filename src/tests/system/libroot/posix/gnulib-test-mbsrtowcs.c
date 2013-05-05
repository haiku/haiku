/* Test of conversion of string to wide string.
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


#define BUFSIZE 10


int main(int argc, char *argv[])
{
	mbstate_t state;
	wchar_t wc;
	size_t ret;
	int mode;
	wchar_t buf[BUFSIZE];
	const char *src;
	mbstate_t temp_state;

	printf("POSIX ...\n");
	{
		char input[] = "Buesser";
		char isoInput[] = "B\374\337er"; /* "Büßer" */

		memset(&state, '\0', sizeof(mbstate_t));

		{
			size_t i;
			for (i = 0; i < BUFSIZE; i++)
				buf[i] = (wchar_t) 0xBADFACE;
		}

		wc = (wchar_t) 0xBADFACE;
		ret = mbrtowc(&wc, input, 1, &state);
		assert(ret == 1);
		assert(wc == 'B');
		assert(mbsinit (&state));
		input[0] = '\0';

		wc = (wchar_t) 0xBADFACE;
		ret = mbrtowc(&wc, input + 1, 1, &state);
		assert(ret == 1);
		assert(wctob (wc) == (unsigned char) 'u');
		assert(mbsinit (&state));
		input[1] = '\0';

		src = input + 2;
		temp_state = state;
		ret = mbsrtowcs(NULL, &src, BUFSIZE, &temp_state);
		assert(ret == 5);
		assert(src == input + 2);
		assert(mbsinit (&state));

		src = input + 2;
		ret = mbsrtowcs(buf, &src, BUFSIZE, &state);
		assert(ret == 5);
		assert(src == NULL);
		assert(wctob (buf[0]) == (unsigned char) 'e');
		assert(buf[1] == 's');
		assert(buf[2] == 's');
		assert(buf[3] == 'e');
		assert(buf[4] == 'r');
		assert(buf[5] == 0);
		assert(buf[6] == (wchar_t) 0xBADFACE);
		assert(mbsinit (&state));

		src = isoInput;
		ret = mbsrtowcs(buf, &src, BUFSIZE, &state);
		assert(ret == (size_t)-1);
		assert(src == isoInput + 1);
	}

	/* configure should already have checked that the locale is supported.  */
	if (setlocale(LC_ALL, "") == NULL) {
		fprintf(stderr, "unable to set standard locale\n");
		return 1;
	}

	/* Test NUL byte input.  */
	{
		const char *src;

		memset(&state, '\0', sizeof(mbstate_t));

		src = "";
		ret = mbsrtowcs(NULL, &src, 0, &state);
		assert(ret == 0);
		assert(mbsinit (&state));

		src = "";
		ret = mbsrtowcs(NULL, &src, 1, &state);
		assert(ret == 0);
		assert(mbsinit (&state));

		wc = (wchar_t) 0xBADFACE;
		src = "";
		ret = mbsrtowcs(&wc, &src, 0, &state);
		assert(ret == 0);
		assert(wc == (wchar_t) 0xBADFACE);
		assert(mbsinit (&state));

		wc = (wchar_t) 0xBADFACE;
		src = "";
		ret = mbsrtowcs(&wc, &src, 1, &state);
		assert(ret == 0);
		assert(wc == 0);
		assert(mbsinit (&state));
	}

	for (mode = '1'; mode <= '4'; ++mode) {
		int unlimited;
		for (unlimited = 0; unlimited < 2; unlimited++) {
			{
				size_t i;
				for (i = 0; i < BUFSIZE; i++)
					buf[i] = (wchar_t) 0xBADFACE;
			}

			switch (mode) {
				case '1':
					/* Locale encoding is ISO-8859-1 or ISO-8859-15.  */
		    	printf("ISO8859-1 ...\n");
				{
					char input[] = "B\374\337er"; /* "Büßer" */
					memset(&state, '\0', sizeof(mbstate_t));

					if (setlocale (LC_ALL, "en_US.ISO8859-1") == NULL) {
						fprintf(stderr,
							"unable to set ISO8859-1 locale, skipping\n");
						break;
					}

					wc = (wchar_t) 0xBADFACE;
					ret = mbrtowc(&wc, input, 1, &state);
					assert(ret == 1);
					assert(wc == 'B');
					assert(mbsinit (&state));
					input[0] = '\0';

					wc = (wchar_t) 0xBADFACE;
					ret = mbrtowc(&wc, input + 1, 1, &state);
					assert(ret == 1);
					assert(wctob (wc) == (unsigned char) '\374');
					assert(mbsinit (&state));
					input[1] = '\0';

					src = input + 2;
					temp_state = state;
					ret = mbsrtowcs(NULL, &src, unlimited ? BUFSIZE : 1,
						&temp_state);
					assert(ret == 3);
					assert(src == input + 2);
					assert(mbsinit (&state));

					src = input + 2;
					ret = mbsrtowcs(buf, &src, unlimited ? BUFSIZE : 1, &state);
					assert(ret == (unlimited ? 3u : 1u));
					assert(src == (unlimited ? NULL : input + 3));
					assert(wctob (buf[0]) == (unsigned char) '\337');
					if (unlimited) {
						assert(buf[1] == 'e');
						assert(buf[2] == 'r');
						assert(buf[3] == 0);
						assert(buf[4] == (wchar_t) 0xBADFACE);
					} else
						assert(buf[1] == (wchar_t) 0xBADFACE);
					assert(mbsinit (&state));
				}
				break;

				case '2':
					/* Locale encoding is UTF-8.  */
		    	printf("UTF-8 ...\n");
				{
					char input[] = "B\303\274\303\237er"; /* "Büßer" */
					char isoInput[] = "B\374\337er"; /* "Büßer" */
					memset(&state, '\0', sizeof(mbstate_t));

					if (setlocale (LC_ALL, "en_US.UTF-8") == NULL) {
						fprintf(stderr,
							"unable to set UTF-8 locale, skipping\n");
						break;
					}

					wc = (wchar_t) 0xBADFACE;
					ret = mbrtowc(&wc, input, 1, &state);
					assert(ret == 1);
					assert(wc == 'B');
					assert(mbsinit (&state));
					input[0] = '\0';

					wc = (wchar_t) 0xBADFACE;
					ret = mbrtowc(&wc, input + 1, 1, &state);
					assert(ret == (size_t)(-2));
					assert(wc == (wchar_t) 0xBADFACE);
					assert(!mbsinit (&state));
					input[1] = '\0';

					src = input + 2;
					temp_state = state;
					ret = mbsrtowcs(NULL, &src, unlimited ? BUFSIZE : 2,
						&temp_state);
					assert(ret == 4);
					assert(src == input + 2);
					assert(!mbsinit (&state));

					src = input + 2;
					ret = mbsrtowcs(buf, &src, unlimited ? BUFSIZE : 2, &state);
					assert(ret == (unlimited ? 4u : 2u));
					assert(src == (unlimited ? NULL : input + 5));
					assert(wctob (buf[0]) == EOF);
					assert(wctob (buf[1]) == EOF);
					if (unlimited) {
						assert(buf[2] == 'e');
						assert(buf[3] == 'r');
						assert(buf[4] == 0);
						assert(buf[5] == (wchar_t) 0xBADFACE);
					} else
						assert(buf[2] == (wchar_t) 0xBADFACE);
					assert(mbsinit (&state));

					src = isoInput;
					ret = mbsrtowcs(buf, &src, BUFSIZE, &state);
					assert(ret == (size_t)-1);
					assert(src == isoInput + 1);
				}
				break;

				case '3':
					/* Locale encoding is EUC-JP.  */
		    	printf("EUC-JP ...\n");
				{
					char input[] = "<\306\374\313\334\270\354>"; /* "<日本語>" */
					memset(&state, '\0', sizeof(mbstate_t));

					if (setlocale (LC_ALL, "en_US.EUC-JP") == NULL) {
						fprintf(stderr,
							"unable to set EUC-JP locale, skipping\n");
						break;
					}

					wc = (wchar_t) 0xBADFACE;
					ret = mbrtowc(&wc, input, 1, &state);
					assert(ret == 1);
					assert(wc == '<');
					assert(mbsinit (&state));
					input[0] = '\0';

					wc = (wchar_t) 0xBADFACE;
					ret = mbrtowc(&wc, input + 1, 2, &state);
					assert(ret == 2);
					assert(wctob (wc) == EOF);
					assert(mbsinit (&state));
					input[1] = '\0';
					input[2] = '\0';

					wc = (wchar_t) 0xBADFACE;
					ret = mbrtowc(&wc, input + 3, 1, &state);
					assert(ret == (size_t)(-2));
					assert(wc == (wchar_t) 0xBADFACE);
					assert(!mbsinit (&state));
					input[3] = '\0';

					src = input + 4;
					temp_state = state;
					ret = mbsrtowcs(NULL, &src, unlimited ? BUFSIZE : 2,
						&temp_state);
					assert(ret == 3);
					assert(src == input + 4);
					assert(!mbsinit (&state));

					src = input + 4;
					ret = mbsrtowcs(buf, &src, unlimited ? BUFSIZE : 2, &state);
					assert(ret == (unlimited ? 3u : 2u));
					assert(src == (unlimited ? NULL : input + 7));
					assert(wctob (buf[0]) == EOF);
					assert(wctob (buf[1]) == EOF);
					if (unlimited) {
						assert(buf[2] == '>');
						assert(buf[3] == 0);
						assert(buf[4] == (wchar_t) 0xBADFACE);
					} else
						assert(buf[2] == (wchar_t) 0xBADFACE);
					assert(mbsinit (&state));
				}
				break;

				case '4':
					/* Locale encoding is GB18030.  */
		    	printf("GB18030 ...\n");
				{
					char input[] = "B\250\271\201\060\211\070er"; /* "Büßer" */
					memset(&state, '\0', sizeof(mbstate_t));

					if (setlocale (LC_ALL, "en_US.GB18030") == NULL) {
						fprintf(stderr,
							"unable to set GB18030 locale, skipping\n");
						break;
					}

					wc = (wchar_t) 0xBADFACE;
					ret = mbrtowc(&wc, input, 1, &state);
					assert(ret == 1);
					assert(wc == 'B');
					assert(mbsinit (&state));
					input[0] = '\0';

					wc = (wchar_t) 0xBADFACE;
					ret = mbrtowc(&wc, input + 1, 1, &state);
					assert(ret == (size_t)(-2));
					assert(wc == (wchar_t) 0xBADFACE);
					assert(!mbsinit (&state));
					input[1] = '\0';

					src = input + 2;
					temp_state = state;
					ret = mbsrtowcs(NULL, &src, unlimited ? BUFSIZE : 2,
						&temp_state);
					assert(ret == 4);
					assert(src == input + 2);
					assert(!mbsinit (&state));

					src = input + 2;
					ret = mbsrtowcs(buf, &src, unlimited ? BUFSIZE : 2, &state);
					assert(ret == (unlimited ? 4u : 2u));
					assert(src == (unlimited ? NULL : input + 7));
					assert(wctob (buf[0]) == EOF);
					assert(wctob (buf[1]) == EOF);
					if (unlimited) {
						assert(buf[2] == 'e');
						assert(buf[3] == 'r');
						assert(buf[4] == 0);
						assert(buf[5] == (wchar_t) 0xBADFACE);
					} else
						assert(buf[2] == (wchar_t) 0xBADFACE);
					assert(mbsinit (&state));
				}
				break;

				default:
					return 1;
			}
		}
	}

	return 0;
}
