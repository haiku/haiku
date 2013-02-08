/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		François Revol <revol@free.fr>
 */

#include "WebHandler.h"

#include "StreamingRingBuffer.h"

#include <NetEndpoint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE(x...)			debug_printf("WebHandler: "x)
#define TRACE_ERROR(x...)	debug_printf("WebHandler: "x)


WebHandler::WebHandler(const char *path, BDataIO *data)
	:
	fPath(path),
	fMultipart(false),
	fType(""),
	fData(data),
	fTarget(NULL)
{
}


WebHandler::WebHandler(const char *path, StreamingRingBuffer *target)
	:
	fPath(path),
	fMultipart(false),
	fType(""),
	fData(NULL),
	fTarget(target)
{
}


WebHandler::~WebHandler()
{
	delete fData;
}


status_t
WebHandler::HandleRequest(WebWorker *worker, BString &path)
{
//	off_t	offset = 0LL;

	return B_OK;
}


int
WebHandler::_CallbackCompare(const BString* key,
	const WebHandler* info)
{
	int diff = strcmp(*key, info->fPath.String());
	TRACE("'%s' <> '%s' %d\n", key->String(), info->fPath.String(), diff);
	if (diff == 0)
		return 0;
	if (diff < 0)
		return -1;
	return 1;
//	return strcmp(*key, info->fPath.String());
}


int
WebHandler::_CallbackCompare(const WebHandler* a, const WebHandler* b)
{
	int diff = strcmp(a->fPath.String(), b->fPath.String());
	return diff;
//	return strcmp(*key, info->fPath.String());
}



