/*
 * Copyright 2007-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Jonas Sundström, jonas@kirilla.com
 *		Stephan Aßmus <superstippi@gmx.de>
 */

/*
 * urlwrapper: wraps URL mime types around command line apps
 * or other apps that don't handle them directly.
 */
#define DEBUG 0

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

#include <fs_volume.h>
#include <Alert.h>
#include <Debug.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <String.h>
#include <Url.h>

#include "urlwrapper.h"


const char* kAppSig = "application/x-vnd.Haiku-urlwrapper";
const char* kTrackerSig = "application/x-vnd.Be-TRAK";
const char* kTerminalSig = "application/x-vnd.Haiku-Terminal";
const char* kBeShareSig = "application/x-vnd.Sugoi-BeShare";
const char* kIMSig = "application/x-vnd.m_eiman.sample_im_client";
const char* kVLCSig = "application/x-vnd.videolan-vlc";

const char* kURLHandlerSigBase = "application/x-vnd.Be.URL.";


UrlWrapper::UrlWrapper() : BApplication(kAppSig)
{
}


UrlWrapper::~UrlWrapper()
{
}


status_t
UrlWrapper::_Warn(const char* url)
{
	BString message("An application has requested the system to open the "
		"following url: \n");
	message << "\n" << url << "\n\n";
	message << "This type of URL has a potential security risk.\n";
	message << "Proceed anyway?";
	BAlert* alert = new BAlert("Warning", message.String(), "Proceed", "Stop",
		NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetShortcut(1, B_ESCAPE);
	int32 button;
	button = alert->Go();
	if (button == 0)
		return B_OK;

	return B_ERROR;
}


void
UrlWrapper::RefsReceived(BMessage* msg)
{
	char buff[B_PATH_NAME_LENGTH];
	int32 index = 0;
	entry_ref ref;
	char* args[] = { const_cast<char*>("urlwrapper"), buff, NULL };
	status_t err;

	while (msg->FindRef("refs", index++, &ref) == B_OK) {
		BFile f(&ref, B_READ_ONLY);
		BNodeInfo ni(&f);
		BString mimetype;
		BString extension(ref.name);
		extension.Remove(0, extension.FindLast('.') + 1);
		if (f.InitCheck() == B_OK && ni.InitCheck() == B_OK) {
			ni.GetType(mimetype.LockBuffer(B_MIME_TYPE_LENGTH));
			mimetype.UnlockBuffer();

			// Internet Explorer Shortcut
			if (mimetype == "text/x-url" || extension == "url") {
				// http://filext.com/file-extension/URL
				// http://www.cyanwerks.com/file-format-url.html
				off_t size;
				if (f.GetSize(&size) < B_OK)
					continue;
				BString contents;
				BString url;
				if (f.ReadAt(0LL, contents.LockBuffer(size), size) < B_OK)
					continue;
				contents.UnlockBuffer();
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
					BUrl u(url.String());
					args[1] = (char*)u.UrlString().String();
					mimetype = kURLHandlerSigBase;
					mimetype += u.Protocol();
					err = be_roster->Launch(mimetype.String(), 1, args + 1);
					if (err != B_OK && err != B_ALREADY_RUNNING)
						err = be_roster->Launch(kAppSig, 1, args + 1);
					continue;
				}
			}
			if (mimetype == "text/x-webloc" || extension == "webloc") {
				// OSX url shortcuts
				// XML file + resource fork
				off_t size;
				if (f.GetSize(&size) < B_OK)
					continue;
				BString contents;
				BString url;
				char *buffer = contents.LockBuffer(size);
				const char bplist_match[] = "bplist00\xd1\x01\x02SURL_\x10";
				if (f.ReadAt(0LL, buffer, size) < B_OK)
					continue;
				printf("webloc\n");
				if (size > (sizeof(bplist_match) + 2)
					&& !strncmp(buffer, bplist_match,
						sizeof(bplist_match) - 1)) {
					// binary plist, let's be crude
					uint8 len = buffer[sizeof(bplist_match) - 1];
					url.SetTo(buffer + sizeof(bplist_match), len);
					*buffer = 0; // make sure we don't try to interpret as xml
				}
				contents.UnlockBuffer();
				int state = 0;
				while (contents.Length()) {
					BString line;
					int32 cr = contents.FindFirst('\n');
					if (cr < 0)
						cr = contents.Length();
					contents.CopyInto(line, 0, cr);
					contents.Remove(0, cr+1);
					line.RemoveAll("\r");
					if (!line.Length())
						continue;
					int32 s, e;
					switch (state) {
						case 0:
							if (!line.ICompare("<?xml", 5))
								state = 1;
							break;
						case 1:
							if (!line.ICompare("<plist", 6))
								state = 2;
							break;
						case 2:
							if (!line.ICompare("<dict>", 6))
								state = 3;
							break;
						case 3:
							if (line.IFindFirst("<key>URL</key>") > -1)
								state = 4;
							break;
						case 4:
							if ((s = line.IFindFirst("<string>")) > -1
								&& (e = line.IFindFirst("</string>")) > s) {
								state = 5;
								s += 8;
								line.MoveInto(url, s, e - s);
								break;
							} else
								state = 3;
							break;
						default:
							break;
					}
					if (state == 5) {
						break;
					}
				}
				if (url.Length()) {
					BUrl u(url.String());
					args[1] = (char*)u.UrlString().String();
					mimetype = kURLHandlerSigBase;
					mimetype += u.Protocol();
					err = be_roster->Launch(mimetype.String(), 1, args + 1);
					if (err != B_OK && err != B_ALREADY_RUNNING)
						err = be_roster->Launch(kAppSig, 1, args + 1);
					continue;
				}
			}

			// NetPositive Bookmark or any file with a META:url attribute
			if (f.ReadAttr("META:url", B_STRING_TYPE, 0LL, buff,
				B_PATH_NAME_LENGTH) > 0) {
				BUrl u(buff);
				args[1] = (char*)u.UrlString().String();
				mimetype = kURLHandlerSigBase;
				mimetype += u.Protocol();
				err = be_roster->Launch(mimetype.String(), 1, args + 1);
				if (err != B_OK && err != B_ALREADY_RUNNING)
					err = be_roster->Launch(kAppSig, 1, args + 1);
				continue;
			}
		}
	}
}


