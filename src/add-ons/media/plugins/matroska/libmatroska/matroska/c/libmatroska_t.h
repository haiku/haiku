/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2003 Steve Lhomme.  All rights reserved.
**
** This file is part of libmatroska.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
    \file libmatroska_t.h
    \version \$Id: libmatroska_t.h,v 1.3 2004/04/14 23:26:17 robux4 Exp $
    \author Steve Lhomme     <robux4 @ users.sf.net>
    \author Ingo Ralf Blum   <ingoralfblum @ users.sf.net>

    \brief Misc type definitions for the C API of libmatroska

    \note These types should be compiler/language independant (just platform dependant)
    \todo recover the sized types (uint16, int32, etc) here too (or maybe here only)
*/

#ifndef _LIBMATROSKA_T_H_INCLUDED_
#define _LIBMATROSKA_T_H_INCLUDED_

#include "ebml/c/libebml_t.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
    \enum track_type
*/
typedef enum track_type {
    track_video       = 0x01, ///< Rectangle-shaped non-transparent pictures aka video
    track_audio       = 0x02, ///< Anything you can hear
    track_complex     = 0x03, ///< Audio and video in same track, used by DV

    track_logo        = 0x10, ///< Overlay-pictures, displayed over video
    track_subtitle    = 0x11, ///< Text-subtitles. One track contains one language and only one track can be active (player-side configuration)
    track_buttons     = 0x12, ///< Buttons meta-infos.

    track_control     = 0x20  ///< Control-codes for menus and other stuff
} track_type;

/*!
    \enum matroska_error_t
    \brief a callback that the library use to inform of errors happening
    \note this should be used by the libmatroska internals
*/
typedef enum {
	error_null_pointer  ///< NULL pointer where something else is expected
} matroska_error_t;

typedef void *matroska_stream;

/*!
    \var void* matroska_id
    \brief UID used to access an Matroska file instance
*/
typedef void* matroska_id;
/*!
    \var void* matroska_track
    \brief UID used to access a track
*/
typedef void* matroska_track;
/*!
    \var char* c_string
    \brief C-String, ie a buffer with characters terminated by \0
*/
typedef char* c_string;
/*!
    \var unsigned int matroska_file_mode
    \brief A bit buffer, each bit representing a value for the file opening
    \todo replace the unsigned int by a sized type (8 or 16 bits)
*/
typedef char * matroska_file_mode;
/*!
    \var void (*matroska_error_callback)(matroska_error_t error_code, char* error_message)
    \brief a callback that the library use to inform of errors happening
*/
typedef void (*matroska_error_callback)(matroska_error_t error_code, char* error_message);

#ifdef __cplusplus
}
#endif

#endif /* _LIBMATROSKA_T_H_INCLUDED_ */
