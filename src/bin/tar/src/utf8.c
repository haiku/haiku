/* Charset handling for GNU tar.

   Copyright (C) 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "system.h"
#include <quotearg.h>
#include "common.h"
#ifdef HAVE_ICONV_H
# include <iconv.h>
#endif

struct langtab
{
  char *lang;        /* Language code */
  char *terr;        /* Territory code */
  char *charset;     /* Corresponding charset */
};

/* The list of language codes defined in ISO 639 with the corresponding
   default character sets.

   NOTES:

   1) The list must be ordered by:
        a) lang field in ascending order
	b) terr field in descending order.
      NULL fields are considered less than non-null ones.
   2) Many entries have NULL charset fields. Please help fill them!
   3) The "default" character set for a given language is a matter
      of preference. Possibly the table should contain a *list* of
      possible character sets.
   4) LC_ALL "modifier" field is not taken into account */

static struct langtab langtab[] = {
  { "C",     NULL, "ASCII"},
  { "POSIX", NULL, "ASCII" },
  { "aa",    NULL, NULL},              /* Afar */
  { "ab",    NULL, NULL},              /* Abkhazian */
  { "ae",    NULL, NULL},              /* Avestan */
  { "af",    NULL, "iso-8859-1"},      /* Afrikaans */
  { "am",    NULL, "UTF-8"},           /* Amharic */
  { "ar",    NULL, "iso-8859-6"},      /* Arabic */
  { "as",    NULL, NULL},              /* Assamese */
  { "ay",    NULL, "iso-8859-1"},      /* Aymara */
  { "az",    NULL, NULL},              /* Azerbaijani */
  { "ba",    NULL, NULL},              /* Bashkir */
  { "be",    NULL, "UTF-8"},           /* Byelorussian; Belarusian */
  { "bg",    NULL, "iso-8859-5"},      /* Bulgarian */
  { "bh",    NULL, NULL},              /* Bihari */
  { "bi",    NULL, NULL},              /* Bislama */
  { "bn",    NULL, NULL},              /* Bengali; Bangla */
  { "bo",    NULL, NULL},              /* Tibetan */
  { "br",    NULL, "iso-8859-1"},      /* Breton: 1,5,8,9 */
  { "bs",    NULL, NULL},              /* Bosnian */
  { "ca",    NULL, "iso-8859-1"},      /* Catalan: 1,5,8,9 */
  { "ce",    NULL, NULL},              /* Chechen */
  { "ch",    NULL, NULL},              /* Chamorro */
  { "co",    NULL, "iso-8859-1"},      /* Corsican */
  { "cs",    NULL, "iso-8859-2"},      /* Czech */
  { "cu",    NULL, NULL },             /* Church Slavic */
  { "cv",    NULL, NULL},              /* Chuvash */
  { "cy",    NULL, "iso-8859-1"},      /* Welsh */
  { "da",    NULL, "iso-8859-1"},      /* Danish: 4-9 */
  { "de",    NULL, "iso-8859-1"},      /* German */
  { "dz",    NULL, NULL },             /* Dzongkha; Bhutani */
  { "el",    NULL, "iso-8859-7"},      /* Greek */
  { "en",    NULL, "iso-8859-1"},      /* English */
  { "eo",    NULL, "iso-8859-3"},      /* Esperanto */
  { "es",    NULL, "iso-8859-1"},      /* Spanish */
  { "et",    NULL, "iso-8859-15"},     /* Estonian: 6,7,9 */
  { "eu",    NULL, "iso-8859-1"},      /* Basque: 5,8,9 */
  { "fa",    NULL, "UTF-8"},           /* Persian */
  { "fi",    NULL, "iso-8859-15"},     /* Finnish */
  { "fj",    NULL, NULL },             /* Fijian; Fiji */
  { "fo",    NULL, "iso-8859-1"},      /* Faroese: 6,9 */
  { "fr",    NULL, "iso-8859-1"},      /* French */
  { "fy",    NULL, "iso-8859-1"},      /* Frisian */
  { "ga",    NULL, "iso-8859-14"},     /* Irish */
  { "gd",    NULL, "iso-8859-14" },    /* Scots; Gaelic */
  { "gl",    NULL, NULL },             /* Gallegan; Galician */
  { "gn",    NULL, NULL},              /* Guarani */
  { "gu",    NULL, NULL},              /* Gujarati */
  { "gv",    NULL, "iso-8859-14"},     /* Manx */
  { "ha",    NULL, NULL },             /* Hausa (?) */
  { "he",    NULL, "iso-8859-8" },     /* Hebrew */
  { "hi",    NULL, NULL},              /* Hindi */
  { "ho",    NULL, NULL},              /* Hiri Motu */
  { "hr",    NULL, "iso-8859-2"},      /* Croatian: 10 */
  { "hu",    NULL, "iso-8859-2"},      /* Hungarian */
  { "hy",    NULL, NULL},              /* Armenian */
  { "hz",    NULL, NULL},              /* Herero */
  { "id",    NULL, "iso-8859-1"},      /* Indonesian (formerly in) */
  { "ia",    NULL, NULL},              /* Interlingua */
  { "ie",    NULL, NULL},              /* Interlingue */
  { "ik",    NULL, NULL},              /* Inupiak */
  { "io",    NULL, NULL},              /* Ido */
  { "is",    NULL, "iso-8859-1"},      /* Icelandic */
  { "it",    NULL, "iso-8859-1"},      /* Italian */
  { "iu",    NULL, NULL},              /* Inuktitut */
  { "ja",    NULL, "EUC-JP"},          /* Japanese */
  { "jv",    NULL, NULL},              /* Javanese */
  { "ka",    NULL, NULL},              /* Georgian */
  { "ki",    NULL, NULL},              /* Kikuyu */
  { "kj",    NULL, NULL},              /* Kuanyama */
  { "kk",    NULL, NULL},              /* Kazakh */
  { "kl",    NULL, "iso-8859-1"},      /* Kalaallisut; Greenlandic */
  { "km",    NULL, NULL},              /* Khmer; Cambodian */
  { "kn",    NULL, NULL},              /* Kannada */
  { "ko",    NULL, "EUC-KR"},          /* Korean */
  { "ks",    NULL, NULL},              /* Kashmiri */
  { "ku",    NULL, NULL},              /* Kurdish */
  { "kv",    NULL, NULL},              /* Komi */
  { "kw",    NULL, "iso-8859-14"},     /* Cornish: 1,5,8 */
  { "ky",    NULL, NULL},              /* Kirghiz */
  { "la",    NULL, "iso-8859-1"},      /* Latin */
  { "lb",    NULL, "iso-8859-1"},      /* Letzeburgesch */
  { "ln",    NULL, NULL},              /* Lingala */
  { "lo",    NULL, NULL},              /* Lao; Laotian */
  { "lt",    NULL, "iso-8859-4"},      /* Lithuanian */
  { "lv",    NULL, "iso-8859-4"},      /* Latvian; Lettish */
  { "mg",    NULL, NULL},              /* Malagasy */
  { "mh",    NULL, NULL},              /* Marshall */
  { "mi",    NULL, NULL},              /* Maori */
  { "mk",    NULL, NULL},              /* Macedonian */
  { "ml",    NULL, NULL},              /* Malayalam */
  { "mn",    NULL, NULL},              /* Mongolian */
  { "mo",    NULL, "iso-8859-2"},      /* Moldavian */
  { "mr",    NULL, NULL},              /* Marathi */
  { "ms",    NULL, NULL},              /* Malay */
  { "mt",    NULL, "iso-8859-3"},      /* Maltese */
  { "my",    NULL, NULL},              /* Burmese */
  { "na",    NULL, NULL},              /* Nauru */
  { "nb",    NULL, "iso-8859-1"},      /* Norwegian Bokmål; Bokm@aa{}l  */
  { "nd",    NULL, NULL},              /* Ndebele, North */
  { "ne",    NULL, NULL},              /* Nepali */
  { "ng",    NULL, NULL},              /* Ndonga */
  { "nl",    NULL, "iso-8859-1"},      /* Dutch: 5,9 */
  { "nn",    NULL, "iso-8859-1"},      /* Norwegian Nynorsk */
  { "no",    NULL, "iso-8859-1"},      /* Norwegian */
  { "nr",    NULL, NULL},              /* Ndebele, South */
  { "nv",    NULL, NULL},              /* Navajo */
  { "ny",    NULL, NULL},              /* Chichewa; Nyanja */
  { "oc",    NULL, NULL},              /* Occitan; Provençal; Proven@,{c}al */
  { "om",    NULL, NULL},              /* (Afan) Oromo */
  { "or",    NULL, NULL},              /* Oriya */
  { "os",    NULL, NULL},              /* Ossetian; Ossetic */
  { "pa",    NULL, NULL},              /* Panjabi; Punjabi */
  { "pi",    NULL, NULL},              /* Pali */
  { "pl",    NULL, "iso-8859-2"},      /* Polish */
  { "ps",    NULL, NULL},              /* Pashto, Pushto */
  { "pt",    NULL, "iso-8859-1"},      /* Portuguese */
  { "qu",    NULL, "iso-8859-1"},      /* Quechua */
  { "rm",    NULL, "iso-8859-1"},      /* Rhaeto-Romance */
  { "rn",    NULL, NULL },             /* Rundi; Kirundi */
  { "ro",    NULL, "iso-8859-2"},      /* Romanian */
  { "ru",    NULL, "koi8-r"},          /* Russian */
  { "rw",    NULL, NULL},              /* Kinyarwanda */
  { "sa",    NULL, NULL},              /* Sanskrit */
  { "sc",    NULL, "iso-8859-1"},      /* Sardinian */
  { "sd",    NULL, NULL},              /* Sindhi */
  { "se",    NULL, "iso-8859-10"},     /* Northern Sami */
  { "sg",    NULL, NULL},              /* Sango; Sangro */
  { "si",    NULL, NULL},              /* Sinhalese */
  { "sk",    NULL, "iso-8859-2"},      /* Slovak */
  { "sl",    NULL, "iso-8859-1"},      /* Slovenian */
  { "sm",    NULL, NULL},              /* Samoan */
  { "sn",    NULL, NULL},              /* Shona */
  { "so",    NULL, NULL},              /* Somali */
  { "sq",    NULL, "iso-8859-1"},      /* Albanian: 2,5,8,9,10 */
  { "sr",    NULL, "iso-8859-2"},      /* Serbian */
  { "ss",    NULL, NULL},              /* Swati; Siswati */
  { "st",    NULL, NULL},              /* Sesotho; Sotho, Southern */
  { "su",    NULL, NULL},              /* Sundanese */
  { "sv",    NULL, "iso-8859-1"},      /* Swedish */
  { "sw",    NULL, NULL},              /* Swahili */
  { "ta",    NULL, NULL},              /* Tamil */
  { "te",    NULL, NULL},              /* Telugu */
  { "tg",    NULL, NULL},              /* Tajik */
  { "th",    NULL, "iso-8859-11"},     /* Thai */
  { "ti",    NULL, NULL},              /* Tigrinya */
  { "tk",    NULL, NULL},              /* Turkmen */
  { "tl",    NULL, "iso-8859-1"},      /* Tagalog */
  { "tn",    NULL, NULL},              /* Tswana; Setswana */
  { "to",    NULL, NULL},              /* Tonga (?) */
  { "tr",    NULL, "iso-8859-9"},      /* Turkish */
  { "ts",    NULL, NULL},              /* Tsonga */
  { "tt",    NULL, NULL},              /* Tatar */
  { "tw",    NULL, NULL},              /* Twi */
  { "ty",    NULL, NULL},              /* Tahitian */
  { "ug",    NULL, NULL},              /* Uighur */
  { "uk",    NULL, "koi8-u"},          /* Ukrainian */
  { "ur",    NULL, NULL},              /* Urdu */
  { "uz",    NULL, NULL},              /* Uzbek */
  { "vi",    NULL, NULL},              /* Vietnamese */
  { "vo",    NULL, NULL},              /* Volapük; Volap@"{u}k; Volapuk */
  { "wa",    NULL, "iso-8859-1"},      /* Walloon */
  { "wo",    NULL, NULL},              /* Wolof */
  { "xh",    NULL, NULL},              /* Xhosa */
  { "yi",    NULL, "iso-8859-8"},      /* Yiddish (formerly ji) */
  { "yo",    NULL, NULL},              /* Yoruba */
  { "za",    NULL, NULL},              /* Zhuang */
  { "zh",    "TW", "big5"},            /* Chinese */
  { "zh",    NULL, "gb2312"},          /* Chinese */
  { "zu",    NULL, NULL},              /* Zulu */
  { NULL }
};

