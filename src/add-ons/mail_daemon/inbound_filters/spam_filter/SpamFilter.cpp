/******************************************************************************
 * AGMSBayesianSpamFilter - Uses Bayesian statistics to evaluate the spaminess
 * of a message.  The evaluation is done by a separate server, this add-on just
 * gets the text and uses scripting commands to get an evaluation from the
 * server.  If the server isn't running, it will be found and started up.  Once
 * the evaluation has been received, it is added to the message as an attribute
 * and optionally as an addition to the subject.  Some other add-on later in
 * the pipeline will use the attribute to delete the message or move it to some
 * other folder.
 *
 * Public Domain 2002, by Alexander G. M. Smith, no warranty.
 *
 * $Log: SpamFilter.cpp,v $
 * Revision 1.1  2004/10/30 22:23:26  brunoga
 * AGMS Spam Filter.
 *
 * Revision 1.19  2004/09/20 15:57:30  nwhitehorn
 * Mostly updated the tree to Be/Haiku style identifier naming conventions. I have a few more things to work out, mostly in mail_util.h, and then I'm proceeding to jamify the build system. Then we go into Haiku CVS.
 *
 * Revision 1.18  2003/09/20 12:39:27  agmsmith
 * Memory leak delete needs [] bug.
 *
 * Revision 1.17  2003/07/08 21:12:47  agmsmith
 * Changed other spam filter defaults to values I find useful.
 *
 * Revision 1.16  2003/07/08 20:56:40  agmsmith
 * Turn on auto-training for the spam filter by default.
 *
 * Revision 1.15  2003/07/06 13:30:33  agmsmith
 * Make sure that the spam filter doesn't auto-train the message twice
 * when it gets a partially downloaded e-mail (will just train on the
 * partial one, ignore the complete message when it gets downloaded).
 *
 * Revision 1.14  2003/05/27 17:12:59  nwhitehorn
 * Massive refactoring of the Protocol/ChainRunner/Filter system. You can probably
 * examine its scope by examining the number of files changed. Regardless, this is
 * preparation for lots of new features, and REAL WORKING IMAP. Yes, you heard me.
 * Enjoy, and prepare for bugs (although I've fixed all the ones I've found, I susp
 * ect there are some memory leaks in ChainRunner).
 *
 * Revision 1.13  2003/02/08 21:54:17  agmsmith
 * Updated the AGMSBayesianSpamServer documentation to match the current
 * version.  Also removed the Beep options from the spam filter, now they
 * are turned on or off in the system sound preferences.
 *
 * Revision 1.12  2002/12/18 02:27:45  agmsmith
 * Added uncertain classification as suggested by BiPolar.
 *
 * Revision 1.11  2002/12/16 16:03:20  agmsmith
 * Changed spam cutoff to 0.95 to work with default Chi-Squared scoring.
 *
 * Revision 1.10  2002/12/13 22:04:42  agmsmith
 * Changed default to turn on the Spam marker in the subject.
 *
 * Revision 1.9  2002/12/13 20:27:44  agmsmith
 * Added auto-training mode to the filter.  It evaluates a message for
 * spaminess then recursively adds it to the database.  This can lead
 * to weird results unless the user corrects the bad classifications.
 *
 * Revision 1.8  2002/11/28 20:20:57  agmsmith
 * Now checks if the spam database is running in headers only mode, and
 * then only downloads headers if that is the case.
 *
 * Revision 1.7  2002/11/10 19:36:26  agmsmith
 * Retry launching server a few times, but not too many.
 *
 * Revision 1.6  2002/11/03 02:21:02  agmsmith
 * Never mind, just use the SourceForge version numbers.  Ugh.
 *
 * Revision 1.8  2002/10/21 16:12:09  agmsmith
 * Added option for spam if no words found, use new method of saving
 * the attribute which avoids hacking the rest of the mail system.
 *
 * Revision 1.7  2002/10/11 20:01:28  agmsmith
 * Added sound effects (system beep) for genuine and spam, plus config option
 * for it.
 *
 * Revision 1.6  2002/10/01 00:45:34  agmsmith
 * Changed default spam ratio to 0.56 from 0.9, for use with
 * the Gary Robinson method in AGMSBayesianSpamServer 1.49.
 *
 * Revision 1.5  2002/09/25 13:23:21  agmsmith
 * Don't leave the data stream at the initial position, try leaving it
 * at the end.  Was having mail progress bar problems.
 *
 * Revision 1.4  2002/09/23 19:14:13  agmsmith
 * Added an option to have the server quit when done.
 *
 * Revision 1.3  2002/09/23 03:33:34  agmsmith
 * First working version, with cutoff ratio and subject modification,
 * and an attribute added if a patch is made to the Folder filter.
 *
 * Revision 1.2  2002/09/21 20:57:22  agmsmith
 * Fixed bugs so now it compiles.
 *
 * Revision 1.1  2002/09/21 20:47:15  agmsmith
 * Initial revision
 */

