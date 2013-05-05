// Sun, 18 Jun 2000
// Y.Takagi

#include <Alert.h>
#include <DataIO.h>
#include <Message.h>
#include <Directory.h>

#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iomanip>
#include <sstream>

#include "LpsClient.h"
#include "LprSetupDlg.h"
#include "LprTransport.h"
#include "LprDefs.h"
#include "DbgMsg.h"


using namespace std;


LprTransport::LprTransport(BMessage *msg)
	:
	BDataIO()
{
	fServer[0] = '\0';
	fQueue[0]  = '\0';
	fFile[0]   = '\0';
	fUser[0]   = '\0';
	fJobId     = 0;
	fError     = false;

	struct passwd *pwd = getpwuid(geteuid());
	if (pwd != NULL && pwd->pw_name != NULL && pwd->pw_name[0])
		strcpy(fUser, pwd->pw_name);
	else
		strcpy(fUser, "baron");

	DUMP_BMESSAGE(msg);

	const char *spool_path = msg->FindString(SPOOL_PATH);
	if (spool_path && *spool_path) {
		BDirectory dir(spool_path);
		DUMP_BDIRECTORY(&dir);

		dir.ReadAttr(LPR_SERVER_NAME, B_STRING_TYPE, 0, fServer, sizeof(fServer));
		if (fServer[0] == '\0') {
			LprSetupDlg *dlg = new LprSetupDlg(&dir);
			if (dlg->Go() == B_ERROR) {
				fError = true;
				return;
			}
		}

		dir.ReadAttr(LPR_SERVER_NAME, B_STRING_TYPE, 0, fServer, sizeof(fServer));
		dir.ReadAttr(LPR_QUEUE_NAME,  B_STRING_TYPE, 0, fQueue,  sizeof(fQueue));
		dir.ReadAttr(LPR_JOB_ID,      B_INT32_TYPE,  0, &fJobId, sizeof(fJobId));
		fJobId++;
		if (fJobId > 255) {
			fJobId = 1;
		}
		dir.WriteAttr(LPR_JOB_ID, B_INT32_TYPE, 0, &fJobId, sizeof(fJobId));

		sprintf(fFile, "%s/%s@ipp.%ld", spool_path, fUser, fJobId);

		fStream.open(fFile, ios::in | ios::out | ios::binary | ios::trunc);
		if (fStream.good()) {
			DBGMSG(("spool_file: %s\n", fFile));
			return;
		}
	}
	fError = true;
}


LprTransport::~LprTransport()
{
	if (!fError)
		_SendFile();

	if (fFile[0] != '\0')
		unlink(fFile);
}


void
LprTransport::_SendFile()
{
	char hostname[128];
	if (gethostname(hostname, sizeof(hostname)) != B_OK)
		strcpy(hostname, "localhost");

	ostringstream cfname;
	cfname << "cfA" << setw(3) << setfill('0') << fJobId << hostname;

	ostringstream dfname;
	dfname << "dfA" << setw(3) << setfill('0') << fJobId << hostname;

	ostringstream cf;
	cf << 'H' << hostname     << '\n';
	cf << 'P' << fUser << '\n';
	cf << 'l' << dfname.str() << '\n';
	cf << 'U' << dfname.str() << '\n';

	long cfsize = cf.str().length();
	long dfsize = fStream.tellg();
	fStream.seekg(0, ios::beg);

	try {
		LpsClient lpr(fServer);

		lpr.connect();
		lpr.receiveJob(fQueue);

		lpr.receiveControlFile(cfsize, cfname.str().c_str());
		lpr.transferData(cf.str().c_str(), cfsize);
		lpr.endTransfer();

		lpr.receiveDataFile(dfsize, dfname.str().c_str());
		lpr.transferData(fStream, dfsize);
		lpr.endTransfer();
	}

	catch (LPSException &err) {
		DBGMSG(("error: %s\n", err.what()));
		BAlert *alert = new BAlert("", err.what(), "OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
	}
}


ssize_t
LprTransport::Read(void *, size_t)
{
	return 0;
}


ssize_t
LprTransport::Write(const void *buffer, size_t size)
{
	if (!fStream.write((char *)buffer, size)) {
		fError = true;
		return 0;
	}
	return size;
}
