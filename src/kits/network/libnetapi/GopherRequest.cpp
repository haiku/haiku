/*
 * Copyright 2013-2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol, revol@free.fr
 */


#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include <Directory.h>
#include <DynamicBuffer.h>
#include <File.h>
#include <GopherRequest.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Socket.h>
#include <StackOrHeapArray.h>
#include <String.h>
#include <StringList.h>

/*
 * TODO: fix '+' in selectors, cf. gopher://gophernicus.org/1/doc/gopher/
 * TODO: add proper favicon
 * TODO: add proper dir and document icons
 * TODO: correctly eat the extraneous .\r\n at end of text files
 * TODO: move parsing stuff to a translator?
 *
 * docs:
 * gopher://gopher.floodgap.com/1/gopher/tech
 * gopher://gopher.floodgap.com/0/overbite/dbrowse?pluginm%201
 *
 * tests:
 * gopher://sdf.org/1/sdf/historical	images
 * gopher://gopher.r-36.net/1/	large photos
 * gopher://sdf.org/1/sdf/classes	binaries
 * gopher://sdf.org/1/users/	long page
 * gopher://jgw.mdns.org/1/	search items
 * gopher://jgw.mdns.org/1/MISC/	's' item (sound)
 * gopher://gopher.floodgap.com/1/gopher	broken link
 * gopher://sdf.org/1/maps/m	missing lines
 * gopher://sdf.org/1/foo	gophernicus reports errors incorrectly
 * gopher://gopher.floodgap.com/1/foo	correct error report
 */

/** Type of Gopher items */
typedef enum {
	GOPHER_TYPE_NONE	= 0,	/**< none set */
	GOPHER_TYPE_ENDOFPAGE	= '.',	/**< a dot alone on a line */
	/* these come from http://tools.ietf.org/html/rfc1436 */
	GOPHER_TYPE_TEXTPLAIN	= '0',	/**< text/plain */
	GOPHER_TYPE_DIRECTORY	= '1',	/**< gopher directory */
	GOPHER_TYPE_CSO_SEARCH	= '2',	/**< CSO search */
	GOPHER_TYPE_ERROR	= '3',	/**< error message */
	GOPHER_TYPE_BINHEX	= '4',	/**< binhex encoded text */
	GOPHER_TYPE_BINARCHIVE	= '5',	/**< binary archive file */
	GOPHER_TYPE_UUENCODED	= '6',	/**< uuencoded text */
	GOPHER_TYPE_QUERY	= '7',	/**< gopher search query */
	GOPHER_TYPE_TELNET	= '8',	/**< telnet link */
	GOPHER_TYPE_BINARY	= '9',	/**< generic binary */
	GOPHER_TYPE_DUPSERV	= '+',	/**< duplicated server */
	GOPHER_TYPE_GIF		= 'g',	/**< GIF image */
	GOPHER_TYPE_IMAGE	= 'I',	/**< image (depends, usually jpeg) */
	GOPHER_TYPE_TN3270	= 'T',	/**< tn3270 session */
	/* not standardized but widely used,
	 * cf. http://en.wikipedia.org/wiki/Gopher_%28protocol%29#Gopher_item_types
	 */
	GOPHER_TYPE_HTML	= 'h',	/**< HTML file or URL */
	GOPHER_TYPE_INFO	= 'i',	/**< information text */
	GOPHER_TYPE_AUDIO	= 's',	/**< audio (wav?) */
	/* not standardized, some servers use them */
	GOPHER_TYPE_DOC		= 'd',	/**< gophernicus uses it for PS and PDF */
	GOPHER_TYPE_PNG		= 'p',	/**< PNG image */
		/* cf. gopher://namcub.accelera-labs.com/1/pics */
	GOPHER_TYPE_MIME	= 'M',	/**< multipart/mixed MIME data */
		/* cf. http://www.pms.ifi.lmu.de/mitarbeiter/ohlbach/multimedia/IT/IBMtutorial/3376c61.html */
	/* cf. http://nofixedpoint.motd.org/2011/02/22/an-introduction-to-the-gopher-protocol/ */
	GOPHER_TYPE_PDF		= 'P',	/**< PDF file */
	GOPHER_TYPE_BITMAP	= ':',	/**< Bitmap image (Gopher+) */
	GOPHER_TYPE_MOVIE	= ';',	/**< Movie (Gopher+) */
	GOPHER_TYPE_SOUND	= '<',	/**< Sound (Gopher+) */
	GOPHER_TYPE_CALENDAR	= 'c',	/**< Calendar */
	GOPHER_TYPE_EVENT	= 'e',	/**< Event */
	GOPHER_TYPE_MBOX	= 'm',	/**< mbox file */
} gopher_item_type;

