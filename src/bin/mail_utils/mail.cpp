/*
 * Copyright 2003-2016, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Santiago (Jacques) Lema
 *		Jérôme Duval, jerome.duval@gmail.com
 *		Augustin Cavalier, <waddlesplash>
 *		Alexander G. M. Smith <agmsmith@ncf.ca>
 */


#include <stdio.h>
#include <unistd.h>

#include <Application.h>
#include <E-mail.h>
#include <String.h>

#define APP_SIG "application/x-vnd.Haiku-mail_utils-mail"


int main(int argc, char* argv[])
{
	BApplication mailApp(APP_SIG);

	// No arguments, show usage
	if (argc < 2) {
		fprintf(stdout,"This program can only send mail, not read it.\n");
		fprintf(stdout,"usage: %s [-v] [-s subject] [-c cc-addr] "
			"[-b bcc-addr] to-addr ...\n", argv[0]);
		return 0;
	}

	char *subject = "No subject";
	char *cc = "";
	char *bcc = "";
	BString to;
	bool verbose = false;

	// Parse arguments
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-v") == 0)
			verbose = true;
		else if (strcmp(argv[i], "-s") == 0) {
			subject = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-c") == 0) {
			cc = argv[i+1];
			i++;
 		} else if (strcmp(argv[i], "-b") == 0) {
 			bcc = argv[i+1];
 			i++;
		} else {
			if (to.Length() > 0)
				to.Append(", ");
			to.Append(argv[i]);
		}
	}

	if (verbose) {
		fprintf(stdout, "\n");
		fprintf(stdout, "To:\t%s\n", to.String());
		fprintf(stdout, "Cc:\t%s\n", cc);
		fprintf(stdout, "Bcc:\t%s\n", bcc);
		fprintf(stdout, "Subj:\t%s\n", subject);
		fprintf(stdout, "\n");
	}

	// Check if recipients are valid
	if (strcmp(to.String(), "") == 0 &&
		strcmp(cc, "") == 0 &&
		strcmp(bcc, "") == 0) {

		fprintf(stderr, "[Error]: You must specify at least one recipient "
			"in to, cc or bcc fields.\n");
		return -1;
	}

	bool isTerminal = isatty(STDIN_FILENO) != 0;
	if (isTerminal) {
		fprintf(stderr, "Now type your message.\n"
			"Type '.' alone on a line to end your text and send it.\n");
	}

	BString body;
	char line[32768] = "";

	// Read each line and collect the body text until we get an end of text
	// marker.  That's a single dot "." on a line typed in by the user,
	// or end of file when reading a file.
	do {
		if (fgets(line, sizeof(line), stdin) == NULL) {
			// End of file or an error happened, just send collected body text.
			break;
		}

		if (isTerminal && strcmp(line, ".\n") == 0)
			break;

		body.Append(line);
	} while (true);

	if (verbose)
		fprintf(stdout, "\nBody:\n%s\n", body.String());

	if (verbose)
		fprintf(stderr, "Sending E-mail...\n");
	fflush(stdout);

	BMailMessage mail;
	mail.AddHeaderField(B_MAIL_TO, to.String());
	mail.AddHeaderField(B_MAIL_CC, cc);
	mail.AddHeaderField(B_MAIL_BCC, bcc);
	mail.AddHeaderField(B_MAIL_SUBJECT, subject);
	mail.AddContent(body.String(), body.Length());
	status_t result = mail.Send();

	if (result == B_OK) {
		if (verbose)
			fprintf(stderr, "Message was sent successfully.\n");
		return 0;
	}

	fprintf(stderr, "Message failed to send: %s\n", strerror(result));
	return result;
}