#include <Beep.h>
#include <fs_attr.h>
#include <Messenger.h>
#include <Node.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include <stdlib.h>
#include <stdio.h>

#include "SpamFilter.h"

// The names match the ones set up by AGMSBayesianSpamServer for sound effects.
static const char *kAGMSBayesBeepGenuineName = "AGMSBayes-Genuine";
static const char *kAGMSBayesBeepSpamName = "AGMSBayes-Spam";
static const char *kAGMSBayesBeepUncertainName = "AGMSBayes-Uncertain";

static const char *kServerSignature =
	"application/x-vnd.agmsmith.AGMSBayesianSpamServer";


AGMSBayesianSpamFilter::AGMSBayesianSpamFilter (BMessage *settings)
	:	BMailFilter (settings),
		fAddSpamToSubject (false),
		fAutoTraining (true),
		fGenuineCutoffRatio (0.01f),
		fHeaderOnly (false),
		fLaunchAttemptCount (0),
		fNoWordsMeansSpam (true),
		fQuitServerWhenFinished (false),
		fSpamCutoffRatio (0.99f)
{
	bool		tempBool;
	float		tempFloat;
	BMessenger	tempMessenger;

	if (settings != NULL) {
		if (settings->FindBool ("AddMarkerToSubject", &tempBool) == B_OK)
			fAddSpamToSubject = tempBool;
		if (settings->FindBool ("AutoTraining", &tempBool) == B_OK)
			fAutoTraining = tempBool;
		if (settings->FindFloat ("GenuineCutoffRatio", &tempFloat) == B_OK)
			fGenuineCutoffRatio = tempFloat;
		if (settings->FindBool ("NoWordsMeansSpam", &tempBool) == B_OK)
			fNoWordsMeansSpam = tempBool;
		if (settings->FindBool ("QuitServerWhenFinished", &tempBool) == B_OK)
			fQuitServerWhenFinished = tempBool;
		if (settings->FindFloat ("SpamCutoffRatio", &tempFloat) == B_OK)
			fSpamCutoffRatio = tempFloat;
	}
}


AGMSBayesianSpamFilter::~AGMSBayesianSpamFilter ()
{
	BMessage quitMessage (B_QUIT_REQUESTED);

	if (fQuitServerWhenFinished && fMessengerToServer.IsValid ())
		fMessengerToServer.SendMessage (&quitMessage);
}


status_t
AGMSBayesianSpamFilter::InitCheck (BString* out_message)
{
	if (out_message != NULL)
		out_message->SetTo (
			"AGMSBayesianSpamFilter::InitCheck is never called!");
	return B_OK;
}