/** Types of fields in a line */
typedef enum {
	FIELD_NAME,
	FIELD_SELECTOR,
	FIELD_HOST,
	FIELD_PORT,
	FIELD_GPFLAG,
	FIELD_EOL,
	FIELD_COUNT = FIELD_EOL
} gopher_field;

/** Map of gopher types to MIME types */
static struct {
	gopher_item_type type;
	const char *mime;
} gopher_type_map[] = {
	/* these come from http://tools.ietf.org/html/rfc1436 */
	{ GOPHER_TYPE_TEXTPLAIN, "text/plain" },
	{ GOPHER_TYPE_DIRECTORY, "text/html;charset=UTF-8" },
	{ GOPHER_TYPE_QUERY, "text/html;charset=UTF-8" },
	{ GOPHER_TYPE_GIF, "image/gif" },
	{ GOPHER_TYPE_HTML, "text/html" },
	/* those are not standardized */
	{ GOPHER_TYPE_PDF, "application/pdf" },
	{ GOPHER_TYPE_PNG, "image/png"},
	{ GOPHER_TYPE_NONE, NULL }
};

static const char *kStyleSheet = "\n"
"/*\n"
" * gopher listing style\n"
" */\n"
"\n"
"body#gopher {\n"
"	/* margin: 10px;*/\n"
"	background-color: Window;\n"
"	color: WindowText;\n"
"	font-size: 100%;\n"
"	padding-bottom: 2em; }\n"
"\n"
"body#gopher div.uplink {\n"
"	padding: 0;\n"
"	margin: 0;\n"
"	position: fixed;\n"
"	top: 5px;\n"
"	right: 5px; }\n"
"\n"
"body#gopher h1 {\n"
"	padding: 5mm;\n"
"	margin: 0;\n"
"	border-bottom: 2px solid #777; }\n"
"\n"
"body#gopher span {\n"
"	margin-left: 1em;\n"
"	padding-left: 2em;\n"
"	font-family: 'Noto Mono', Courier, monospace;\n"
"	word-wrap: break-word;\n"
"	white-space: pre-wrap; }\n"
"\n"
"body#gopher span.error {\n"
"	color: #f00; }\n"
"\n"
"body#gopher span.unknown {\n"
"	color: #800; }\n"
"\n"
"body#gopher span.dir {\n"
"	background-image: url('resource:icons/directory.png');\n"
"	background-repeat: no-repeat;\n"
"	background-position: bottom left; }\n"
"\n"
"body#gopher span.text {\n"
"	background-image: url('resource:icons/content.png');\n"
"	background-repeat: no-repeat;\n"
"	background-position: bottom left; }\n"
"\n"
"body#gopher span.query {\n"
"	background-image: url('resource:icons/search.png');\n"
"	background-repeat: no-repeat;\n"
"	background-position: bottom left; }\n"
"\n"
"body#gopher span.img img {\n"
"	display: block;\n"
"	margin-left:auto;\n"
"	margin-right:auto; }\n";

static const int32 kGopherBufferSize = 4096;

static const bool kInlineImages = true;


BGopherRequest::BGopherRequest(const BUrl& url, BUrlProtocolListener* listener,
	BUrlContext* context)
	:
	BNetworkRequest(url, listener, context, "BUrlProtocol.Gopher", "gopher"),
	fItemType(GOPHER_TYPE_NONE),
	fPosition(0),
	fResult()
{
	fSocket = new(std::nothrow) BSocket();

	fUrl.UrlDecode();
	// the first part of the path is actually the document type

	fPath = Url().Path();
	if (!Url().HasPath() || fPath.Length() == 0 || fPath == "/") {
		// default entry
		fItemType = GOPHER_TYPE_DIRECTORY;
		fPath = "";
	} else if (fPath.Length() > 1 && fPath[0] == '/') {
		fItemType = fPath[1];
		fPath.Remove(0, 2);
	}
}


