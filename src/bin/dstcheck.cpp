/* 
 * Copyright 2004, Jérôme Duval, jerome.duval@free.fr.
 * Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Application.h>
#include <FindDirectory.h>
#include <MessageRunner.h>
#include <Roster.h>
#include <String.h>
#include <TextView.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const uint32 TIMEDALERT_UPDATE = 'taup';

class TimedAlert : public BAlert
{
public:
	TimedAlert(const char *title, const char *text, const char *button1,
		const char *button2 = NULL, const char *button3 = NULL, 
		button_width width = B_WIDTH_AS_USUAL,  alert_type type = B_INFO_ALERT);
	void MessageReceived(BMessage *);
	void Show();
	static void GetLabel(BString &string);
private:
	BMessageRunner *fRunner;
};

#define STRING1 "Attention!\n\nBecause of the switch from daylight saving time,\nyour \
computer's clock may be an hour off. Currently,\nyour computer thinks it is "
#define STRING2 ".\n\nIs this the correct time?"

TimedAlert::TimedAlert(const char *title, const char *text, const char *button1,
	const char *button2, const char *button3,
	button_width width,  alert_type type)
	: BAlert(title, text, button1, button2, button3, width, type),
	fRunner(NULL)
{
	
}


void
TimedAlert::Show()
{
	fRunner = new BMessageRunner(BMessenger(this), new BMessage(TIMEDALERT_UPDATE), 60000000);
	BAlert::Show();
}


void 
TimedAlert::MessageReceived(BMessage *msg)
{
	if (msg->what == TIMEDALERT_UPDATE) {
		BString string;
		GetLabel(string);	
		this->TextView()->SetText(string.String());
	} else
		BAlert::MessageReceived(msg);
}

void
TimedAlert::GetLabel(BString &string)
{
	string = STRING1;
	time_t t;
	struct tm tm;
	char timestring[15];
	time(&t);
	localtime_r(&t, &tm);
	strftime(timestring, 15, "%I:%M %p", &tm);
	string += timestring;
	string += STRING2;
}


int
main(int argc, char **argv)
{
	time_t t;
	struct tm tm;
	tzset();
	time(&t);
	localtime_r(&t, &tm);	

	char path[B_PATH_NAME_LENGTH];	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, -1, true, path, B_PATH_NAME_LENGTH) != B_OK) {
		fprintf(stderr, "%s: can't find settings directory\n", argv[0]);
		exit(1);
	}

	strcat(path, "/time_dststatus");
	bool newFile = false;
	bool dst = false;
	int fd = open(path, O_RDWR | O_EXCL | O_CREAT);
	if (fd < 0) {
		newFile = false;
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

	if (dst != tm.tm_isdst) {

		BApplication app("application/x-vnd.Haiku-cmd-dstconfig");	

		BString string;
		TimedAlert::GetLabel(string);

		int32 index = (new TimedAlert("timedAlert", string.String(), "Ask me later", "Yes", "No"))->Go();
		if (index == 0)
			exit(0);

		if (index == 2) {
			index = (new BAlert("dstcheck", "Would you like to set the clock using the Time and\nDate preference utility?", 
				"No", "Yes"))->Go();

			if (index == 1)
				be_roster->Launch("application/x-vnd.Haiku-TIME");
		}
	}

	lseek(fd, 0, SEEK_SET);
	char dst_byte = tm.tm_isdst ? '1' : '0';
	write(fd, &dst_byte, 1);
	close(fd);
	
	return 0;	
}

