#include <Alert.h>
#include <Application.h>
#include <AppFileInfo.h>
#include <Mime.h>
#include <Message.h>
#include <TypeConstants.h>
#include <Roster.h>
#include <String.h>
#include <stdio.h>
#include <unistd.h>

#define HANDLE_FILE
#define HANDLE_SH
#define HANDLE_BESHARE
//#define HANDLE_IM
#define HANDLE_VLC

const char *kAppSig = "application/x-vnd.haiku.urlwrapper";

#ifdef __HAIKU__
const char *kTerminalSig = "application/x-vnd.Haiku-Terminal";
#else
const char *kTerminalSig = "application/x-vnd.Be-SHEL";
#endif

#ifdef HANDLE_FILE
const char *kTrackerSig = "application/x-vnd.Be-TRAK";
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

// TODO: make a Url class
class Url : public BString {
public:
		Url(const char *url) : BString(url) { fStatus = ParseAndSplit(); };
		~Url() {};
status_t	InitCheck() const { return fStatus; };
status_t	ParseAndSplit();

bool		HasHost() const { return host.Length(); };
bool		HasPort() const { return port.Length(); };
bool		HasUser() const { return user.Length(); };
bool		HasPass() const { return pass.Length(); };
bool		HasPath() const { return path.Length(); };
BString		Proto() const { return BString(proto); };
BString		Host() const { return BString(host); };
BString		Port() const { return BString(port); };
BString		User() const { return BString(user); };
BString		Pass() const { return BString(pass); };

BString		proto;
BString		host;
BString		port;
BString		user;
BString		pass;
BString		path;
private:
status_t	fStatus;
};

class UrlWrapperApp : public BApplication
{
public:
	UrlWrapperApp();
	~UrlWrapperApp();
status_t	SplitUrl(const char *url, BString &host, BString &port, BString &user, BString &pass, BString &path);
status_t	UnurlString(BString &s);
status_t	Warn(const char *url);
virtual void	ArgvReceived(int32 argc, char **argv);
virtual void	ReadyToRun(void);
private:

};

// TODO: handle ":port" as well
// TODO: handle "/path" as well
// proto:[//]user:pass@host:port/path
status_t Url::ParseAndSplit()
{
	int32 v;
	//host = *this;
	BString left;
printf("s:%s\n", String());

	v = FindFirst(":");
	if (v < 0)
		return EINVAL;
	
	CopyInto(proto, 0, v);
	CopyInto(left, v + 1, Length() - v);
	if (left.FindFirst("//") == 0)
		left.RemoveFirst("//");
	
	// path part
	v = left.FindFirst("/");
	if (v == 0 || proto == "file") {
		path = left;
printf("path:%s\n", path.String());
		return 0;
	}
	
	if (v > -1) {
		left.MoveInto(path, v+1, left.Length()-v);
		left.Remove(v, 1);
	}
printf("path:%s\n", path.String());

	// user:pass@host
	v = left.FindFirst("@");
	if (v > -1) {
		left.MoveInto(user, 0, v);
		left.Remove(0, 1);
printf("user:%s\n", user.String());
		v = user.FindFirst(":");
		if (v > -1) {
			user.MoveInto(pass, v, user.Length() - v);
			pass.Remove(0, 1);
printf("pass:%s\n", pass.String());
		}
	}

	// host:port
	v = left.FindFirst(":");
	if (v > -1) {
		left.MoveInto(port, v + 1, left.Length() - v);
		left.Remove(v, 1);
printf("port:%s\n", port.String());
	}

	// not much left...
	host = left;
printf("host:%s\n", host.String());

	return 0;
}

status_t UrlWrapperApp::SplitUrl(const char *url, BString &host, BString &port, BString &user, BString &pass, BString &path)
{
	Url u(url);
	if (u.InitCheck() < 0)
		return u.InitCheck();
	host = u.host;
	port = u.port;
	user = u.user;
	pass = u.pass;
	path = u.path;
	return 0;
}

UrlWrapperApp::UrlWrapperApp() : BApplication(kAppSig)
{
#if 0
	BMimeType mt(B_URL_TELNET);
	if (mt.InitCheck())
		return;
	if (!mt.IsInstalled()) {
		mt.Install();
	}
#endif
#if 0
	BAppFileInfo afi;
	if (!afi.Supports(&mt)) {
		//printf("adding support for telnet url\n");
		BMessage typemsg;
		typemsg.AddString("types", url_mime);
		afi.SetSupportedTypes(&typemsg, true);
	}
#endif
}

UrlWrapperApp::~UrlWrapperApp()
{
}



status_t UrlWrapperApp::UnurlString(BString &s)
{
	// TODO:WRITEME
	return B_OK;
}

status_t UrlWrapperApp::Warn(const char *url)
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


