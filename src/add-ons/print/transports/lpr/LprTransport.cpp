// Sun, 18 Jun 2000
// Y.Takagi

#include <DataIO.h>
#include <Message.h>
#include <Directory.h>
#include <net/netdb.h>
#include <Alert.h>
#include <stdio.h>
#include <unistd.h>
#include <iomanip>
#include  "_sstream"

#include "LpsClient.h"
#include "LprSetupDlg.h"
#include "LprTransport.h"
#include "LprDefs.h"
#include "DbgMsg.h"

#if (!__MWERKS__ || defined(WIN32))
using namespace std;
#else 
#define std
#endif

LprTransport::LprTransport(BMessage *msg)
	: BDataIO()
{
	__server[0] = '\0';
	__queue[0]  = '\0';
	__file[0]   = '\0';
	__user[0]   = '\0';
	__jobid     = 0;
	__error     = false;

	DUMP_BMESSAGE(msg);

	const char *spool_path = msg->FindString(SPOOL_PATH);
	if (spool_path && *spool_path) {
		BDirectory dir(spool_path);
		DUMP_BDIRECTORY(&dir);

		dir.ReadAttr(LPR_SERVER_NAME, B_STRING_TYPE, 0, __server, sizeof(__server));
		if (__server[0] == '\0') {
			LprSetupDlg *dlg = new LprSetupDlg(&dir);
			if (dlg->Go() == B_ERROR) {
				__error = true;
				return;
			}
		}

		dir.ReadAttr(LPR_SERVER_NAME, B_STRING_TYPE, 0, __server, sizeof(__server));
		dir.ReadAttr(LPR_QUEUE_NAME,  B_STRING_TYPE, 0, __queue,  sizeof(__queue));
		dir.ReadAttr(LPR_JOB_ID,      B_INT32_TYPE,  0, &__jobid, sizeof(__jobid));
		__jobid++;
		if (__jobid > 255) {
			__jobid = 1;
		}
		dir.WriteAttr(LPR_JOB_ID, B_INT32_TYPE, 0, &__jobid, sizeof(__jobid));

		getusername(__user, sizeof(__user));
		if (__user[0] == '\0') {
			strcpy(__user, "baron");
		}

		sprintf(__file, "%s/%s@ipp.%ld", spool_path, __user, __jobid);

		__fs.open(__file, ios::in | ios::out | ios::binary | ios::trunc);
		if (__fs.good()) {
			DBGMSG(("spool_file: %s\n", __file));
			return;
		}
	}
	__error = true;
}

LprTransport::~LprTransport()
{
	char hostname[128];
	gethostname(hostname, sizeof(hostname));

	char username[128];
#ifdef WIN32
	unsigned long length = sizeof(hostname);
	GetUserName(username, &length);
#else
	getusername(username, sizeof(username));
#endif

	ostringstream cfname;
	cfname << "cfA" << setw(3) << setfill('0') << __jobid << hostname;

	ostringstream dfname;
	dfname << "dfA" << setw(3) << setfill('0') << __jobid << hostname;

	ostringstream cf;
	cf << 'H' << hostname     << '\n';
	cf << 'P' << username     << '\n';
	cf << 'l' << dfname.str() << '\n';
	cf << 'U' << dfname.str() << '\n';

	long cfsize = cf.str().length();
	long dfsize = __fs.tellg();
	__fs.seekg(0, ios::beg);

	try {
		LpsClient lpr(__server);

		lpr.connect();
		lpr.receiveJob(__queue);

		lpr.receiveControlFile(cfsize, cfname.str().c_str());
		lpr.transferData(cf.str().c_str(), cfsize);
		lpr.endTransfer();

		lpr.receiveDataFile(dfsize, dfname.str().c_str());
		lpr.transferData(__fs, dfsize);
		lpr.endTransfer();
	}

	catch (LPSException &err) {
		DBGMSG(("error: %s\n", err.what()));
		BAlert *alert = new BAlert("", err.what(), "OK");
		alert->Go();
	}

	unlink(__file);
}

ssize_t LprTransport::Read(void *, size_t)
{
	return 0;
}

ssize_t LprTransport::Write(const void *buffer, size_t size)
{
//	DBGMSG(("write: %d\n", size));
	if (!__fs.write(buffer, size)) {
		__error = true;
		return 0;
	}
//	return __fs.pcount();
	return size;
}
