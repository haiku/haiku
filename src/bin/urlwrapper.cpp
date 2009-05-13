/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Jonas Sundström, jonas@kirilla.com
 */

/*
 * urlwrapper: wraps URL mime types around command line apps
 * or other apps that don't handle them directly.
 */
#define DEBUG 0

#include <Alert.h>
#include <Application.h>
#include <Debug.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <String.h>
#include <Url.h>

#include "urlwrapper.h"

const char *kAppSig = APP_SIGNATURE;
const char *kTrackerSig = "application/x-vnd.Be-TRAK";

#ifdef __HAIKU__
const char *kTerminalSig = "application/x-vnd.Haiku-Terminal";
#else
const char *kTerminalSig = "application/x-vnd.Be-SHEL";
#endif

#ifdef HANDLE_BESHARE
const char *kBeShareSig = "application/x-vnd.Sugoi-BeShare";
#endif

#ifdef HANDLE_IM
const char *kIMSig = "application/x-vnd.m_eiman.sample_im_client";
#endif

#ifdef HANDLE_VLC
const char *kVLCSig = "application/x-vnd.videolan-vlc";
#endif


class UrlWrapper : public BApplication
{
public:
								UrlWrapper();
								~UrlWrapper();

	virtual void				RefsReceived(BMessage *msg);
	virtual void				ArgvReceived(int32 argc, char **argv);
	virtual void				ReadyToRun(void);

private:
			status_t			_Warn(const char *url);
};


UrlWrapper::UrlWrapper() : BApplication(kAppSig)
{
}


UrlWrapper::~UrlWrapper()
{
}


