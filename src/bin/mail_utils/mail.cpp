/*
 * Copyright 2003-2016, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Santiago (Jacques) Lema
 *		Jérôme Duval, jerome.duval@gmail.com
 *		Augustin Cavalier, <waddlesplash>
 */


#include <Application.h>
#include <String.h>
#include <E-mail.h>

#include <stdio.h>


#define APP_SIG				"application/x-vnd.Haiku-mail_utils-mail"


int main(int argc, char* argv[])
{
	BApplication mailApp(APP_SIG);

	// No arguments, show usage
	if (argc < 2) {
		fprintf(stdout,"This program can only send mail, not read it.\n");
		fprintf(stdout,"usage: %s [-v] [-s subject] [-c cc-addr] "
			"[-b bcc-addr] to-addr ...\n", argv[0]);
		fflush(stdout);
		return 0;
	}

	char *subject = "No title";
	char *cc = "";
	char *bcc = "";
	BString to = "";
	BString body = "";

	bool verbose =false;
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
			to.Append(argv[i]);
			if (i < argc - 1)
				to.Append(" ");
  		}
	}

	if (verbose) {
		fprintf(stdout, "\n");
		fprintf(stdout, "To:\t<%s> \n", to.String());
		fprintf(stdout, "Cc:\t<%s> \n", cc);
		fprintf(stdout, "Bcc:\t<%s> \n", bcc);
		fprintf(stdout, "Subj:\t<%s> \n", subject);
		fprintf(stdout, "Body:\t<%s> \n", body.String());
    	fprintf(stdout, "\n");
	}

	// Check if recipients are valid
    if (strcmp(to.String(), "") == 0 &&
    	strcmp(cc, "") == 0 &&
    	strcmp(bcc, "") == 0) {

		fprintf(stdout, "[Error]: You must specify at least one recipient "
			"in to, cc or bcc fields.\n");
		return -1;
	}

	// Read each line until we get a single dot "." on a line
	char line[32768] = "";

	printf("Now type your message.\nType '.' alone on a line to send it.\n");
	do {
	    gets(line);

	    if (strcmp(line, ".") != 0) {
	    	body.Append(line).Append("\n");
		}
	    // fprintf(stdout,"Line: %s \n",line);
	} while (strcmp(line, ".") != 0);


	if (verbose)
   		fprintf(stdout, "\nBody:\n%s\n", body.String());

	if (verbose)
   		fprintf(stdout, "\nSending E-mail...\n");
	fflush(stdout);

	BMailMessage mail;
	mail.AddHeaderField(B_MAIL_TO, to.String());
	mail.AddHeaderField(B_MAIL_CC, cc);
	mail.AddHeaderField(B_MAIL_BCC, bcc);
	mail.AddHeaderField(B_MAIL_SUBJECT, subject);
	mail.AddContent(body.String(), strlen(body.String()));
	status_t result = mail.Send();

	if (result == B_OK) {
		fprintf(stdout, "\nMessage was sent successfully.\n");
		return 0;
	}

	fprintf(stdout, "Message failed to send: %s", strerror(result));
	return result;
}
