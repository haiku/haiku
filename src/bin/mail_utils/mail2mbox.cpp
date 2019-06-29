/*
 * Copyright 2005-2009, Haiku Inc.
 * This file may be used under the terms of the MIT License.
 *
 * Originally public domain written by Alexander G. M. Smith.
 */


/*!	BeMailToMBox is a utility program (requested by Frank Zschockelt) that
	converts BeOS e-mail files into Unix mailbox files (the kind that Pine
	uses).  All the files in the input directory are concatenated with the
	appropriate mbox header lines added between them, and trailing blank lines
	reduced.  The resulting text is written to standard output.  Command line
	driven.
*/

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <Application.h>
#include <StorageKit.h>
#include <SupportKit.h>


extern const char* __progname;
static const char* kProgramName = __progname;

time_t gDateStampTime;
	// Time value used for stamping each message header. Incremented by 1 second
	// for each message, starts out with the current local time.


/*!	Global utility function to display an error message and return.  The message
	part describes the error, and if errorNumber is non-zero, gets the string
	", error code $X (standard description)." appended to it.  If the message
	is NULL then it gets defaulted to "Something went wrong".
*/
static void
DisplayErrorMessage(const char* messageString = NULL, status_t errorNumber = 0,
	const char* titleString = NULL)
{
	char errorBuffer[2048];

	if (titleString == NULL)
		titleString = "Error Message:";

	if (messageString == NULL) {
		if (errorNumber == B_OK)
			messageString = "No error, no message, why bother?";
		else
			messageString = "Error";
	}

	if (errorNumber != 0) {
		snprintf(errorBuffer, sizeof(errorBuffer), "%s: %s (%" B_PRIx32 ")"
			"has occured.", messageString, strerror(errorNumber), errorNumber);
		messageString = errorBuffer;
	}

	fputs(titleString, stderr);
	fputc('\n', stderr);
	fputs(messageString, stderr);
	fputc('\n', stderr);
}