void
UrlWrapper::ArgvReceived(int32 argc, char** argv)
{
	if (argc <= 1)
		return;

	const char* failc = " || read -p 'Press any key'";
	const char* pausec = " ; read -p 'Press any key'";
	char* args[] = { (char *)"/bin/sh", (char *)"-c", NULL, NULL};
	status_t err;

	BUrl url(argv[1]);

	BString full = BUrl(url).SetProtocol(BString()).UrlString();
	BString proto = url.Protocol();
	BString host = url.Host();
	BString port = BString() << url.Port();
	BString user = url.UserInfo();
	BString pass = url.Password();
	BString path = url.Path();

	if (!url.IsValid()) {
		fprintf(stderr, "malformed url: '%s'\n", url.UrlString().String());
		return;
	}

	// XXX: debug
	PRINT(("PROTO='%s'\n", proto.String()));
	PRINT(("HOST='%s'\n", host.String()));
	PRINT(("PORT='%s'\n", port.String()));
	PRINT(("USER='%s'\n", user.String()));
	PRINT(("PASS='%s'\n", pass.String()));
	PRINT(("PATH='%s'\n", path.String()));

	if (proto == "about") {
		app_info info;
		BString sig;
		// BUrl could get an accessor for the full - proto part...
		sig = host << "/" << path;
		BMessage msg(B_ABOUT_REQUESTED);
		if (be_roster->GetAppInfo(sig.String(), &info) == B_OK) {
			BMessenger msgr(sig.String());
			msgr.SendMessage(&msg);
			return;
		}
		if (be_roster->Launch(sig.String(), &msg) == B_OK)
			return;
		be_roster->Launch("application/x-vnd.Haiku-About");
		return;
	}

	if (proto == "telnet") {
		BString cmd("telnet ");
		if (url.HasUserInfo())
			cmd << "-l " << user << " ";
		cmd << host;
		if (url.HasPort())
			cmd << " " << port;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << failc;
		args[2] = (char*)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		return;
	}

	// see draft:
	// http://tools.ietf.org/wg/secsh/draft-ietf-secsh-scp-sftp-ssh-uri/
	if (proto == "ssh") {
		BString cmd("ssh ");

		if (url.HasUserInfo())
			cmd << "-l " << user << " ";
		if (url.HasPort())
			cmd << "-oPort=" << port << " ";
		cmd << host;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << failc;
		args[2] = (char*)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}

	if (proto == "ftp") {
		BString cmd("ftp ");

		cmd << proto << "://";
		/*
		if (user.Length())
			cmd << "-l " << user << " ";
		cmd << host;
		*/
		cmd << full;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << failc;
		args[2] = (char*)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}

	if (proto == "sftp") {
		BString cmd("sftp ");

		//cmd << url;
		if (url.HasPort())
			cmd << "-oPort=" << port << " ";
		if (url.HasUserInfo())
			cmd << user << "@";
		cmd << host;
		if (url.HasPath())
			cmd << ":" << path;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << failc;
		args[2] = (char*)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}

	if (proto == "finger") {
		BString cmd("/bin/finger ");

		if (url.HasUserInfo())
			cmd << user;
		if (url.HasHost() == 0)
			host = "127.0.0.1";
		cmd << "@" << host;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << pausec;
		args[2] = (char*)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}

	if (proto == "http" || proto == "https" /*|| proto == "ftp"*/) {
		BString cmd("/bin/wget ");

		//cmd << url;
		cmd << proto << "://";
		if (url.HasUserInfo())
			cmd << user << "@";
		cmd << full;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << pausec;
		args[2] = (char*)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}

	if (proto == "file") {
		BMessage m(B_REFS_RECEIVED);
		entry_ref ref;
		_DecodeUrlString(path);
		if (get_ref_for_path(path.String(), &ref) < B_OK)
			return;
		m.AddRef("refs", &ref);
		be_roster->Launch(kTrackerSig, &m);
		return;
	}

	// XXX:TODO: split options
	if (proto == "query") {
		// mktemp ?
		BString qname("/tmp/query-url-temp-");
		qname << getpid() << "-" << system_time();
		BFile query(qname.String(), O_CREAT|O_EXCL);
		// XXX: should check for failure

		BString s;
		int32 v;

		_DecodeUrlString(full);
		// TODO: handle options (list of attrs in the column, ...)

		v = 'qybF'; // QuerY By Formula XXX: any #define for that ?
		query.WriteAttr("_trk/qryinitmode", B_INT32_TYPE, 0LL, &v, sizeof(v));
		s = "TextControl";
		query.WriteAttr("_trk/focusedView", B_STRING_TYPE, 0LL, s.String(),
			s.Length()+1);
		s = full;
		PRINT(("QUERY='%s'\n", s.String()));
		query.WriteAttr("_trk/qryinitstr", B_STRING_TYPE, 0LL, s.String(),
			s.Length()+1);
		query.WriteAttr("_trk/qrystr", B_STRING_TYPE, 0LL, s.String(),
			s.Length()+1);
		s = "application/x-vnd.Be-query";
		query.WriteAttr("BEOS:TYPE", 'MIMS', 0LL, s.String(), s.Length()+1);


		BEntry e(qname.String());
		entry_ref er;
		if (e.GetRef(&er) >= B_OK)
			be_roster->Launch(&er);
		return;
	}

	if (proto == "sh") {
		BString cmd(full);
		if (_Warn(url.UrlString()) != B_OK)
			return;
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << pausec;
		args[2] = (char*)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}

	if (proto == "beshare") {
		team_id team;
		BMessenger msgr(kBeShareSig);
		// if no instance is running, or we want a specific server, start it.
		if (!msgr.IsValid() || url.HasHost()) {
			be_roster->Launch(kBeShareSig, (BMessage*)NULL, &team);
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

	if (proto == "icq" || proto == "msn") {
		// TODO
		team_id team;
		be_roster->Launch(kIMSig, (BMessage*)NULL, &team);
		BMessenger msgr(NULL, team);
		if (url.HasHost()) {
			BMessage mserver(B_REFS_RECEIVED);
			mserver.AddString("server", host);
			msgr.SendMessage(&mserver);

		}
		// TODO: handle errors
		return;
	}

	if (proto == "mms" || proto == "rtp" || proto == "rtsp") {
		args[0] = (char*)url.UrlString().String();
		be_roster->Launch(kVLCSig, 1, args);
		return;
	}

	if (proto == "nfs") {
		BString parameter(host);
		_DecodeUrlString(path);
		if (url.HasPort())
			parameter << ":" << port;
		//XXX: should not always be absolute! FIXME
		parameter << ":/" << path;
		BString prettyPath(path);
		prettyPath.Remove(0, prettyPath.FindLast("/") + 1);
		if (path == "" || path == "/")
			prettyPath = "root";
		prettyPath << " on " << host;
		prettyPath.Prepend("/");
		if (mkdir(prettyPath.String(), 0755) < 0) {
			perror("mkdir");
			return;
		}
		dev_t volume;
		uint32 flags = 0;
		fprintf(stderr, "parms:'%s'\n", parameter.String());
		volume = fs_mount_volume(prettyPath.String(), NULL, "nfs4", flags,
			parameter.String());
		if (volume < B_OK) {
			fprintf(stderr, "fs_mount_volume: %s\n", strerror(volume));
			return;
		}

		BMessage m(B_REFS_RECEIVED);
		entry_ref ref;
		if (get_ref_for_path(prettyPath.String(), &ref) < B_OK)
			return;
		m.AddRef("refs", &ref);
		be_roster->Launch(kTrackerSig, &m);
		return;
	}

	if (proto == "doi") {
		BString url("http://dx.doi.org/");
		BString mimetype;

		url << full;
		BUrl u(url.String());
		args[0] = const_cast<char*>("urlwrapper"); //XXX
		args[1] = (char*)u.UrlString().String();
		args[2] = NULL;
		mimetype = kURLHandlerSigBase;
		mimetype += u.Protocol();

		err = be_roster->Launch(mimetype.String(), 1, args + 1);
		if (err != B_OK && err != B_ALREADY_RUNNING)
			err = be_roster->Launch(kAppSig, 1, args + 1);
		// TODO: handle errors
		return;
	}

	/*

	More ?
	cf. http://en.wikipedia.org/wiki/URI_scheme
	cf. http://www.iana.org/assignments/uri-schemes.html

	Audio: (SoundPlay specific, identical to http:// to a shoutcast server)

	vnc: ?
	irc: ?
	im: http://tools.ietf.org/html/rfc3860

	svn: handled by checkitout
	cvs: handled by checkitout
	git: handled by checkitout
	rsync: handled by checkitout - http://tools.ietf.org/html/rfc5781

	smb: cifsmount ?
	nfs: mount_nfs ? http://tools.ietf.org/html/rfc2224
	ipp: http://tools.ietf.org/html/rfc3510

	mailto: ? Mail & Beam both handle it already (not fully though).
	imap: to describe mail accounts ? http://tools.ietf.org/html/rfc5092
	pop: http://tools.ietf.org/html/rfc2384
	mid: cid: as per RFC 2392
	http://www.rfc-editor.org/rfc/rfc2392.txt query MAIL:cid
	message:<MID> http://daringfireball.net/2007/12/message_urls_leopard_mail

	itps: pcast: podcast: s//http/ + parse xml to get url to mp3 stream...
	audio: s//http:/ + default MediaPlayer
	-- see http://forums.winamp.com/showthread.php?threadid=233130

	gps: ? I should submit an RFC for that one :)

	webcal: (is http: to .ics file)

	data: (but it's dangerous)

	*/


}


status_t
UrlWrapper::_DecodeUrlString(BString& string)
{
	// TODO: check for %00 and bail out!
	int32 length = string.Length();
	int i;
	for (i = 0; string[i] && i < length - 2; i++) {
		if (string[i] == '%' && isxdigit(string[i+1])
			&& isxdigit(string[i+2])) {
			int c;
			sscanf(string.String() + i + 1, "%02x", &c);
			string.Remove(i, 3);
			string.Insert((char)c, 1, i);
			length -= 2;
		}
	}

	return B_OK;
}


void
UrlWrapper::ReadyToRun(void)
{
	Quit();
}


// #pragma mark


int main(int argc, char** argv)
{
	UrlWrapper app;
	if (be_app)
		app.Run();
	return 0;
}