status_t UrlWrapper::_Warn(const char *url)
{
	BString message("An application has requested the system to open the following url: \n");
	message << "\n" << url << "\n\n";
	message << "This type of urls has a potential security risk.\n";
	message << "Proceed anyway ?";
	BAlert *alert = new BAlert("Warning", message.String(), "Ok", "No", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	int32 v;
	v = alert->Go();
	if (v == 0)
		return B_OK;
	return B_ERROR;
}


void UrlWrapper::RefsReceived(BMessage *msg)
{
	char buff[B_PATH_NAME_LENGTH];
	int32 index = 0;
	entry_ref ref;
	char *args[] = { "urlwrapper", buff, NULL };
	status_t err;

	while (msg->FindRef("refs", index++, &ref) == B_OK) {
		BFile f(&ref, B_READ_ONLY);
		BNodeInfo ni(&f);
		BString mimetype;
		if (f.InitCheck() == B_OK && ni.InitCheck() == B_OK) {
			ni.GetType(mimetype.LockBuffer(B_MIME_TYPE_LENGTH));
			mimetype.UnlockBuffer();
#ifdef HANDLE_URL_FILES
			// http://filext.com/file-extension/URL
			if (mimetype == "wwwserver/redirection"
			 || mimetype == "application/internet-shortcut"
			 || mimetype == "application/x-url"
			 || mimetype == "message/external-body"
			 || mimetype == "text/url"
			 || mimetype == "text/x-url") {
				// http://www.cyanwerks.com/file-format-url.html
				off_t size;
				if (f.GetSize(&size) < B_OK)
					continue;
				BString contents, url;
				if (f.ReadAt(0LL, contents.LockBuffer(size), size) < B_OK)
					continue;
				while (contents.Length()) {
					BString line;
					int32 cr = contents.FindFirst('\n');
					if (cr < 0)
						cr = contents.Length();
					//contents.MoveInto(line, 0, cr);
					contents.CopyInto(line, 0, cr);
					contents.Remove(0, cr+1);
					line.RemoveAll("\r");
					if (!line.Length())
						continue;
					if (!line.ICompare("URL=", 4)) {
						line.MoveInto(url, 4, line.Length());
						break;
					}
				}
				if (url.Length()) {
					args[1] = (char *)url.String();
					//ArgvReceived(2, args);
					err = be_roster->Launch("application/x-vnd.Be.URL.http", 1, args+1);
					continue;
				}
			}
#endif
			/* eat everything as bookmark files */
#ifdef HANDLE_BOOKMARK_FILES
			if (f.ReadAttr("META:url", B_STRING_TYPE, 0LL, buff, B_PATH_NAME_LENGTH) > 0) {
				//ArgvReceived(2, args);
				err = be_roster->Launch("application/x-vnd.Be.URL.http", 1, args+1);
				continue;
			}
#endif
		}
	}
}


void UrlWrapper::ArgvReceived(int32 argc, char **argv)
{
	if (argc <= 1)
		return;
	
	const char *failc = " || read -p 'Press any key'";
	const char *pausec = " ; read -p 'Press any key'";
	char *args[] = { "/bin/sh", "-c", NULL, NULL};

	BPrivate::Support::BUrl url(argv[1]);

	BString full = url.Full();
	BString proto = url.Proto();
	BString host = url.Host();
	BString port = url.Port();
	BString user = url.User();
	BString pass = url.Pass();
	BString path = url.Path();

	if (url.InitCheck() < 0) {
		fprintf(stderr, "malformed url: '%s'\n", url.String());
		return;
	}
	
	// XXX: debug
	PRINT(("PROTO='%s'\n", proto.String()));
	PRINT(("HOST='%s'\n", host.String()));
	PRINT(("PORT='%s'\n", port.String()));
	PRINT(("USER='%s'\n", user.String()));
	PRINT(("PASS='%s'\n", pass.String()));
	PRINT(("PATH='%s'\n", path.String()));

	if (proto == "telnet") {
		BString cmd("telnet ");
		if (url.HasUser())
			cmd << "-l " << user << " ";
		cmd << host;
		if (url.HasPort())
			cmd << " " << port;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << failc;
		args[2] = (char *)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		return;
	}
	
	// see draft: http://tools.ietf.org/wg/secsh/draft-ietf-secsh-scp-sftp-ssh-uri/
	if (proto == "ssh") {
		BString cmd("ssh ");
		
		if (url.HasUser())
			cmd << "-l " << user << " ";
		if (url.HasPort())
			cmd << "-oPort=" << port << " ";
		cmd << host;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << failc;
		args[2] = (char *)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}

	if (proto == "ftp") {
		BString cmd("ftp ");
		
		/*
		if (user.Length())
			cmd << "-l " << user << " ";
		cmd << host;
		*/
		cmd << full;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << failc;
		args[2] = (char *)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}
	
	if (proto == "sftp") {
		BString cmd("sftp ");
		
		//cmd << url;
		if (url.HasPort())
			cmd << "-oPort=" << port << " ";
		if (url.HasUser())
			cmd << user << "@";
		cmd << host;
		if (url.HasPath())
			cmd << ":" << path;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << failc;
		args[2] = (char *)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}

	if (proto == "finger") {
		BString cmd("/bin/finger ");
		
		if (url.HasUser())
			cmd << user;
		if (url.HasHost() == 0)
			host = "127.0.0.1";
		cmd << "@" << host;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << pausec;
		args[2] = (char *)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}

#ifdef HANDLE_HTTP_WGET
	if (proto == "http") {
		BString cmd("/bin/wget ");
		
		//cmd << url;
		if (url.HasUser())
			cmd << user << "@";
		cmd << full;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << pausec;
		args[2] = (char *)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}
#endif

#ifdef HANDLE_FILE
	if (proto == "file") {
		BMessage m(B_REFS_RECEIVED);
		entry_ref ref;
		url.UnurlString(path);
		if (get_ref_for_path(path.String(), &ref) < B_OK)
			return;
		m.AddRef("refs", &ref);
		be_roster->Launch(kTrackerSig, &m);
		return;
	}
#endif

#ifdef HANDLE_QUERY
	// XXX:TODO: split options
	if (proto == "query") {
		// mktemp ?
		BString qname("/tmp/query-url-temp-");
		qname << getpid() << "-" << system_time();
		BFile query(qname.String(), O_CREAT|O_EXCL);
		// XXX: should check for failure
		
		BString s;
		int32 v;
		
		url.UnurlString(full);
		// TODO: handle options (list of attrs in the column, ...)

		v = 'qybF'; // QuerY By Formula XXX: any #define for that ?
		query.WriteAttr("_trk/qryinitmode", B_INT32_TYPE, 0LL, &v, sizeof(v));
		s = "TextControl";
		query.WriteAttr("_trk/focusedView", B_STRING_TYPE, 0LL, s.String(), s.Length()+1);
		s = full;
		PRINT(("QUERY='%s'\n", s.String()));
		query.WriteAttr("_trk/qryinitstr", B_STRING_TYPE, 0LL, s.String(), s.Length()+1);
		query.WriteAttr("_trk/qrystr", B_STRING_TYPE, 0LL, s.String(), s.Length()+1);
		s = "application/x-vnd.Be-query";
		query.WriteAttr("BEOS:TYPE", 'MIMS', 0LL, s.String(), s.Length()+1);
		

		BEntry e(qname.String());
		entry_ref er;
		if (e.GetRef(&er) >= B_OK)
			be_roster->Launch(&er);
		return;
	}
#endif

#ifdef HANDLE_SH
	if (proto == "sh") {
		BString cmd(full);
		if (_Warn(url.String()) != B_OK)
			return;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << pausec;
		args[2] = (char *)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}
#endif

#ifdef HANDLE_BESHARE
	if (proto == "beshare") {
		team_id team;
		BMessenger msgr(kBeShareSig);
		// if no instance is running, or we want a specific server, start it.
		if (!msgr.IsValid() || url.HasHost()) {
			be_roster->Launch(kBeShareSig, (BMessage *)NULL, &team);
			msgr = BMessenger(NULL, team);
		}
		if (url.HasHost()) {
			BMessage mserver('serv');
			mserver.AddString("server", host);
			msgr.SendMessage(&mserver);
			
		}
		if (url.HasPath()) {
			BMessage mquery('quer');
			mquery.AddString("query", path);
			msgr.SendMessage(&mquery);
		}
		// TODO: handle errors
		return;
	}
#endif

#ifdef HANDLE_IM
	if (proto == "icq" || proto == "msn") {
		// TODO
		team_id team;
		be_roster->Launch(kIMSig, (BMessage *)NULL, &team);
		BMessenger msgr(NULL, team);
		if (url.HasHost()) {
			BMessage mserver(B_REFS_RECEIVED);
			mserver.AddString("server", host);
			msgr.SendMessage(&httpmserver);
			
		}
		// TODO: handle errors
		return;
	}
#endif

#ifdef HANDLE_VLC
	if (proto == "mms" || proto == "rtp" || proto == "rtsp") {
		args[0] = (char *)url.String();
		be_roster->Launch(kVLCSig, 1, args);
		return;
	}
#endif

#ifdef HANDLE_AUDIO
	// TODO
#endif

	// vnc: ?
	// irc: ?
	// 
	// svn: ?
	// cvs: ?
	// smb: cifsmount ?
	// nfs: mount_nfs ?
	//
	// mailto: ? but BeMail & Beam both handle it already (not fully though).
	//
	// itps: pcast: podcast: s//http/ + parse xml to get url to mp3 stream...
	// audio: s//http:/ + default MediaPlayer -- see http://forums.winamp.com/showthread.php?threadid=233130
	//
	// gps: ? I should submit an RFC for that one :)

}


void UrlWrapper::ReadyToRun(void)
{
	Quit();
}


int main(int argc, char **argv)
{
	UrlWrapper app;
	if (be_app)
		app.Run();
	return 0;
}