BGopherRequest::~BGopherRequest()
{
	Stop();

	delete fSocket;
}


status_t
BGopherRequest::Stop()
{
	if (fSocket != NULL) {
		fSocket->Disconnect();
			// Unlock any pending connect, read or write operation.
	}
	return BNetworkRequest::Stop();
}


const BUrlResult&
BGopherRequest::Result() const
{
	return fResult;
}


status_t
BGopherRequest::_ProtocolLoop()
{
	if (fSocket == NULL)
		return B_NO_MEMORY;

	if (!_ResolveHostName(fUrl.Host(), fUrl.HasPort() ? fUrl.Port() : 70)) {
		_EmitDebug(B_URL_PROTOCOL_DEBUG_ERROR,
			"Unable to resolve hostname (%s), aborting.",
				fUrl.Host().String());
		return B_SERVER_NOT_FOUND;
	}

	_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT, "Connection to %s on port %d.",
		fUrl.Authority().String(), fRemoteAddr.Port());
	status_t connectError = fSocket->Connect(fRemoteAddr);

	if (connectError != B_OK) {
		_EmitDebug(B_URL_PROTOCOL_DEBUG_ERROR, "Socket connection error %s",
			strerror(connectError));
		return connectError;
	}

	//! ProtocolHook:ConnectionOpened
	if (fListener != NULL)
		fListener->ConnectionOpened(this);

	_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT,
		"Connection opened, sending request.");

	_SendRequest();
	_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT, "Request sent.");

	// Receive loop
	bool receiveEnd = false;
	status_t readError = B_OK;
	ssize_t bytesRead = 0;
	//ssize_t bytesReceived = 0;
	//ssize_t bytesTotal = 0;
	bool dataValidated = false;
	BStackOrHeapArray<char, 4096> chunk(kGopherBufferSize);

	while (!fQuit && !receiveEnd) {
		fSocket->WaitForReadable();
		bytesRead = fSocket->Read(chunk, kGopherBufferSize);

		if (bytesRead < 0) {
			readError = bytesRead;
			break;
		} else if (bytesRead == 0)
			receiveEnd = true;

		fInputBuffer.AppendData(chunk, bytesRead);

		if (!dataValidated) {
			size_t i;
			// on error (file doesn't exist, ...) the server sends
			// a faked directory entry with an error message
			if (fInputBuffer.Size() && fInputBuffer.Data()[0] == '3') {
				int tabs = 0;
				bool crlf = false;

				// make sure the buffer only contains printable characters
				// and has at least 3 tabs before a CRLF
				for (i = 0; i < fInputBuffer.Size(); i++) {
					char c = fInputBuffer.Data()[i];
					if (c == '\t') {
						if (!crlf)
							tabs++;
					} else if (c == '\r' || c == '\n') {
						if (tabs < 3)
							break;
						crlf = true;
					} else if (!isprint(fInputBuffer.Data()[i])) {
						crlf = false;
						break;
					}
				}
				if (crlf && tabs > 2 && tabs < 5) {
					// TODO:
					//if enough data
					// else continue
					fItemType = GOPHER_TYPE_DIRECTORY;
					readError = B_RESOURCE_NOT_FOUND;
					// continue parsing the error text anyway
				}
			}
			// special case for buggy(?) Gophernicus/1.5
			static const char *buggy = "Error: File or directory not found!";
			if (fInputBuffer.Size() > strlen(buggy)
				&& !memcmp(fInputBuffer.Data(), buggy, strlen(buggy))) {
				fItemType = GOPHER_TYPE_DIRECTORY;
				readError = B_RESOURCE_NOT_FOUND;
				// continue parsing the error text anyway
				// but it won't look good
			}

			// now we probably have correct data
			dataValidated = true;

			//! ProtocolHook:ResponseStarted
			if (fListener != NULL)
				fListener->ResponseStarted(this);

			// now we can assign MIME type if we know it
			const char *mime = "application/octet-stream";
			for (i = 0; gopher_type_map[i].type != GOPHER_TYPE_NONE; i++) {
				if (gopher_type_map[i].type == fItemType) {
					mime = gopher_type_map[i].mime;
					break;
				}
			}
			fResult.SetContentType(mime);

			// we don't really have headers but well...
			//! ProtocolHook:HeadersReceived
			if (fListener != NULL)
				fListener->HeadersReceived(this, fResult);
		}

		if (_NeedsParsing())
			_ParseInput(receiveEnd);
		else if (fInputBuffer.Size()) {
			// send input directly
			if (fListener != NULL) {
				fListener->DataReceived(this, (const char *)fInputBuffer.Data(),
					fPosition, fInputBuffer.Size());
			}

			fPosition += fInputBuffer.Size();

			// XXX: this is plain stupid, we already copied the data
			// and just want to drop it...
			char *inputTempBuffer = new(std::nothrow) char[bytesRead];
			if (inputTempBuffer == NULL) {
				readError = B_NO_MEMORY;
				break;
			}
			fInputBuffer.RemoveData(inputTempBuffer, fInputBuffer.Size());
			delete[] inputTempBuffer;
		}
	}

	if (fPosition > 0) {
		fResult.SetLength(fPosition);
		if (fListener != NULL)
			fListener->DownloadProgress(this, fPosition, fPosition);
	}

	fSocket->Disconnect();

	if (readError != B_OK)
		return readError;

	return fQuit ? B_INTERRUPTED : B_OK;
}