/*!	Determine if a line of text is the start of another message.  Pine mailbox
	files have messages that start with a line that could say something like
	"From agmsmith@achilles.net Fri Oct 31 21:19:36 EST 1997" or maybe something
	like "From POPmail Mon Oct 20 21:12:36 1997" or in a more modern format,
	"From agmsmith@achilles.net Tue Sep 4 09:04:11 2001 -0400".  I generalise it
	to "From blah Day MMM NN XX:XX:XX TZONE1 YYYY TZONE2".  Blah is an e-mail
	address you can ignore (just treat it as a word separated by spaces).  Day
	is a 3 letter day of the week.  MMM is a 3 letter month name.  NN is the two
	digit day of the week, has a leading space if the day is less than 10.
	XX:XX:XX is the time, the X's are digits.  TZONE1 is the old style optional
	time zone of 3 capital letters.  YYYY is the four digit year.  TZONE2 is the
	optional modern time zone info, a plus or minus sign and 4 digits.  Returns
	true if the line of text (ended with a NUL byte, no line feed or carriage
	returns at the end) is the start of a message.
*/
bool
IsStartOfMailMessage(char* lineString)
{
	// It starts with "From "
	if (memcmp("From ", lineString, 5) != 0)
		return false;

	char* string = lineString + 4;
	while (*string == ' ')
		string++;

	// Skip over the e-mail address (or stop at the end of string).

	while (*string != ' ' && *string != 0)
		string++;
	while (*string == ' ')
		string++;

	// TODO: improve this!!!

	// Look for the 3 letter day of the week.
	if (memcmp(string, "Mon", 3) != 0 && memcmp(string, "Tue", 3) != 0
		&& memcmp(string, "Wed", 3) != 0 && memcmp(string, "Thu", 3) != 0
		&& memcmp(string, "Fri", 3) != 0 && memcmp(string, "Sat", 3) != 0
		&& memcmp(string, "Sun", 3) != 0) {
		fprintf(stderr, "False alarm, not a valid day of the week in \"%s\""
			".\n", lineString);
		return false;
	}

	string += 3;
	while (*string == ' ')
		string++;

	// Look for the 3 letter month code.
	if (memcmp(string, "Jan", 3) != 0 && memcmp(string, "Feb", 3) != 0
		&& memcmp(string, "Mar", 3) != 0 && memcmp(string, "Apr", 3) != 0
		&& memcmp(string, "May", 3) != 0 && memcmp(string, "Jun", 3) != 0
		&& memcmp(string, "Jul", 3) != 0 && memcmp(string, "Aug", 3) != 0
		&& memcmp(string, "Sep", 3) != 0 && memcmp(string, "Oct", 3) != 0
		&& memcmp(string, "Nov", 3) != 0 && memcmp(string, "Dec", 3) != 0) {
		fprintf(stderr, "False alarm, not a valid month name in \"%s\".\n",
			lineString);
		return false;
	}

	string += 3;
	while (*string == ' ')
		string++;

	// Skip the day of the month.  Require at least one digit.
	if (*string < '0' || *string > '9') {
		fprintf(stderr, "False alarm, not a valid day of the "
			"month number in \"%s\".\n", lineString);
		return false;
	}

	while (*string >= '0' && *string <= '9')
		string++;
	while (*string == ' ')
		string++;

	// Check the time.  Look for the sequence
	// digit-digit-colon-digit-digit-colon-digit-digit.

	if (string[0] < '0' || string[0] > '9'
		|| string[1] < '0' || string[1] > '9'
		|| string[2] != ':'
		|| string[3] < '0' || string[3] > '9'
		|| string[4] < '0' || string[4] > '9'
		|| string[5] != ':'
		|| string[6] < '0' || string[6] > '9'
		|| string[7] < '0' || string[7] > '9') {
		fprintf(stderr, "False alarm, not a valid time value in \"%s\".\n",
			lineString);
		return false;
	}

	string += 8;
	while (*string == ' ')
		string++;

	// Look for the optional antique 3 capital letter time zone and skip it.
	if (string[0] >= 'A' && string[0] <= 'Z'
		&& string[1] >= 'A' && string[1] <= 'Z'
		&& string[2] >= 'A' && string[2] <= 'Z') {
		string += 3;
		while (*string == ' ')
			string++;
	}

	// Look for the 4 digit year.
	if (string[0] < '0' || string[0] > '9'
		|| string[1] < '0' || string[1] > '9'
		|| string[2] < '0' || string[2] > '9'
		|| string[3] < '0' || string[3] > '9') {
		fprintf(stderr, "False alarm, not a valid 4 digit year in \"%s\".\n",
			lineString);
		return false;
	}

	string += 4;
	while (*string == ' ')
		string++;

	// Look for the optional modern time zone and skip over it if present.
	if ((string[0] == '+' || string[0] == '-')
		&& string[1] >= '0' && string[1] <= '9'
		&& string[2] >= '0' && string[2] <= '9'
		&& string[3] >= '0' && string[3] <= '9'
		&& string[4] >= '0' && string[4] <= '9') {
		string += 5;
		while (*string == ' ')
			string++;
	}

	// Look for end of string.
	if (*string != 0) {
		fprintf(stderr, "False alarm, extra stuff after the "
			"year/time zone in \"%s\".\n", lineString);
		return false;
	}

	return true;
}


