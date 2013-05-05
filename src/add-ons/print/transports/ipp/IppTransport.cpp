// Sun, 18 Jun 2000
// Y.Takagi

#include <Alert.h>
#include <DataIO.h>
#include <Message.h>
#include <Directory.h>

#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "URL.h"
#include "IppContent.h"
#include "IppURLConnection.h"
#include "IppSetupDlg.h"
#include "IppTransport.h"
#include "IppDefs.h"
#include "DbgMsg.h"

#if (!__MWERKS__)
using namespace std;
#else 
#define std
#endif

IppTransport::IppTransport(BMessage *msg)
	: BDataIO()
{
	__url[0]  = '\0';
	__user[0] = '\0';
	__file[0] = '\0';
	__jobid   = 0;
	__error   = false;

	DUMP_BMESSAGE(msg);

	const char *spool_path = msg->FindString(SPOOL_PATH);
	if (spool_path && *spool_path) {
		BDirectory dir(spool_path);
		DUMP_BDIRECTORY(&dir);

		dir.ReadAttr(IPP_URL, B_STRING_TYPE, 0, __url, sizeof(__url));
		if (__url[0] == '\0') {
			IppSetupDlg *dlg = new IppSetupDlg(&dir);
			if (dlg->Go() == B_ERROR) {
				__error = true;
				return;
			}
		}

		dir.ReadAttr(IPP_URL,    B_STRING_TYPE, 0, __url,    sizeof(__url));
		dir.ReadAttr(IPP_JOB_ID, B_INT32_TYPE,  0, &__jobid, sizeof(__jobid));
		__jobid++;
		if (__jobid > 255) {
			__jobid = 1;
		}
		dir.WriteAttr(IPP_JOB_ID, B_INT32_TYPE, 0, &__jobid, sizeof(__jobid));

		struct passwd *pwd = getpwuid(geteuid());
		if (pwd != NULL && pwd->pw_name != NULL && pwd->pw_name[0])
			strcpy(__user, pwd->pw_name);
		else
			strcpy(__user, "baron");

		sprintf(__file, "%s/%s@ipp.%ld", spool_path, __user, __jobid);

		__fs.open(__file, ios::in | ios::out | ios::binary | ios::trunc);
		if (__fs.good()) {
			DBGMSG(("spool_file: %s\n", __file));
			return;
		}
	}
	__error = true;
}

IppTransport::~IppTransport()
{
	string error_msg;

	if (!__error && __fs.good()) {
		DBGMSG(("create IppContent\n"));
		IppContent *request = new IppContent;
		request->setOperationId(IPP_PRINT_JOB);
		request->setDelimiter(IPP_OPERATION_ATTRIBUTES_TAG);
		request->setCharset("attributes-charset", "utf-8");
		request->setNaturalLanguage("attributes-natural-language", "en-us");
		request->setURI("printer-uri", __url);
		request->setMimeMediaType("document-format", "application/octet-stream");
		request->setNameWithoutLanguage("requesting-user-name", __user);
//		request->setNameWithoutLanguage("job-name", __file);	// optional
		request->setDelimiter(IPP_END_OF_ATTRIBUTES_TAG);

		long fssize = __fs.tellg();
		__fs.seekg(0, ios::beg);
		request->setRawData(__fs, fssize);

		URL url(__url);
		IppURLConnection conn(url);
		conn.setIppRequest(request);
		conn.setRequestProperty("Connection", "close");

		DBGMSG(("do connect\n"));

		HTTP_RESPONSECODE response_code = conn.getResponseCode();
		if (response_code == HTTP_OK) {
			const char *content_type = conn.getContentType();
			if (content_type && !strncasecmp(content_type, "application/ipp", 15)) {
				const IppContent *ipp_response = conn.getIppResponse();
				if (ipp_response->fail()) {
					__error = true;
					error_msg = ipp_response->getStatusMessage();
				}
			} else {
				__error = true;
				error_msg = "cannot get a IPP response.";
			}
		} else if (response_code != HTTP_UNKNOWN) {
			__error = true;
			error_msg = conn.getResponseMessage();
		} else {
			__error = true;
			error_msg = "cannot connect to the IPP server.";
		}
	}

	unlink(__file);

	if (__error) {
		BAlert *alert = new BAlert("", error_msg.c_str(), "OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
	}
}

ssize_t IppTransport::Read(void *, size_t)
{
	return 0;
}

ssize_t IppTransport::Write(const void *buffer, size_t size)
{
//	DBGMSG(("write: %d\n", size));

	if (!__fs.write((const char *)buffer, size)) {
		__error = true;
		return 0;
	}
//	return __fs.pcount();
	return size;
}
