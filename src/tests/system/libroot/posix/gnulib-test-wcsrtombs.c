/* Test of conversion of wide string to string.
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
#include <stdlib.h>
#include <string.h>
#include <wchar.h>


#define BUFSIZE 20


int
main (int argc, char *argv[])
{
  int mode;

  for (mode = '0'; mode <= '4'; ++mode)
    {
      wchar_t input[10];
      size_t n;
      const wchar_t *src;
      char buf[BUFSIZE];
      size_t ret;

      {
        size_t i;
        for (i = 0; i < BUFSIZE; i++)
          buf[i] = '_';
      }

      switch (mode)
        {
        case '0':
          /* Locale encoding is POSIX  */
          printf("POSIX ...\n");
          {
            const char original[] = "Buesser";

            ret = mbstowcs (input, original, 10);
            assert(ret == 7);

            for (n = 0; n < 10; n++)
              {
                src = input;
                ret = wcsrtombs (NULL, &src, n, NULL);
                assert(ret == 7);
                assert(src == input);

                src = input;
                ret = wcsrtombs (buf, &src, n, NULL);
                assert(ret == (n <= 7 ? n : 7));
                assert(src == (n <= 7 ? input + n : NULL));
                assert(memcmp (buf, original, ret) == 0);
                if (src == NULL)
                  assert(buf[ret] == '\0');
                assert(buf[ret + (src == NULL) + 0] == '_');
                assert(buf[ret + (src == NULL) + 1] == '_');
                assert(buf[ret + (src == NULL) + 2] == '_');
              }

            input[2] = 0xDEADBEEFul;
			src = input;
			ret = wcsrtombs(buf, &src, BUFSIZE, NULL);
			assert(ret == (size_t)-1);
			assert(src == input + 2);
          }
          break;

        case '1':
          /* Locale encoding is ISO-8859-1 or ISO-8859-15.  */
          printf("ISO8859-1 ...\n");
          {
            const char original[] = "B\374\337er"; /* "Büßer" */

       	    if (setlocale (LC_ALL, "en_US.ISO8859-1") == NULL)
       	      {
       		    fprintf(stderr, "unable to set ISO8859-1 locale, skipping\n");
       		    break;
       	      }

            ret = mbstowcs (input, original, 10);
            assert(ret == 5);

            for (n = 0; n < 10; n++)
              {
                src = input;
                ret = wcsrtombs (NULL, &src, n, NULL);
                assert(ret == 5);
                assert(src == input);

                src = input;
                ret = wcsrtombs (buf, &src, n, NULL);
                assert(ret == (n <= 5 ? n : 5));
                assert(src == (n <= 5 ? input + n : NULL));
                assert(memcmp (buf, original, ret) == 0);
                if (src == NULL)
                  assert(buf[ret] == '\0');
                assert(buf[ret + (src == NULL) + 0] == '_');
                assert(buf[ret + (src == NULL) + 1] == '_');
                assert(buf[ret + (src == NULL) + 2] == '_');
              }
          }
          break;

        case '2':
          /* Locale encoding is UTF-8.  */
          printf("UTF-8 ... \n");
          {
            const char original[] = "B\303\274\303\237er"; /* "Büßer" */

       	    if (setlocale (LC_ALL, "en_US.UTF-8") == NULL)
       	      {
       		    fprintf(stderr, "unable to set UTF-8 locale, skipping\n");
       		    break;
       	      }

            ret = mbstowcs (input, original, 10);
            assert(ret == 5);

            for (n = 0; n < 10; n++)
              {
                src = input;
                ret = wcsrtombs (NULL, &src, n, NULL);
                assert(ret == 7);
                assert(src == input);

                src = input;
                ret = wcsrtombs (buf, &src, n, NULL);
                assert(ret == (n < 1 ? n :
                                n < 3 ? 1 :
                                n < 5 ? 3 :
                                n <= 7 ? n : 7));
                assert(src == (n < 1 ? input + n :
                                n < 3 ? input + 1 :
                                n < 5 ? input + 2 :
                                n <= 7 ? input + (n - 2) : NULL));
                assert(memcmp (buf, original, ret) == 0);
                if (src == NULL)
                  assert(buf[ret] == '\0');
                assert(buf[ret + (src == NULL) + 0] == '_');
                assert(buf[ret + (src == NULL) + 1] == '_');
                assert(buf[ret + (src == NULL) + 2] == '_');
              }

            input[2] = 0xDEADBEEFul;
			src = input;
			ret = wcsrtombs(buf, &src, BUFSIZE, NULL);
			assert(ret == (size_t)-1);
			assert(src == input + 2);
          }
          break;

        case '3':
          /* Locale encoding is EUC-JP.  */
          printf("EUC-JP ... \n");
          {
            const char original[] = "<\306\374\313\334\270\354>"; /* "<日本語>" */

       	    if (setlocale (LC_ALL, "en_US.EUC-JP") == NULL)
       	      {
       		    fprintf(stderr, "unable to set EUC-JP locale, skipping\n");
       		    break;
       	      }

            ret = mbstowcs (input, original, 10);
            assert(ret == 5);

            for (n = 0; n < 10; n++)
              {
                src = input;
                ret = wcsrtombs (NULL, &src, n, NULL);
                assert(ret == 8);
                assert(src == input);

                src = input;
                ret = wcsrtombs (buf, &src, n, NULL);
                assert(ret == (n < 1 ? n :
                                n < 3 ? 1 :
                                n < 5 ? 3 :
                                n < 7 ? 5 :
                                n <= 8 ? n : 8));
                assert(src == (n < 1 ? input + n :
                                n < 3 ? input + 1 :
                                n < 5 ? input + 2 :
                                n < 7 ? input + 3 :
                                n <= 8 ? input + (n - 3) : NULL));
                assert(memcmp (buf, original, ret) == 0);
                if (src == NULL)
                  assert(buf[ret] == '\0');
                assert(buf[ret + (src == NULL) + 0] == '_');
                assert(buf[ret + (src == NULL) + 1] == '_');
                assert(buf[ret + (src == NULL) + 2] == '_');
              }
          }
          break;


        case '4':
          printf("GB18030 ... \n");
          /* Locale encoding is GB18030.  */
          {
            const char original[] = "B\250\271\201\060\211\070er"; /* "Büßer" */

       	    if (setlocale (LC_ALL, "en_US.GB18030") == NULL)
       	      {
       		    fprintf(stderr, "unable to set GB18030 locale, skipping\n");
       		    break;
       	      }

            ret = mbstowcs (input, original, 10);
            assert(ret == 5);

            for (n = 0; n < 10; n++)
              {
                src = input;
                ret = wcsrtombs (NULL, &src, n, NULL);
                assert(ret == 9);
                assert(src == input);

                src = input;
                ret = wcsrtombs (buf, &src, n, NULL);
                assert(ret == (n < 1 ? n :
                                n < 3 ? 1 :
                                n < 7 ? 3 :
                                n <= 9 ? n : 9));
                assert(src == (n < 1 ? input + n :
                                n < 3 ? input + 1 :
                                n < 7 ? input + 2 :
                                n <= 9 ? input + (n - 4) : NULL));
                assert(memcmp (buf, original, ret) == 0);
                if (src == NULL)
                  assert(buf[ret] == '\0');
                assert(buf[ret + (src == NULL) + 0] == '_');
                assert(buf[ret + (src == NULL) + 1] == '_');
                assert(buf[ret + (src == NULL) + 2] == '_');
              }
          }
          break;

        default:
          return 1;
        }
    }

  return 0;
}
