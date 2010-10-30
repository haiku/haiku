/*
 * "$Id: i18n.c,v 1.8 2009/04/11 19:05:12 rlk Exp $"
 *
 *   Internationalization functions for CUPS drivers.
 *
 *   Copyright 2008 Michael Sweet (mike@easysw.com)
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
 *   stp_i18n_load()   - Load a message catalog for a locale.
 *   stp_i18n_lookup() - Lookup a string in the message catalog...
 *   stp_i18n_printf() - Send a formatted string to stderr.
 *   stpi_unquote()    - Unquote characters in strings.
 */

/*
 * Include necessary files...
 */

#include "i18n.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <iconv.h>


/*
 * GNU gettext uses a simple .po file format:
 *
 *   # comment
 *   msgid "id"
 *   "optional continuation"
 *   msgstr "str"
 *   "optional continuation"
 *
 * Both the id and str strings use standard C quoting for special characters
 * like newline and the double quote character.
 */


/*
 * Cache structure...
 */

typedef struct stpi_i18n_s
{
  struct stpi_i18n_s	*next;		/* Next catalog */
  char			locale[6];	/* Locale */
  stp_string_list_t	*po;		/* Message catalog */
} stpi_i18n_t;


/*
 * Local functions...
 */

static void	stpi_unquote(char *s);


/*
 * Local globals...
 */

static stpi_i18n_t	*stpi_pocache = NULL;


/*
 * 'stp_i18n_load()' - Load a message catalog for a locale.
 */

const stp_string_list_t *		/* O - Message catalog */
stp_i18n_load(const char *locale)	/* I - Locale name */
{
  stp_string_list_t	*po;		/* Message catalog */
  char			ll_CC[6],	/* Locale ID */
			poname[1024];	/* .po filename */
  stpi_i18n_t		*pocache;	/* Current cache entry */
  FILE			*pofile;	/* .po file */
  const char		*stp_localedir;	/* STP_LOCALEDIR environment variable */
  char			line[4096],	/* Line buffer */
			*ptr,		/* Pointer into buffer */
			id[4096],	/* Translation ID */
			str[4096],	/* Translation string */
			utf8str[4096];	/* UTF-8 translation string */
  int			in_id,		/* Processing "id" string? */
			in_str,		/* Processing "str" string? */
			linenum;	/* Line number in .po file */
  iconv_t		ic;		/* Transcoder to UTF-8 */
  size_t		inbytes,	/* Number of input buffer bytes */
			outbytes;	/* Number of output buffer bytes */
  char			*inptr,		/* Pointer into input buffer */
			*outptr;	/* Pointer into output buffer */
  int			fuzzy = 0;	/* Fuzzy translation? */


  if (!locale)
    return (NULL);

 /*
  * See if the locale is already loaded...
  */

  for (pocache = stpi_pocache; pocache; pocache = pocache->next)
    if (!strcmp(locale, pocache->locale))
      return (pocache->po);

 /*
  * Find the message catalog for the given locale...
  */

  if ((stp_localedir = getenv("STP_LOCALEDIR")) == NULL)
    stp_localedir = PACKAGE_LOCALE_DIR;

  strncpy(ll_CC, locale, sizeof(ll_CC) - 1);
  ll_CC[sizeof(ll_CC) - 1] = '\0';

  if ((ptr = strchr(ll_CC, '.')) != NULL)
    *ptr = '\0';

  snprintf(poname, sizeof(poname), "%s/%s/gutenprint_%s.po", stp_localedir,
	   ll_CC, ll_CC);
  if (access(poname, 0) && strlen(ll_CC) > 2)
  {
    ll_CC[2] = '\0';

    snprintf(poname, sizeof(poname), "%s/%s/gutenprint_%s.po", stp_localedir,
             ll_CC, ll_CC);
  }

  if ((pofile = fopen(poname, "rb")) == NULL)
    return (NULL);

 /*
  * Read the messages and add them to a string list...
  */

  if ((po = stp_string_list_create()) == NULL)
  {
    fclose(pofile);
    return (NULL);
  }

  linenum = 0;
  id[0]   = '\0';
  str[0]  = '\0';
  in_id   = 0;
  in_str  = 0;
  ic      = 0;

  while (fgets(line, sizeof(line), pofile))
  {
    linenum ++;

   /*
    * Skip blank and comment lines...
    */

    if (line[0] == '#')
    {
      if (line[1] == ':')
        fuzzy = 0;

      if (strstr(line, "fuzzy"))
        fuzzy = 1;
    }

    if (fuzzy || line[0] == '#' || line[0] == '\n')
      continue;

   /*
    * Strip the trailing quote...
    */

    if ((ptr = (char *)strrchr(line, '\"')) == NULL)
    {
      fprintf(stderr, "DEBUG: Expected quoted string on line %d of %s!\n",
	      linenum, poname);
      break;
    }

    *ptr = '\0';

   /*
    * Find start of value...
    */

    if ((ptr = strchr(line, '\"')) == NULL)
    {
      fprintf(stderr, "DEBUG: Expected quoted string on line %d of %s!\n",
	      linenum, poname);
      break;
    }

    ptr ++;

   /*
    * Create or add to a message...
    */

    if (!strncmp(line, "msgid", 5))
    {
      in_id  = 1;
      in_str = 0;

      if (id[0] && str[0])
      {
        stpi_unquote(id);

        if (ic)
	{
	 /*
	  * Convert string to UTF-8...
	  */

	  inbytes  = strlen(str);
	  inptr    = str;
	  outbytes = sizeof(utf8str);
	  outptr   = utf8str;

          iconv(ic, &inptr, &inbytes, &outptr, &outbytes);
	  *outptr = '\0';

	 /*
	  * Add it to the string list...
	  */

          stpi_unquote(utf8str);
          stp_string_list_add_string(po, id, utf8str);
	}
	else
	{
          stpi_unquote(str);
          stp_string_list_add_string(po, id, str);
        }
      }
      else if (!id[0] && str[0] && !ic)
      {
       /*
        * Look for the character set...
	*/

	const char	*charset = strstr(str, "charset=");
					/* Source character set definition */
	char		fromcode[255],	/* Source character set */
			*fromptr;	/* Pointer into fromcode */

	if (charset)
	{
	 /*
	  * Extract character set and setup a transcode context...
	  */

	  strncpy(fromcode, charset + 8, sizeof(fromcode) - 1);
	  fromcode[sizeof(fromcode) - 1] = '\0';
          for (fromptr = fromcode; *fromptr; fromptr ++)
	    if (!isalnum(*fromptr & 255) && *fromptr != '-')
	      break;
          *fromptr = '\0';

          if (strcasecmp(fromcode, "utf-8"))
	  {
            if ((ic = iconv_open("UTF-8", fromcode)) == (iconv_t)-1)
	    {
	      fprintf(stderr,
	              "DEBUG: Unable to convert character set \"%s\": %s\n",
	              fromcode, strerror(errno));
	      ic = 0;
	    }
	  }
        }
      }

      strncpy(id, ptr, sizeof(id) - 1);
      id[sizeof(id) - 1] = '\0';
      str[0] = '\0';
    }
    else if (!strncmp(line, "msgstr", 6))
    {
      in_id  = 0;
      in_str = 1;

      strncpy(str, ptr, sizeof(str) - 1);
      str[sizeof(str) - 1] = '\0';
    }
    else if (line[0] == '\"' && in_str)
    {
      int	str_len = strlen(str),
		ptr_len = strlen(ptr);


      if ((str_len + ptr_len + 1) > sizeof(str))
        ptr_len = sizeof(str) - str_len - 1;

      if (ptr_len > 0)
      {
        memcpy(str + str_len, ptr, ptr_len);
	str[str_len + ptr_len] = '\0';
      }
    }
    else if (line[0] == '\"' && in_id)
    {
      int	id_len = strlen(id),
		ptr_len = strlen(ptr);


      if ((id_len + ptr_len + 1) > sizeof(id))
        ptr_len = sizeof(id) - id_len - 1;

      if (ptr_len > 0)
      {
        memcpy(id + id_len, ptr, ptr_len);
	id[id_len + ptr_len] = '\0';
      }
    }
    else
    {
      fprintf(stderr, "DEBUG: Unexpected text on line %d of %s!\n",
	      linenum, poname);
      break;
    }
  }

  if (id[0] && str[0])
  {
    stpi_unquote(id);

    if (ic)
    {
     /*
      * Convert string to UTF-8...
      */

      inbytes  = strlen(str);
      inptr    = str;
      outbytes = sizeof(utf8str);
      outptr   = utf8str;

      iconv(ic, &inptr, &inbytes, &outptr, &outbytes);
      *outptr = '\0';

     /*
      * Add it to the string list...
      */

      stpi_unquote(utf8str);
      stp_string_list_add_string(po, id, utf8str);
    }
    else
    {
      stpi_unquote(str);
      stp_string_list_add_string(po, id, str);
    }
  }

  fclose(pofile);

 /*
  * Add this to the cache...
  */

  if ((pocache = calloc(1, sizeof(stpi_i18n_t))) != NULL)
  {
    strncpy(pocache->locale, locale, sizeof(pocache->locale) - 1);
    pocache->po   = po;
    pocache->next = stpi_pocache;
    stpi_pocache  = pocache;
  }
  
  if (ic)
    iconv_close(ic);
  return (po);
}