/* Given the language and (optionally) territory code, return the
   default character set for that language. See notes above. */

const char *
charset_lookup (char *lang, char *terr)
{
  static struct langtab *p;

  if (!lang)
    return NULL;
  for (p = langtab; p->lang; p++)
    if (strcasecmp (p->lang, lang) == 0
	&& (terr == NULL 
	    || p->terr == NULL
	    || !strcasecmp (p->terr, terr) == 0))
      return p->charset;
  return NULL;
}

static const char *
get_input_charset ()
{
  const char *charset = NULL;
  char *tmp;

  /* Try to deduce the charset from LC_ALL or LANG variables */

  tmp = getenv ("LC_ALL");
  if (!tmp)
    tmp = getenv ("LANG");

  if (tmp)
    {
      char *lang;
      char *terr;

      lang = strtok (tmp, "_");
      terr = strtok (NULL, ".");
      charset = strtok (NULL, "@");

      if (!charset)
	charset = charset_lookup (lang, terr);
    }

  if (!charset)
    charset = "iso-8859-1";
  return charset;
}



#ifndef HAVE_LIBICONV

iconv_t
iconv_open (const char *tocode, const char *fromcode)
{
  return (iconv_t)(-1);
}

size_t
iconv (iconv_t cd, ICONV_CONST char **inbuf, size_t *inbytesleft,
       char **outbuf, size_t *outbytesleft)
{
  return 0;
}

