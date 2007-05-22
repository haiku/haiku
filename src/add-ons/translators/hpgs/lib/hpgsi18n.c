/***********************************************************************
 *                                                                     *
 * $Id: hpgsi18n.c 270 2006-01-29 21:12:23Z softadm $
 *                                                                     *
 * hpgs - HPGl Script, a hpgl/2 interpreter, which uses a Postscript   *
 *        API for rendering a scene and thus renders to a variety of   *
 *        devices and fileformats.                                     *
 *                                                                     *
 * (C) 2004-2006 ev-i Informationstechnologie GmbH  http://www.ev-i.at *
 *                                                                     *
 * Author: Wolfgang Glas                                               *
 *                                                                     *
 *  hpgs is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU Lesser General Public          *
 * License as published by the Free Software Foundation; either        *
 * version 2.1 of the License, or (at your option) any later version.  *
 *                                                                     *
 * hpgs is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Lesser General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Lesser General Public    *
 * License along with this library; if not, write to the               *
 * Free Software  Foundation, Inc., 59 Temple Place, Suite 330,        *
 * Boston, MA  02111-1307  USA                                         *
 *                                                                     *
 ***********************************************************************
 *                                                                     *
 * Private subroutines used by subroutines of the library.             *
 *                                                                     *
 ***********************************************************************/

#include <hpgsi18n.h>

#ifdef HPGS_HAVE_GETTEXT

#define HPGS_I18N_DOMAIN "hpgs"

void hpgs_i18n_init()
{
  char *sp = hpgs_sprintf_malloc("%s%cshare%clocale",
                                 hpgs_get_prefix(),
                                 HPGS_PATH_SEPARATOR,
                                 HPGS_PATH_SEPARATOR);

  if (!sp) return;

  bindtextdomain(HPGS_I18N_DOMAIN,sp);
  free(sp);
}

const char *hpgs_i18n(const char *msg)
{
  return dgettext(HPGS_I18N_DOMAIN,msg);
}

const char *hpgs_i18n_n(const char *msg,
                        const char *msg_plural,
                        unsigned long n)
{
  return dngettext(HPGS_I18N_DOMAIN,msg,msg_plural,n);
}

#else

const char *hpgs_i18n(const char *msg)
{ return msg; }

const char *hpgs_i18n_n(const char *msg,
                        const char *msg_plural,
                        unsigned long n)
{ return n==1 ? msg : msg_plural; }

#endif // HPGS_HAVE_GETTEXT
