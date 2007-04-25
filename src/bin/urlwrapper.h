/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 */

/*
 * urlwrapper: compile-time configuration of supported protocols.
 */

/* 
 * comment out to disable handling a specific protocol
 */

/* file: redirects to Tracker */
#define HANDLE_FILE

/* http: downloads with wget in a Terminal */
#define HANDLE_HTTP_WGET

/* query: BeOS/Haiku-specific: this should allow putting queries in web pages :) */
#define HANDLE_QUERY

/* mid: cid: as per RFC 2392 */
/* http://www.rfc-editor.org/rfc/rfc2392.txt query MAIL:cid */
/* UNIMPLEMENTED */
//#define HANDLE_MID_CID

/* sh: executes a shell command (before warning user of danger) */
#define HANDLE_SH

/* beshare: optionaly connect to a server and start a query */
#define HANDLE_BESHARE

/* icq: msn: ... should open im_client to this user */
/* UNIMPLEMENTED */
//#define HANDLE_IM

/* mms: rtp: rtsp: opens the stream with VLC */
#define HANDLE_VLC

/* audio: redirects SoundPlay-urls for shoutcast streams */
/* UNIMPLEMENTED */
//#define HANDLE_AUDIO