/*!	Read the input file, convert it to mbox format, and write it to standard
	output.  Returns zero if successful, a negative error code if an error
	occured.
*/
status_t
ProcessMessageFile(char* fileName)
{
	fprintf(stdout, "Now processing: \"%s\"\n", fileName);

	FILE* inputFile = fopen(fileName, "rb");
	if (inputFile == NULL) {
		DisplayErrorMessage("Unable to open file", errno);
		return errno;
	}

	// Extract a text message from the Mail file.

	BString messageText;
	int lineNumber = 0;

	while (!feof(inputFile)) {
		// First read in one line of text.
		char line[102400];
		if (fgets(line, sizeof(line), inputFile) == NULL) {
			if (ferror(inputFile)) {
				char errorString[2048];
				snprintf(errorString, sizeof(errorString),
					"Error while reading from \"%s\"", fileName);
				DisplayErrorMessage(errorString, errno);
				fclose(inputFile);
				return errno;
			}
			break;
				// No error, just end of file.
		}

		// Remove any trailing control characters (line feed usually, or CRLF).
		// Might also nuke trailing tabs too. Doesn't usually matter. The main
		// thing is to allow input files with both LF and CRLF endings (and
		// even CR endings if you come from the Macintosh world).

		char* string = line + strlen(line) - 1;
		while (string >= line && *string < 32)
			string--;
		*(++string) = 0;

		if (lineNumber == 0 && line[0] == 0) {
			// Skip leading blank lines.
			continue;
		}
		lineNumber++;

		// Prepend the new mbox message header, if the first line of the message
		// doesn't already have one.
		if (lineNumber == 1 && !IsStartOfMailMessage(line)) {
			time_t timestamp = gDateStampTime++;
			messageText.Append("From baron@be.com ");
			messageText.Append(ctime(&timestamp));
		}

		// Append the line to the current message text.
		messageText.Append(line);
		messageText.Append("\n");
	}

	// Remove blank lines from the end of the message (a pet peeve of mine), but
	// end the message with two new lines to separate it from the next message.
	int i = messageText.Length();
	while (i > 0 && (messageText[i - 1] == '\n' || messageText[i - 1] == '\r'))
		i--;
	messageText.Truncate(i);
	messageText.Append("\n\n");

	// Write the message out.

	status_t status = B_OK;

	if (puts(messageText.String()) < 0) {
		DisplayErrorMessage ("Error while writing the message", errno);
		status = errno;
	}

	fclose(inputFile);
	return status;
}


int
main(int argc, char** argv)
{
	BApplication app("application/x-vnd.Haiku-mail2mbox");

	if (argc <= 1 || argc >= 3) {
		printf("%s is a utility for converting Mail e-mail\n", argv[0]);
		printf("files to Unix Pine style e-mail files.  It could well\n");
		printf("work with other Unix style mailbox files.  Each message in\n");
		printf("the input directory is converted and sent to the standard\n");
		printf("output.  Usage:\n\n");
		printf("%s InputDirectory >OutputFile\n\n", kProgramName);
		printf("Public domain, by Alexander G. M. Smith.\n");
		return -10;
	}

	// Set the date stamp to the current time.
	gDateStampTime = time (NULL);

	// Try to open the input directory.
	char inputPathName[B_PATH_NAME_LENGTH];
	strlcpy(inputPathName, argv[1], sizeof(inputPathName) - 2);

	char tempString[2048];

	DIR* dir = opendir(inputPathName);
	if (dir == NULL) {
		sprintf(tempString, "Problems opening directory named \"%s\".",
			inputPathName);
		DisplayErrorMessage(tempString, errno);
		return 1;
	}

	// Append a trailing slash to the directory name, if it needs one.
	if (inputPathName[strlen(inputPathName) - 1] != '/')
		strcat(inputPathName, "/");

	int messagesDoneCount = 0;
	status_t status = B_OK;

	while (dirent_t* entry = readdir(dir)) {
		// skip '.' and '..'
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			break;

		strlcpy(tempString, inputPathName, sizeof(tempString));
		strlcat(tempString, entry->d_name, sizeof(tempString));

		status = ProcessMessageFile(tempString);
		if (status != B_OK)
			break;

		messagesDoneCount++;
	}

	closedir(dir);

	if (status != B_OK) {
		DisplayErrorMessage("Stopping early because an error occured", status);
		return status;
	}

	fprintf(stderr, "Did %d messages successfully.\n", messagesDoneCount);
	return 0;
}
