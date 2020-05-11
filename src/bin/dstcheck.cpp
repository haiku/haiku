/*
 * Copyright 2004, Jérôme Duval, jerome.duval@free.fr.
 * Distributed under the terms of the MIT License.
 */


#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <MessageRunner.h>
#include <Roster.h>
#include <String.h>
#include <TextView.h>
#include <TimeFormat.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "dstcheck"


const uint32 TIMEDALERT_UPDATE = 'taup';

class TimedAlert : public BAlert {
public:
	TimedAlert(const char *title, const char *text, const char *button1,
		const char *button2 = NULL, const char *button3 = NULL,
		button_width width = B_WIDTH_AS_USUAL,
		alert_type type = B_INFO_ALERT);
	void MessageReceived(BMessage *);
	void Show();

	static void GetLabel(BString &string);

private:
	BMessageRunner *fRunner;
};


TimedAlert::TimedAlert(const char *title, const char *text, const char *button1,
		const char *button2, const char *button3,
		button_width width, alert_type type)
	: BAlert(title, text, button1, button2, button3, width, type),
	fRunner(NULL)
{
	SetShortcut(0, B_ESCAPE);
}


void
TimedAlert::Show()
{
	fRunner
		= new BMessageRunner(this, new BMessage(TIMEDALERT_UPDATE), 60000000);
	SetFeel(B_FLOATING_ALL_WINDOW_FEEL);
	BAlert::Show();
}


void
TimedAlert::MessageReceived(BMessage *msg)
{
	if (msg->what == TIMEDALERT_UPDATE) {
		BString string;
		GetLabel(string);
		TextView()->SetText(string.String());
	} else {
		BAlert::MessageReceived(msg);
	}
}


void
TimedAlert::GetLabel(BString &string)
{
	string = B_TRANSLATE("Attention!\n\nBecause of the switch from daylight "
		"saving time, your computer's clock may be an hour off.\n"
		"Your computer thinks it is %current time%.\n\nIs this the correct "
		"time?");

	time_t t;
	struct tm tm;
	char timestring[15];
	time(&t);
	localtime_r(&t, &tm);

	BTimeFormat().Format(timestring, 15, t, B_SHORT_TIME_FORMAT);

	string.ReplaceFirst("%current time%", timestring);
}


//	#pragma mark -


int
main(int argc, char **argv)
{
	time_t t;
	struct tm tm;
	tzset();
	time(&t);
	localtime_r(&t, &tm);

	if (tm.tm_year < (2020 - 1900)) {
		fprintf(stderr, "%s: not checking because clock is not set.\n",
			argv[0]);
		exit(1);
	}

	char path[B_PATH_NAME_LENGTH];
	if (find_directory(B_USER_SETTINGS_DIRECTORY, -1, true, path,
			B_PATH_NAME_LENGTH) != B_OK) {
		fprintf(stderr, "%s: can't find settings directory\n", argv[0]);
		exit(1);
	}

	strcat(path, "/time_dststatus");
	bool dst = false;
	int fd = open(path, O_RDWR | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		fd = open(path, O_RDWR);
		if (fd < 0) {
			perror("couldn't open dst status settings file");
			exit(1);
		}

		char dst_byte;
		read(fd, &dst_byte, 1);

		dst = dst_byte == '1';
	} else {
		dst = tm.tm_isdst;
	}

	if (dst != tm.tm_isdst || argc > 1) {
		BApplication app("application/x-vnd.Haiku-cmd-dstconfig");

		BString string;
		TimedAlert::GetLabel(string);

		int32 index = (new TimedAlert("timedAlert", string.String(),
			B_TRANSLATE("Keep this time"), B_TRANSLATE("Ask me later"),
			B_TRANSLATE("Manually adjust time" B_UTF8_ELLIPSIS)))->Go();
		if (index == 1)
			exit(0);

		if (index == 2)
			be_roster->Launch("application/x-vnd.Haiku-Time");
	}

	lseek(fd, 0, SEEK_SET);
	char dst_byte = tm.tm_isdst ? '1' : '0';
	write(fd, &dst_byte, 1);
	close(fd);

	return 0;
}