void
BGopherRequest::_SendRequest()
{
	BString request;

	request << fPath;

	if (Url().HasRequest())
		request << '\t' << Url().Request();

	request << "\r\n";

	fSocket->Write(request.String(), request.Length());
}


bool
BGopherRequest::_NeedsParsing()
{
	if (fItemType == GOPHER_TYPE_DIRECTORY
		|| fItemType == GOPHER_TYPE_QUERY)
		return true;
	return false;
}


bool
BGopherRequest::_NeedsLastDotStrip()
{
	if (fItemType == GOPHER_TYPE_DIRECTORY
		|| fItemType == GOPHER_TYPE_QUERY
		|| fItemType == GOPHER_TYPE_TEXTPLAIN)
		return true;
	return false;
}


void
BGopherRequest::_ParseInput(bool last)
{
	BString line;

	while (_GetLine(line) == B_OK) {
		char type = GOPHER_TYPE_NONE;
		BStringList fields;

		line.MoveInto(&type, 0, 1);

		line.Split("\t", false, fields);

		if (type != GOPHER_TYPE_ENDOFPAGE
			&& fields.CountStrings() < FIELD_GPFLAG)
			_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT,
				"Unterminated gopher item (type '%c')", type);

		BString pageTitle;
		BString item;
		BString title = fields.StringAt(FIELD_NAME);
		BString link("gopher://");
		BString user;
		if (fields.CountStrings() > 3) {
			link << fields.StringAt(FIELD_HOST);
			if (fields.StringAt(FIELD_PORT).Length())
				link << ":" << fields.StringAt(FIELD_PORT);
			link << "/" << type;
			//if (fields.StringAt(FIELD_SELECTOR).ByteAt(0) != '/')
			//	link << "/";
			link << fields.StringAt(FIELD_SELECTOR);
		}
		_HTMLEscapeString(title);
		_HTMLEscapeString(link);

		switch (type) {
			case GOPHER_TYPE_ENDOFPAGE:
				/* end of the page */
				break;
			case GOPHER_TYPE_TEXTPLAIN:
				item << "<a href=\"" << link << "\">"
						"<span class=\"text\">" << title << "</span></a>"
						"<br/>\n";
				break;
			case GOPHER_TYPE_BINARY:
			case GOPHER_TYPE_BINHEX:
			case GOPHER_TYPE_BINARCHIVE:
			case GOPHER_TYPE_UUENCODED:
				item << "<a href=\"" << link << "\">"
						"<span class=\"binary\">" << title << "</span></a>"
						"<br/>\n";
				break;
			case GOPHER_TYPE_DIRECTORY:
				/*
				 * directory link
				 */
				item << "<a href=\"" << link << "\">"
						"<span class=\"dir\">" << title << "</span></a>"
						"<br/>\n";
				break;
			case GOPHER_TYPE_ERROR:
				item << "<span class=\"error\">" << title << "</span>"
						"<br/>\n";
				if (fPosition == 0 && pageTitle.Length() == 0)
					pageTitle << "Error: " << title;
				break;
			case GOPHER_TYPE_QUERY:
				/* TODO: handle search better.
				 * For now we use an unnamed input field and accept sending ?=foo
				 * as it seems at least Veronica-2 ignores the = but it's unclean.
				 */
				item << "<form method=\"get\" action=\"" << link << "\" "
							"onsubmit=\"window.location = this.action + '?' + "
								"this.elements['q'].value; return false;\">"
						"<span class=\"query\">"
						"<label>" << title << " "
						"<input id=\"q\" name=\"\" type=\"text\" align=\"right\" />"
						"</label>"
						"</span></form>"
						"<br/>\n";
				break;
			case GOPHER_TYPE_TELNET:
				/* telnet: links
				 * cf. gopher://78.80.30.202/1/ps3
				 * -> gopher://78.80.30.202:23/8/ps3/new -> new@78.80.30.202
				 */
				link = "telnet://";
				user = fields.StringAt(FIELD_SELECTOR);
				if (user.FindLast('/') > -1) {
					user.Remove(0, user.FindLast('/'));
					link << user << "@";
				}
				link << fields.StringAt(FIELD_HOST);
				if (fields.StringAt(FIELD_PORT) != "23")
					link << ":" << fields.StringAt(FIELD_PORT);

				item << "<a href=\"" << link << "\">"
						"<span class=\"telnet\">" << title << "</span></a>"
						"<br/>\n";
				break;
			case GOPHER_TYPE_TN3270:
				/* tn3270: URI scheme, cf. http://tools.ietf.org/html/rfc6270 */
				link = "tn3270://";
				user = fields.StringAt(FIELD_SELECTOR);
				if (user.FindLast('/') > -1) {
					user.Remove(0, user.FindLast('/'));
					link << user << "@";
				}
				link << fields.StringAt(FIELD_HOST);
				if (fields.StringAt(FIELD_PORT) != "23")
					link << ":" << fields.StringAt(FIELD_PORT);

				item << "<a href=\"" << link << "\">"
						"<span class=\"telnet\">" << title << "</span></a>"
						"<br/>\n";
				break;
			case GOPHER_TYPE_CSO_SEARCH:
				/* CSO search.
				 * At least Lynx supports a cso:// URI scheme:
				 * http://lynx.isc.org/lynx2.8.5/lynx2-8-5/lynx_help/lynx_url_support.html
				 */
				link = "cso://";
				user = fields.StringAt(FIELD_SELECTOR);
				if (user.FindLast('/') > -1) {
					user.Remove(0, user.FindLast('/'));
					link << user << "@";
				}
				link << fields.StringAt(FIELD_HOST);
				if (fields.StringAt(FIELD_PORT) != "105")
					link << ":" << fields.StringAt(FIELD_PORT);

				item << "<a href=\"" << link << "\">"
						"<span class=\"cso\">" << title << "</span></a>"
						"<br/>\n";
				break;
			case GOPHER_TYPE_GIF:
			case GOPHER_TYPE_IMAGE:
			case GOPHER_TYPE_PNG:
			case GOPHER_TYPE_BITMAP:
				/* quite dangerous, cf. gopher://namcub.accela-labs.com/1/pics */
				if (kInlineImages) {
					item << "<a href=\"" << link << "\">"
							"<span class=\"img\">" << title << " "
							"<img src=\"" << link << "\" "
								"alt=\"" << title << "\"/>"
							"</span></a>"
							"<br/>\n";
					break;
				}
				/* fallback to default, link them */
				item << "<a href=\"" << link << "\">"
						"<span class=\"img\">" << title << "</span></a>"
						"<br/>\n";
				break;
			case GOPHER_TYPE_HTML:
				/* cf. gopher://pineapple.vg/1 */
				if (fields.StringAt(FIELD_SELECTOR).StartsWith("URL:")) {
					link = fields.StringAt(FIELD_SELECTOR);
					link.Remove(0, 4);
				}
				/* cf. gopher://sdf.org/1/sdf/classes/ */

				item << "<a href=\"" << link << "\">"
						"<span class=\"html\">" << title << "</span></a>"
						"<br/>\n";
				break;
			case GOPHER_TYPE_INFO:
				// TITLE resource, cf.
				// gopher://gophernicus.org/0/doc/gopher/gopher-title-resource.txt
				if (fPosition == 0 && pageTitle.Length() == 0
					&& fields.StringAt(FIELD_SELECTOR) == "TITLE") {
						pageTitle = title;
						break;
				}
				item << "<span class=\"info\">" << title << "</span>"
						"<br/>\n";
				break;
			case GOPHER_TYPE_AUDIO:
			case GOPHER_TYPE_SOUND:
				item << "<a href=\"" << link << "\">"
						"<span class=\"audio\">" << title << "</span></a>"
						"<audio src=\"" << link << "\" "
							//TODO:Fix crash in WebPositive with these
							//"controls=\"controls\" "
							//"width=\"300\" height=\"50\" "
							"alt=\"" << title << "\"/>"
						"<span>[player]</span></audio>"
						"<br/>\n";
				break;
			case GOPHER_TYPE_PDF:
			case GOPHER_TYPE_DOC:
				/* generic case for known-to-work items */
				item << "<a href=\"" << link << "\">"
						"<span class=\"document\">" << title << "</span></a>"
						"<br/>\n";
				break;
			case GOPHER_TYPE_MOVIE:
				item << "<a href=\"" << link << "\">"
						"<span class=\"video\">" << title << "</span></a>"
						"<video src=\"" << link << "\" "
							//TODO:Fix crash in WebPositive with these
							//"controls=\"controls\" "
							//"width=\"300\" height=\"300\" "
							"alt=\"" << title << "\"/>"
						"<span>[player]</span></audio>"
						"<br/>\n";
				break;
			default:
				_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT,
					"Unknown gopher item (type 0x%02x '%c')", type, type);
				item << "<a href=\"" << link << "\">"
						"<span class=\"unknown\">" << title << "</span></a>"
						"<br/>\n";
				break;
		}

		if (fPosition == 0) {
			if (pageTitle.Length() == 0)
				pageTitle << "Index of " << Url();

			const char *uplink = ".";
			if (fPath.EndsWith("/"))
				uplink = "..";

			// emit header
			BString header;
			header << 
				"<html>\n"
				"<head>\n"
				"<meta http-equiv=\"Content-Type\""
					" content=\"text/html; charset=UTF-8\" />\n"
				//FIXME: fix links
				//"<link rel=\"icon\" type=\"image/png\""
				//	" href=\"resource:icons/directory.png\">\n"
				"<style type=\"text/css\">\n" << kStyleSheet << "</style>\n"
				"<title>" << pageTitle << "</title>\n"
				"</head>\n"
				"<body id=\"gopher\">\n"
				"<div class=\"uplink dontprint\">\n"
				"<a href=" << uplink << ">[up]</a>\n"
				"<a href=\"/\">[top]</a>\n"
				"</div>\n"
				"<h1>" << pageTitle << "</h1>\n";

			fListener->DataReceived(this, header.String(), fPosition,
				header.Length());

			fPosition += header.Length();
		}

		if (item.Length()) {
			fListener->DataReceived(this, item.String(), fPosition,
				item.Length());

			fPosition += item.Length();
		}
	}

	if (last) {
		// emit footer
		BString footer =
			"</div>\n"
			"</body>\n"
			"</html>\n";

		fListener->DataReceived(this, footer.String(), fPosition,
			footer.Length());

		fPosition += footer.Length();
	}
}


BString&
BGopherRequest::_HTMLEscapeString(BString &str)
{
	str.ReplaceAll("&", "&amp;");
	str.ReplaceAll("<", "&lt;");
	str.ReplaceAll(">", "&gt;");
	return str;
}
