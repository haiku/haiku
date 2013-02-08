/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		François Revol <revol@free.fr>
 */
#ifndef WEB_HANDLER_H
#define WEB_HANDLER_H

#include <OS.h>
#include <String.h>
#include <SupportDefs.h>

class BNetEndpoint;
class StreamingRingBuffer;
class BDataIO;
class WebWorker;

class WebHandler {
public:
								WebHandler(const char *path,
									BDataIO *data=NULL);
								WebHandler(const char *path,
									StreamingRingBuffer *target);
		virtual					~WebHandler();

		void					SetMultipart(bool multipart=true) {
									fMultipart = true; }
		bool					IsMultipart() { return fMultipart; }
		void					SetType(const char *type) { fType = type; }


		BString &				Name() { return fPath; }
//		BNetEndpoint *			Endpoint() { return fEndpoint; }
virtual status_t				HandleRequest(WebWorker *worker,
									BString &path);

static	int						_CallbackCompare(const BString* key,
									const WebHandler* info);
static	int						_CallbackCompare(const WebHandler* a,
									const WebHandler* b);

private:
		friend class WebWorker;
		BString					fPath;

		bool					fMultipart;
		BString					fType;	// MIME
		BDataIO *				fData;
		StreamingRingBuffer *	fTarget;

		thread_id				fReceiverThread;
		bool					fStopThread;
};

#endif // WEB_HANDLER_H
