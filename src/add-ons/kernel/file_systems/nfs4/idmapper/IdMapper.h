/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef IDMAPPER_H
#define IDMAPPER_H


enum MsgCode {
	MsgError,
	MsgReply,
	MsgNameToUID,
	MsgUIDToName,
	MsgNameToGID,
	MsgGIDToName
};

static const char*	kRequestPortName	= "nfs4idmap_request";
static const char*	kReplyPortName		= "nfs4idmap_reply";


#endif	// IDMAPPER_H