int
iconv_close (iconv_t cd)
{
  return 0;
}

#endif /* !HAVE_LIBICONV */




static iconv_t conv_desc[2] = { (iconv_t) -1, (iconv_t) -1 };

static iconv_t
utf8_init (bool to_utf)
{
  if (conv_desc[(int) to_utf] == (iconv_t) -1)
    {
      if (to_utf)
	conv_desc[(int) to_utf] = iconv_open ("UTF-8", get_input_charset ());
      else
	conv_desc[(int) to_utf] = iconv_open (get_input_charset (), "UTF-8");
    }
  return conv_desc[(int) to_utf];
}
		  
bool
utf8_convert(bool to_utf, const char *input, char **output)
{
  const char *ib;
  char *ob;
  size_t inlen;
  size_t outlen;
  size_t rc;
  iconv_t cd = utf8_init (to_utf);

  if (cd == 0)
    {
      *output = xstrdup (input);
      return true;
    }
  else if (cd == (iconv_t)-1)
    return false;

  inlen = strlen (input) + 1;
  outlen = inlen * MB_LEN_MAX + 1;
  ob = *output = xmalloc (outlen);
  ib = input;
  rc = iconv (cd, &ib, &inlen, &ob, &outlen);
  *ob = 0;
  return rc != -1;
}


bool
string_ascii_p (const char *str)
{
  const unsigned char *p = (const unsigned char *)str;
  for (; *p; p++)
    if (*p > 127)
      return false;
  return true;
}