void UrlWrapperApp::ArgvReceived(int32 argc, char **argv)
{
#if 0
	for (int i = 1; i < argc; i++) {
		//printf("argv[%d]=%s\n", i, argv[i]);
	}
#endif
	if (argc <= 1)
		return;
	
	const char *failc = " || read -p 'Press any key'";
	const char *pausec = "; read -p 'Press any key'";
	char *args[] = { "/bin/sh", "-c", NULL, NULL};

	BString proto;
	BString host;
	BString port;
	BString user;
	BString pass;
	BString path;

	Url u(argv[1]);
	BString rawurl(argv[1]);
	BString url = rawurl;
	if (url.FindFirst(":") < 0) {
		fprintf(stderr, "malformed url: '%s'\n", url.String());
		return;
	}
	url.MoveInto(proto, 0, url.FindFirst(":"));
	url.Remove(0, 1);
	if (url.FindFirst("//") == 0)
		url.RemoveFirst("//");
	
	// pre-slice the url, but you're not forced to use the result.
	// original still in rawurl.
	SplitUrl(u.String(), host, port, user, pass, path);
	
	// XXX: debug
	printf("PROTO='%s'\n", proto.String());
	printf("HOST='%s'\n", host.String());
	printf("PORT='%s'\n", port.String());
	printf("USER='%s'\n", user.String());
	printf("PASS='%s'\n", pass.String());
	printf("PATH='%s'\n", path.String());
	
	if (proto == "telnet") {
		BString cmd("telnet ");
		if (user.Length())
			cmd << "-l " << user << " ";
		cmd << host;
		printf("CMD='%s'\n", cmd.String());
		cmd << failc;
		args[2] = (char *)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		return;
	}
	
	if (proto == "ssh") {
		BString cmd("ssh ");
		
		if (user.Length())
			cmd << "-l " << user << " ";
		cmd << host;
		printf("CMD='%s'\n", cmd.String());
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
		cmd << url;
		printf("CMD='%s'\n", cmd.String());
		cmd << failc;
		args[2] = (char *)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}
	
	if (proto == "sftp") {
		BString cmd("sftp ");
		
		/*
		if (user.Length())
			cmd << "-l " << user << " ";
		cmd << host;
		*/
		cmd << url;
		printf("CMD='%s'\n", cmd.String());
		cmd << failc;
		args[2] = (char *)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}

	if (proto == "finger") {
		BString cmd("finger ");
		
		// TODO: SplitUrl thinks the user is host when it's not present... FIXME.
		if (user.Length())
			cmd << user;
		if (host.Length() == 0)
			host = "127.0.0.1";
		cmd << "@" << host;
		printf("CMD='%s'\n", cmd.String());
		cmd << pausec;
		args[2] = (char *)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		// TODO: handle errors
		return;
	}

#ifdef HANDLE_FILE
	if (proto == "file") {
		BMessage m(B_REFS_RECEIVED);
		entry_ref ref;
		// UnurlString(path);
		if (get_ref_for_path(path.String(), &ref) < B_OK)
			return;
		m.AddRef("refs", &ref);
		be_roster->Launch(kTrackerSig, &m);
		return;
	}
#endif

#ifdef HANDLE_SH
	if (proto == "sh") {
		BString cmd(url);
		if (Warn(rawurl.String()) != B_OK)
			return;
		printf("CMD='%s'\n", cmd.String());
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
		be_roster->Launch(kBeShareSig, (BMessage *)NULL, &team);
		BMessenger msgr(NULL, team);
		if (host.Length()) {
			BMessage mserver('serv');
			mserver.AddString("server", host);
			//msgs.AddItem(&mserver);
			msgr.SendMessage(mserver);
			
		}
		if (path.Length()) {
			BMessage mquery('quer');
			mquery.AddString("query", path);
			//msgs.AddItem(&mquery);
			msgr.SendMessage(mquery);
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
		if (host.Length()) {
			BMessage mserver(B_REFS_RECEIVED);
			mserver.AddString("server", host);
			//msgs.AddItem(&mserver);
			msgr.SendMessage(mserver);
			
		}
		// TODO: handle errors
		return;
	}
#endif

#ifdef HANDLE_VLC
	if (proto == "mms" || proto == "rtp" || proto == "rtsp") {
		args[0] = "vlc";
		args[1] = (char *)rawurl.String();
		be_roster->Launch(kVLCSig, 2, args);
		return;
	}
#endif
	
	// vnc: ?
	// irc: ?
	// 
	// svn: ?
	// cvs: ?
	// smb: ?
	// nfs: ?

}

void UrlWrapperApp::ReadyToRun(void)
{
	Quit();
}

int main(int argc, char **argv)
{
	UrlWrapperApp app;
	if (be_app)
		app.Run();
	return 0;
}
