#include <Application.h>
#include <AppFileInfo.h>
#include <Mime.h>
#include <Message.h>
#include <TypeConstants.h>
#include <Roster.h>
#include <String.h>
#include <stdio.h>
#include <unistd.h>

#define PROTO "telnet"

class TWApp : public BApplication
{
public:
	TWApp();
	~TWApp();
status_t	SplitUrlHostUserPass(const char *url, BString &host, BString &user, BString &pass);
virtual void	ArgvReceived(int32 argc, char **argv);
virtual void	ReadyToRun(void);
private:

};

const char *app_mime = "application/x-vnd.mmu_man.telnetwrapper";
//const char *url_mime = "application/x-vnd.Be.URL.telnet";
const char *url_mime = B_URL_TELNET;
const char *terminal_sig = "application/x-vnd.Be-SHEL";

TWApp::TWApp() : BApplication(app_mime)
{
	BMimeType mt(url_mime);
	if (mt.InitCheck())
		return;
	if (!mt.IsInstalled()) {
		mt.Install();
	}
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

TWApp::~TWApp()
{
}


// XXX: handle ":port" as well
status_t TWApp::SplitUrlHostUserPass(const char *url, BString &host, BString &user, BString &pass)
{
	host = url;
	if (host.FindFirst("@") > -1) {
		printf("%s -- %s\n", host.String(), user.String());
		host.MoveInto(user, 0, host.FindFirst("@"));
		host.Remove(0, 1);
		if (user.FindFirst(":") > -1) {
			user.MoveInto(pass, user.FindFirst(":"), user.Length());
			pass.Remove(0, 1);
			return 3;
		}
		printf("%s -- %s\n", host.String(), user.String());
		return 2;
	}
	return 1;
}

void TWApp::ArgvReceived(int32 argc, char **argv)
{
#if 0
	for (int i = 1; i < argc; i++) {
		//printf("argv[%d]=%s\n", i, argv[i]);
	}
#endif
	if (argc <= 1)
		return;
	
	char *args[] = { "/bin/sh", "-c", NULL, NULL};

	BString host;
	BString user;
	BString pass;

	BString url(argv[1]);
	
	// XXX: should factorize that
	
	if (url.FindFirst("telnet:") == 0) {
		url.RemoveFirst("telnet:");
		if (url.FindFirst("//") == 0)
			url.RemoveFirst("//");
		
		
		const char *failc = " || read -p 'Press any key'";
		BString cmd("telnet ");
		SplitUrlHostUserPass(url.String(), host, user, pass);
		printf("HOST='%s'\n", host.String());
		printf("USER='%s'\n", user.String());
		printf("PASS='%s'\n", pass.String());
		if (user.Length())
			cmd << "-l " << user << " ";
		cmd << host;
		printf("CMD='%s'\n", cmd.String());
		cmd << failc;
		args[2] = (char *)cmd.String();
		be_roster->Launch(terminal_sig, 3, args);
		return;
	}
	
	if (url.FindFirst("ssh:") == 0) {
		url.RemoveFirst("ssh:");
		if (url.FindFirst("//") == 0)
			url.RemoveFirst("//");
		
		
		const char *failc = " || read -p 'Press any key'";
		BString cmd("ssh ");
		
		SplitUrlHostUserPass(url.String(), host, user, pass);
		printf("HOST='%s'\n", host.String());
		printf("USER='%s'\n", user.String());
		printf("PASS='%s'\n", pass.String());
		if (user.Length())
			cmd << "-l " << user << " ";
		cmd << host;
		printf("CMD='%s'\n", cmd.String());
		cmd << failc;
		args[2] = (char *)cmd.String();
		be_roster->Launch(terminal_sig, 3, args);
		// XXX: handle errors
		return;
	}

	if (url.FindFirst("ftp:") == 0) {
		url.RemoveFirst("ftp:");
		if (url.FindFirst("//") == 0)
			url.RemoveFirst("//");
		
		
		const char *failc = " || read -p 'Press any key'";
		BString cmd("ftp ");
		
		/*
		SplitUrlHostUserPass(url.String(), host, user, pass);
		printf("HOST='%s'\n", host.String());
		printf("USER='%s'\n", user.String());
		printf("PASS='%s'\n", pass.String());
		if (user.Length())
			cmd << "-l " << user << " ";
		cmd << host;
		*/
		cmd << url;
		printf("CMD='%s'\n", cmd.String());
		cmd << failc;
		args[2] = (char *)cmd.String();
		be_roster->Launch(terminal_sig, 3, args);
		// XXX: handle errors
		return;
	}
	
	if (url.FindFirst("sftp:") == 0) {
		url.RemoveFirst("sftp:");
		if (url.FindFirst("//") == 0)
			url.RemoveFirst("//");
		
		
		const char *failc = " || read -p 'Press any key'";
		BString cmd("sftp ");
		
		/*
		SplitUrlHostUserPass(url.String(), host, user, pass);
		printf("HOST='%s'\n", host.String());
		printf("USER='%s'\n", user.String());
		printf("PASS='%s'\n", pass.String());
		if (user.Length())
			cmd << "-l " << user << " ";
		cmd << host;
		*/
		cmd << url;
		printf("CMD='%s'\n", cmd.String());
		cmd << failc;
		args[2] = (char *)cmd.String();
		be_roster->Launch(terminal_sig, 3, args);
		// XXX: handle errors
		return;
	}
	
	
	// finger:user@host
	// vnc: ?
	// irc: ?
	// 
	// file: -> Tracker ?
	// svn: ?
	// cvs: ?
	// mms: -> VLC
	// rtsp: -> VLC
	// rtp: -> VLC
	

}

void TWApp::ReadyToRun(void)
{
	Quit();
}

int main(int argc, char **argv)
{
	TWApp app;
	if (be_app)
		app.Run();
	return 0;
}
