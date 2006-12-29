/*
 * Copyright 2002-2006, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mathew Hounsell
 *		Vasilis Kaoutsis, kaoutsis@sch.gr
 */


#include <Alert.h>
#include <Application.h>

#include <stdio.h>
#include <string.h>


const char* kSignature = "application/x-vnd.Haiku.cmd-alert";

const char* kButtonDefault = "OK";

const int32 kErrorInitFail = 127;
const int32 kErrorArgumentsFail = 126;


class AlertApplication : public BApplication {
	public:
		AlertApplication();
		virtual ~AlertApplication() { }

		virtual void ReadyToRun();
		virtual void ArgvReceived(int32 argc, char** argv);

		bool GoodArguments() const { return fOk; }
		int32 ChoiceNumber() const { return fChoiceNumber; }
		char const* ChoiceText() const { return fChoiceText; }

	private:
		void _SetChoice(int32 buttonIndex);
		void _Usage() const;

	private:
		bool fOk;
		bool fModal;
		alert_type fIcon;
		char* fArgumentText;
		char* fArgumentButton0;
		char* fArgumentButton1;
		char* fArgumentButton2;
		char* fChoiceText;
		int32 fChoiceNumber; 
};


AlertApplication::AlertApplication()
	: BApplication(kSignature),
	fOk(false),
	fModal(false),
	fIcon(B_INFO_ALERT),
	fArgumentText(NULL),
	fArgumentButton0(NULL),
	fArgumentButton1(NULL),
	fArgumentButton2(NULL),
	fChoiceText(NULL),
	fChoiceNumber(0)
{
}


/*!
	Invoked when the application receives a B_ARGV_RECEIVED message.
	The message is sent when command line arguments are used in launching the
	app from the shell. The function isn't called if there were no command
	line arguments.
*/
void
AlertApplication::ArgvReceived(int32 argc, char** argv)
{
	// Now there is at least one
	// command line argument option.

	const uint32 kArgCount = argc - 1;
	uint32 index = 1;
	bool iconFlag = false;

	// Look for valid '--' options.		
	for (; index <= kArgCount; ++index) {
		if (argv[index][0] == '-' && argv[index][1] == '-') {
			const char* option = argv[index] + 2;

			if (!strcmp(option, "modal"))
				fModal = true;
			else if (!strcmp(option, "empty") && !iconFlag) {
				fIcon = B_EMPTY_ALERT;
				iconFlag = true;
			} else if (!strcmp(option, "info") && !iconFlag) {
				fIcon = B_INFO_ALERT;
				iconFlag = true;
			} else if (!strcmp(option, "idea") && !iconFlag) {
				fIcon = B_IDEA_ALERT;
				iconFlag = true;
			} else if (!strcmp(option, "warning") && !iconFlag) {
				fIcon = B_WARNING_ALERT;
				iconFlag = true;
			} else if (!strcmp(option, "stop") && !iconFlag) {
				fIcon = B_STOP_ALERT;
				iconFlag = true;
			} else {
				// Unrecognized '--' opition.
				fprintf(stdout, "Unrecognized option %s\n", argv[index]);
				return;
			}
		} else {
			// Option doesn't start with '--'
			break;
		}
		
		if (index == kArgCount) {
			// User provides arguments that all begins with '--',
			// so none can be considered as text argument.
			fprintf(stdout, "Missing the text argument!\n");
			return;
		}
	}

	fArgumentText = strdup(argv[index]);
		// First argument that start without --,
		// so grub it as the text argument.

	if (index == kArgCount) {
		// No more text argument. Give Button0
		// the default label.
		fArgumentButton0 = strdup(kButtonDefault);
		fOk = true;
		return;
	}

	if (index < kArgCount) {
		// There is another argument,
		// so let that be the Button0 label.
		fArgumentButton0 = strdup(argv[++index]);
	}

	if (index < kArgCount) {
		// There is another argument,
		// so let that be the Button1 label.
		fArgumentButton1 = strdup(argv[++index]);
	}	

	if (index < kArgCount) {
		// There is another argument,
		// so let that be the Button2 label.
		fArgumentButton2 = strdup(argv[++index]);
	}

	// Ignore all other arguments if they exist,
	// since they are useless.

	fOk = true;
}


void
AlertApplication::_SetChoice(int32 buttonIndex)
{
	fChoiceNumber = buttonIndex;
	switch (fChoiceNumber) {
		case 0:
			fChoiceText = fArgumentButton0;
			break;

		case 1:
			fChoiceText = fArgumentButton1;
			break;

		case 2:
			fChoiceText = fArgumentButton2;
			break;
	}
}


void
AlertApplication::_Usage() const
{
	fprintf(stderr,
		"usage: alert [ <type> ] [ --modal ] [ --help ] text [ button1 [ button2 [ button3 ]]]\n"
		"<type> is --empty | --info | --idea | --warning | --stop\n"
		"--modal makes the alert system modal and shows it in all workspaces.\n"
		"If any button argument is given, exit status is button number (starting with 0)\n"
		"and 'alert' will print the title of the button pressed to stdout.\n");
}


/*!
	Is called when the app receives a B_READY_TO_RUN message. The message
	is sent automatically during the Run() function, and is sent after the
	initial B_REFS_RECEIVED and B_ARGV_RECEIVED messages (if any) have been
	handled.
*/
void
AlertApplication::ReadyToRun()
{
	if (GoodArguments()) {
		BAlert* alert = new BAlert("alert", fArgumentText,
			fArgumentButton0, fArgumentButton1, fArgumentButton2,
			B_WIDTH_AS_USUAL, fIcon);

		if (fModal)
			alert->SetFeel(B_MODAL_ALL_WINDOW_FEEL);

		_SetChoice(alert->Go());
	} else
		_Usage();

	Quit();
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	AlertApplication app;
	if (app.InitCheck() != B_OK)
		return kErrorInitFail;

	app.Run();
	if (!app.GoodArguments())
		return kErrorArgumentsFail;

	fprintf(stdout, "%s\n", app.ChoiceText());
	return app.ChoiceNumber();
}