status_t
AGMSBayesianSpamFilter::ProcessMailMessage (
	BPositionIO** io_message,
	BEntry* io_entry,
	BMessage* io_headers,
	BPath* io_folder,
	const char* io_uid)
{
	ssize_t		 amountRead;
	attr_info	 attributeInfo;
	const char	*classificationString;
	off_t		 dataSize;
	BPositionIO	*dataStreamPntr = *io_message;
	status_t	 errorCode = B_OK;
	int32        headerLength;
	BString      headerString;
	BString		 newSubjectString;
	BNode        nodeForOutputFile;
	bool		 nodeForOutputFileInitialised = false;
	const char	*oldSubjectStringPntr;
	char         percentageString [30];
	BMessage	 replyMessage;
	BMessage	 scriptingMessage;
	team_id		 serverTeam;
	float		 spamRatio;
	char		*stringBuffer = NULL;
	char         tempChar;
	status_t     tempErrorCode;
	const char  *tokenizeModeStringPntr;

	// Set up a BNode to the final output file so that we can write custom
	// attributes to it.  Non-custom attributes are stored separately in
	// io_headers.

	if (io_entry != NULL && B_OK == nodeForOutputFile.SetTo (io_entry))
		nodeForOutputFileInitialised = true;

	// Get a connection to the spam database server.  Launch if needed, should
	// only need it once, unless another e-mail thread shuts down the server
	// inbetween messages.  This code used to be in InitCheck, but apparently
	// that isn't called.

	if (fLaunchAttemptCount == 0 || !fMessengerToServer.IsValid ()) {
		if (fLaunchAttemptCount > 3)
			goto ErrorExit; // Don't try to start the server too many times.
		fLaunchAttemptCount++;

		// Make sure the server is running.
		if (!be_roster->IsRunning (kServerSignature)) {
			errorCode = be_roster->Launch (kServerSignature);
			if (errorCode != B_OK)
				goto ErrorExit;
		}

		// Set up the messenger to the database server.
		serverTeam = be_roster->TeamFor (kServerSignature);
		if (serverTeam < 0)
			goto ErrorExit;
		fMessengerToServer =
			BMessenger (kServerSignature, serverTeam, &errorCode);
		if (!fMessengerToServer.IsValid ())
			goto ErrorExit;

		// Check if the server is running in headers only mode.  If so, we only
		// need to download the header rather than the entire message.
		scriptingMessage.MakeEmpty ();
		scriptingMessage.what = B_GET_PROPERTY;
		scriptingMessage.AddSpecifier ("TokenizeMode");
		replyMessage.MakeEmpty ();
		if ((errorCode = fMessengerToServer.SendMessage (&scriptingMessage,
			&replyMessage)) != B_OK)
			goto ErrorExit;
		if ((errorCode = replyMessage.FindInt32 ("error", &tempErrorCode))
			!= B_OK)
			goto ErrorExit;
		if ((errorCode = tempErrorCode) != B_OK)
			goto ErrorExit;
		if ((errorCode = replyMessage.FindString ("result",
			&tokenizeModeStringPntr)) != B_OK)
			goto ErrorExit;
		fHeaderOnly = (tokenizeModeStringPntr != NULL
			&& strcmp (tokenizeModeStringPntr, "JustHeader") == 0);
	}

	// See if the message has already been classified.  Happens for messages
	// which are partially downloaded when you have auto-training on.  Could
	// untrain the partial part before training on the complete message, but we
	// don't know how big it was, so instead just ignore the message.

	if (nodeForOutputFileInitialised) {
		if (nodeForOutputFile.GetAttrInfo ("MAIL:classification",
			&attributeInfo) == B_OK)
			return B_OK;
	}

	// Copy the message to a string so that we can pass it to the spam database
	// (the even messier alternative is a temporary file).  Do it in a fashion
	// which allows NUL bytes in the string.  This method of course limits the
	// message size to a few hundred megabytes.  If we're using header mode,
	// only read the header rather than the full message.

	if (fHeaderOnly) {
		// Read just the header, it ends with an empty CRLF line.
		dataStreamPntr->Seek (0, SEEK_SET);
		while ((errorCode = dataStreamPntr->Read (&tempChar, 1)) == 1) {
			headerString.Append (tempChar, 1);
			headerLength = headerString.Length();
			if (headerLength >= 4 && strcmp (headerString.String() +
				headerLength - 4, "\r\n\r\n") == 0)
				break;
		}
		if (errorCode < 0)
			goto ErrorExit;

		dataSize = headerString.Length();
		stringBuffer = new char [dataSize + 1];
		memcpy (stringBuffer, headerString.String(), dataSize);
		stringBuffer[dataSize] = 0;
	} else {
		// Read the whole file.  The seek to the end may take a while since
		// that triggers downloading of the entire message (and caching in a
		// slave file - see the MessageIO class).
		dataSize = dataStreamPntr->Seek (0, SEEK_END);
		if (dataSize <= 0)
			goto ErrorExit;

		try {
			stringBuffer = new char [dataSize + 1];
		} catch (...) {
			errorCode = ENOMEM;
			goto ErrorExit;
		}

		dataStreamPntr->Seek (0, SEEK_SET);
		amountRead = dataStreamPntr->Read (stringBuffer, dataSize);
		if (amountRead != dataSize)
			goto ErrorExit;
		stringBuffer[dataSize] = 0; // Add an end of string NUL, just in case.
	}

	// Send off a scripting command to the database server, asking it to
	// evaluate the string for spaminess.  Note that it can return ENOMSG
	// when there are no words (a good indicator of spam which is pure HTML
	// if you are using plain text only tokenization), so we could use that
	// as a spam marker too.  Code copied for the reevaluate stuff below.

	scriptingMessage.MakeEmpty ();
	scriptingMessage.what = B_SET_PROPERTY;
	scriptingMessage.AddSpecifier ("EvaluateString");
	errorCode = scriptingMessage.AddData ("data", B_STRING_TYPE,
		stringBuffer, dataSize + 1, false /* fixed size */);
	if (errorCode != B_OK)
		goto ErrorExit;
	replyMessage.MakeEmpty ();
	errorCode = fMessengerToServer.SendMessage (&scriptingMessage,
		&replyMessage);
	if (errorCode != B_OK
		|| replyMessage.FindInt32 ("error", &errorCode) != B_OK)
		goto ErrorExit; // Unable to read the return code.
	if (errorCode == ENOMSG && fNoWordsMeansSpam)
		spamRatio = fSpamCutoffRatio; // Yes, no words and that means spam.
	else if (errorCode != B_OK
		|| replyMessage.FindFloat ("result", &spamRatio) != B_OK)
		goto ErrorExit; // Classification failed in one of many ways.

	// If we are auto-training, feed back the message to the server as a
	// training example (don't train if it is uncertain).  Also redo the
	// evaluation after training.

	if (fAutoTraining) {
		if (spamRatio >= fSpamCutoffRatio || spamRatio < fGenuineCutoffRatio) {
			scriptingMessage.MakeEmpty ();
			scriptingMessage.what = B_SET_PROPERTY;
			scriptingMessage.AddSpecifier ((spamRatio >= fSpamCutoffRatio)
				? "SpamString" : "GenuineString");
			errorCode = scriptingMessage.AddData ("data", B_STRING_TYPE,
				stringBuffer, dataSize + 1, false /* fixed size */);
			if (errorCode != B_OK)
				goto ErrorExit;
			replyMessage.MakeEmpty ();
			errorCode = fMessengerToServer.SendMessage (&scriptingMessage,
				&replyMessage);
			if (errorCode != B_OK
				|| replyMessage.FindInt32 ("error", &errorCode) != B_OK)
				goto ErrorExit; // Unable to read the return code.
			if (errorCode != B_OK)
				goto ErrorExit; // Failed to set a good example.
		}

		// Note the kind of example made so that the user doesn't reclassify
		// the message twice (the spam server looks for this attribute).

		classificationString =
			(spamRatio >= fSpamCutoffRatio)
			? "Spam"
			: ((spamRatio < fGenuineCutoffRatio) ? "Genuine" : "Uncertain");
		if (nodeForOutputFileInitialised)
			nodeForOutputFile.WriteAttr ("MAIL:classification", B_STRING_TYPE,
				0 /* offset */, classificationString,
				strlen (classificationString) + 1);

		// Now that the database has changed due to training, recompute the
		// spam ratio.  Hopefully it will have become more extreme in the
		// correct direction (not switched from being spam to being genuine).
		// Code copied from above.

		scriptingMessage.MakeEmpty ();
		scriptingMessage.what = B_SET_PROPERTY;
		scriptingMessage.AddSpecifier ("EvaluateString");
		errorCode = scriptingMessage.AddData ("data", B_STRING_TYPE,
			stringBuffer, dataSize + 1, false /* fixed size */);
		if (errorCode != B_OK)
			goto ErrorExit;
		replyMessage.MakeEmpty ();
		errorCode = fMessengerToServer.SendMessage (&scriptingMessage,
			&replyMessage);
		if (errorCode != B_OK
			|| replyMessage.FindInt32 ("error", &errorCode) != B_OK)
			goto ErrorExit; // Unable to read the return code.
		if (errorCode == ENOMSG && fNoWordsMeansSpam)
			spamRatio = fSpamCutoffRatio; // Yes, no words and that means spam.
		else if (errorCode != B_OK
			|| replyMessage.FindFloat ("result", &spamRatio) != B_OK)
			goto ErrorExit; // Classification failed in one of many ways.
	}

	// Store the spam ratio in an attribute called MAIL:ratio_spam,
	// attached to the eventual output file.

	if (nodeForOutputFileInitialised)
		nodeForOutputFile.WriteAttr ("MAIL:ratio_spam",
			B_FLOAT_TYPE, 0 /* offset */, &spamRatio, sizeof (spamRatio));

	// Also add it to the subject, if requested.

	if (fAddSpamToSubject
		&& spamRatio >= fSpamCutoffRatio
		&& io_headers->FindString ("Subject", &oldSubjectStringPntr) == B_OK) {
		newSubjectString.SetTo ("[Spam ");
		sprintf (percentageString, "%05.2f", spamRatio * 100.0);
		newSubjectString << percentageString << "%] ";
		newSubjectString << oldSubjectStringPntr;
		io_headers->ReplaceString ("Subject", newSubjectString);
	}

	// Beep using different sounds for spam and genuine, as Jeremy Friesner
	// nudged me to get around to implementing.  And add uncertain to that, as
	// "BiPolar" suggested.  If the user doesn't want to hear the sound, they
	// can turn it off in the system sound preferences.

	if (spamRatio >= fSpamCutoffRatio) {
		system_beep (kAGMSBayesBeepSpamName);
	} else if (spamRatio < fGenuineCutoffRatio) {
		system_beep (kAGMSBayesBeepGenuineName);
	} else {
		system_beep (kAGMSBayesBeepUncertainName);
	}

	return B_OK;

ErrorExit:
	fprintf (stderr, "Error exit from "
		"AGMSBayesianSpamFilter::ProcessMailMessage, code maybe %ld (%s).\n",
		errorCode, strerror (errorCode));
	delete [] stringBuffer;
	return B_OK; // Not MD_ERROR so the message doesn't get left on server.
}


status_t
descriptive_name (
	BMessage *settings,
	char *buffer)
{
	bool		addMarker = false;
	bool		autoTraining = true;
	float		cutoffRatio = 0.99f;
	bool		tempBool;
	float		tempFloat;

	if (settings != NULL) {
		if (settings->FindBool ("AddMarkerToSubject", &tempBool) == B_OK)
			addMarker = tempBool;
		if (settings->FindBool ("AutoTraining", &tempBool) == B_OK)
			autoTraining = tempBool;
		if (settings->FindFloat ("SpamCutoffRatio", &tempFloat) == B_OK)
			cutoffRatio = tempFloat;
	}

	sprintf (buffer, "Spam >= %05.3f", (double) cutoffRatio);
	if (addMarker)
		strcat (buffer, ", Mark Subject");
	if (autoTraining)
		strcat (buffer, ", Self-training");
	strcat (buffer, ".");

	return B_OK;
}


BMailFilter *
instantiate_mailfilter (
	BMessage *settings,
	BMailChainRunner *)
{
	return new AGMSBayesianSpamFilter (settings);
}
