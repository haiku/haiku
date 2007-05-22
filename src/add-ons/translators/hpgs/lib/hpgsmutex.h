/***********************************************************************
 *                                                                     *
 * $Id: hpgsmutex.h 286 2006-03-03 15:14:14Z softadm $
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
 * Private header file for thread mutex support.                       *
 *                                                                     *
 ***********************************************************************/

#ifndef __HPGS_MUTEX_H__
#define __HPGS_MUTEX_H__

#ifndef	__HPGS_H
#include <hpgs.h>
#endif

#ifdef WIN32
#include <windows.h>
typedef CRITICAL_SECTION hpgs_mutex_t;
#else
#include <pthread.h>
typedef pthread_mutex_t hpgs_mutex_t;
#endif

static void hpgs_mutex_init(hpgs_mutex_t *m);
static void hpgs_mutex_destroy(hpgs_mutex_t *m);
static void hpgs_mutex_lock(hpgs_mutex_t *m);
static void hpgs_mutex_unlock(hpgs_mutex_t *m);

#ifdef WIN32

__inline__ void hpgs_mutex_init(hpgs_mutex_t *m)
{ InitializeCriticalSection(m); }

__inline__ void hpgs_mutex_destroy(hpgs_mutex_t *m)
{ DeleteCriticalSection(m); }

__inline__ void hpgs_mutex_lock(hpgs_mutex_t *m)
{ EnterCriticalSection(m); }

__inline__ void hpgs_mutex_unlock(hpgs_mutex_t *m)
{ LeaveCriticalSection(m); }

#else

__inline__ void hpgs_mutex_init(hpgs_mutex_t *m)
{ pthread_mutex_init(m,0); }

__inline__ void hpgs_mutex_destroy(hpgs_mutex_t *m)
{ pthread_mutex_destroy(m); }

__inline__ void hpgs_mutex_lock(hpgs_mutex_t *m)
{ pthread_mutex_lock(m); }

__inline__ void hpgs_mutex_unlock(hpgs_mutex_t *m)
{ pthread_mutex_unlock(m); }

#endif

#endif
