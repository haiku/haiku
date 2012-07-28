/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef _MIME_TYPES_H
#define _MIME_TYPES_H


namespace BPrivate {

#define B_FILE_MIMETYPE				"application/octet-stream"
#define B_DIR_MIMETYPE				"application/x-vnd.Be-directory"
#define B_VOLUME_MIMETYPE			"application/x-vnd.Be-volume"
#define B_QUERY_MIMETYPE			"application/x-vnd.Be-query"
#define B_QUERY_TEMPLATE_MIMETYPE	"application/x-vnd.Be-queryTemplate"
#define B_LINK_MIMETYPE				"application/x-vnd.Be-symlink"
#define B_ROOT_MIMETYPE				"application/x-vnd.Be-root"
#define B_BOOKMARK_MIMETYPE			"application/x-vnd.Be-bookmark"
#define B_PERSON_MIMETYPE			"application/x-person"

#define B_PRINTER_MIMETYPE			"application/x-vnd.Be.printer"
#define B_PRINTER_SPOOL_MIMETYPE	"application/x-vnd.Be.printer-spool"

#define kPlainTextMimeType			"text/plain"

#define kBitmapMimeType				"image/x-vnd.Be-bitmap"
#define kLargeIconType				"icon/large"
#define kMiniIconType				"icon/mini"

} // namespace BPrivate

using namespace BPrivate;

#endif	// _MIME_TYPES_H