/*
 * 'stp_i18n_lookup()' - Lookup a string in the message catalog...
 */

const char *				/* O - Localized message */
stp_i18n_lookup(
    const stp_string_list_t *po,		/* I - Message catalog */
    const char        *message)		/* I - Message */
{
  stp_param_string_t	*param;		/* Matching message */


  if (po && (param = stp_string_list_find(po, message)) != NULL && param->text)
    return (param->text);
  else
    return (message);
}


/*
 * 'stp_i18n_printf()' - Send a formatted string to stderr.
 */

void
stp_i18n_printf(
    const stp_string_list_t *po,		/* I - Message catalog */
    const char        *message,		/* I - Printf-style message */
    ...)				/* I - Additional arguments as needed */
{
  va_list	ap;			/* Argument pointer */


  va_start(ap, message);
  vfprintf(stderr, stp_i18n_lookup(po, message), ap);
  va_end(ap);
}


/*
 * 'stpi_unquote()' - Unquote characters in strings.
 */

static void
stpi_unquote(char *s)		/* IO - Original string */
{
  char	*d = s;			/* Destination pointer */


  while (*s)
  {
    if (*s == '\\')
    {
      s ++;
      if (isdigit(*s))
      {
	*d = 0;

	while (isdigit(*s))
	{
	  *d = *d * 8 + *s - '0';
	  s ++;
	}

	d ++;
      }
      else
      {
	if (*s == 'n')
	  *d ++ = '\n';
	else if (*s == 'r')
	  *d ++ = '\r';
	else if (*s == 't')
	  *d ++ = '\t';
	else
	  *d++ = *s;

	s ++;
      }
    }
    else
      *d++ = *s++;
  }

  *d = '\0';
}


/*
 * End of "$Id: i18n.c,v 1.8 2009/04/11 19:05:12 rlk Exp $".
 */
