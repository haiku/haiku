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
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.
**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
    \file libmatroska.h
    \version \$Id: libmatroska.h,v 1.2 2004/04/14 23:26:17 robux4 Exp $
    \author Steve Lhomme     <robux4 @ users.sf.net>
    \author Ingo Ralf Blum   <ingoralfblum @ users.sf.net>

    \brief C API to the libmatroska library
    \note These are the functions that should be exported (visible from outisde the library)
    \todo Put a function here for all the MUST in the Matroska spec
    \todo Put a brief description of each function, and a description of the params and return value
    \todo Change the int values to sized types
*/

#ifndef _LIBMATROSKA_H_INCLUDED_
#define _LIBMATROSKA_H_INCLUDED_

#include "libmatroska_t.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef OLD

/*!
    \fn int matroska_plug_log(matroska_error_callback)
    \brief Attach a callback to be informed when error occurs
    \param callback The callback that will be called when logging occurs \return 0 if successfull
*/
int matroska_plug_log(matroska_error_callback callback);

/*!
    \fn int matroska_unplug_log(matroska_error_callback)
    \brief Unattach an attached callback to be informed when error occurs
    \param callback The callback that was called when logging occurs \return 0 if successfull
*/
int matroska_unplug_log(matroska_error_callback callback);

/*!
    \fn matroska_id matroska_open_file(c_string,matroska_file_mode)
    \brief Open an instance of an Matroska file
    \param string The name of the file to open (including OS depedant path) \param mode The mode to open the file (read, write, etc)
    \return NULL if the opening failed or an ID that will be used to access this file from the API
*/
matroska_stream MATROSKA_EXPORT matroska_open_stream_file(c_string string, open_mode mode);

matroska_id MATROSKA_EXPORT matroska_open_stream(matroska_stream a_stream);

/*!
    \fn matroska_id matroska_open_url(c_string)
    \brief Open an instance of an Matroska file from a URL
    \param string The name of the URL to open \return NULL if the opening failed or an ID that will be used to access this file from the API
    \warning Open only for reading ?
    \note It requires that Curl is compiled or installed
*/
matroska_id matroska_open_url(c_string string);

/*!
    \fn int matroska_close(matroska_id)
    \brief Close the specified Matroska instance
    \param id The instance to close \return 0 if successfull
*/
void MATROSKA_EXPORT matroska_close(matroska_id id);

void MATROSKA_EXPORT matroska_end(matroska_id id, uint32 totaltime);

matroska_track MATROSKA_EXPORT matroska_create_track(matroska_id id, enum track_type type);

void MATROSKA_EXPORT matroska_read_head(matroska_id id);
void MATROSKA_EXPORT matroska_read_tracks(matroska_id id);

uint8 MATROSKA_EXPORT matroska_get_number_track(matroska_id id);

matroska_track MATROSKA_EXPORT matroska_get_track(matroska_id id, uint8 track_index);

void MATROSKA_EXPORT matroska_get_track_info(matroska_id id, matroska_track track, track_info * infos);

/*
int matroska_track_write_block(matroska_track, void* buffer, unsigned int size);
int matroska_track_close(matroska_track);
*/

#endif /* OLD */

#ifdef __cplusplus
}
#endif

#endif /* _LIBMATROSKA_H_INCLUDED_ */
