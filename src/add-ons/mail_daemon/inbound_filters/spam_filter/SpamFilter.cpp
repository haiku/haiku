/*
 * Copyright 2002-2011, Haiku, Inc. All rights reserved.
 * Copyright 2002 Alexander G. M. Smith.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
/******************************************************************************
 * $Id: SpamFilter.cpp 29284 2009-02-22 13:45:40Z bga $
 *
 * SpamFilter - Uses Bayesian statistics to evaluate the spaminess of a
 * message.  The evaluation is done by a separate server, this add-on just gets
 * the text and uses scripting commands to get an evaluation from the server.
 * If the server isn't running, it will be found and started up.  Once the
 * evaluation has been received, it is added to the message as an attribute and
 * optionally as an addition to the subject.  Some other add-on later in the
 * pipeline will use the attribute to delete the message or move it to some
 * other folder.
 *
 * Public Domain 2002, by Alexander G. M. Smith, no warranty.
 *
 * $Log: SpamFilter.cpp,v $ (SVN doesn't support log messages so manually done)
 * r11769 | bonefish | 2005-03-17 03:30:54 -0500 (Thu, 17 Mar 2005) | 1 line
 * Move trunk into respective module.
 *
 * r9934 | nwhitehorn | 2004-11-11 21:55:05 -0500 (Thu, 11 Nov 2004) | 2 lines
 * Added AGMS's excellent spam detection software.  Still some weirdness with
 * the configuration interface from E-mail prefs.
 *
 * r9669 | brunoga | 2004-10-30 18:23:26 -0400 (Sat, 30 Oct 2004) | 2 lines
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
#include <Catalog.h>
#include <fs_attr.h>
#include <Messenger.h>
#include <Node.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <FindDirectory.h>
#include <Entry.h>

#include <stdlib.h>
#include <stdio.h>

#include "SpamFilter.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SpamFilter"


// The names match the ones set up by spamdbm for sound effects.
static const char *kAGMSBayesBeepGenuineName = "SpamFilter-Genuine";
static const char *kAGMSBayesBeepSpamName = "SpamFilter-Spam";
static const char *kAGMSBayesBeepUncertainName = "SpamFilter-Uncertain";

static const char *kServerSignature = "application/x-vnd.agmsmith.spamdbm";


AGMSBayesianSpamFilter::AGMSBayesianSpamFilter(MailProtocol& protocol,
	AddonSettings* addonSettings)
	:
	MailFilter(protocol, addonSettings),

	fAddSpamToSubject(false),
	fAutoTraining(true),
	fGenuineCutoffRatio(0.01f),
	fHeaderOnly(false),
	fLaunchAttemptCount(0),
	fNoWordsMeansSpam(true),
	fQuitServerWhenFinished(false),
	fSpamCutoffRatio(0.99f)
{
	bool		tempBool;
	float		tempFloat;
	BMessenger	tempMessenger;

	const BMessage* settings = &addonSettings->Settings();
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
	if (fQuitServerWhenFinished && fMessengerToServer.IsValid ())
		fMessengerToServer.SendMessage(B_QUIT_REQUESTED);
}


void
AGMSBayesianSpamFilter::HeaderFetched(const entry_ref& ref, BFile* file)
{
	_CheckForSpam(file);
}


void
AGMSBayesianSpamFilter::BodyFetched(const entry_ref& ref, BFile* file)
{
	if (fHeaderOnly)
		return;

	// See if the message has already been classified.  Happens for messages
	// which are partially downloaded when you have auto-training on.  Could
	// untrain the partial part before training on the complete message, but we
	// don't know how big it was, so instead just ignore the message.
	attr_info attributeInfo;
	if (file->GetAttrInfo ("MAIL:classification", &attributeInfo) == B_OK)
		return;

	_CheckForSpam(file);
}


status_t
AGMSBayesianSpamFilter::_CheckForSpam(BFile* file)
{
	// Get a connection to the spam database server.  Launch if needed, should
	// only need it once, unless another e-mail thread shuts down the server
	// inbetween messages.  This code used to be in InitCheck, but apparently
	// that isn't called.
	printf("Checking for Spam Server.\n");
	if (fLaunchAttemptCount == 0 || !fMessengerToServer.IsValid ()) {
		if (_GetTokenizeMode() != B_OK)
			return B_ERROR;
	}

	off_t dataSize;
	file->GetSize(&dataSize);
	char* stringBuffer = new char[dataSize + 1];
	file->Read(stringBuffer, dataSize);
	stringBuffer[dataSize] = 0; // Add an end of string NUL, just in case.

	float spamRatio;
	if (_GetSpamRatio(stringBuffer, dataSize, spamRatio) != B_OK)
		return B_ERROR;
	
	// If we are auto-training, feed back the message to the server as a
	// training example (don't train if it is uncertain).
	if (fAutoTraining && (spamRatio >= fSpamCutoffRatio
		|| spamRatio < fGenuineCutoffRatio)) {
			_TrainServer(stringBuffer, dataSize, spamRatio);
	}

	delete[] stringBuffer;

	// write attributes
	const char *classificationString;
	classificationString = (spamRatio >= fSpamCutoffRatio) ? "Spam"
		: ((spamRatio < fGenuineCutoffRatio) ? "Genuine" : "Uncertain");
	file->WriteAttr("MAIL:classification", B_STRING_TYPE, 0 /* offset */,
		classificationString, strlen(classificationString) + 1);

	// Store the spam ratio in an attribute called MAIL:ratio_spam,
	// attached to the eventual output file.
	file->WriteAttr("MAIL:ratio_spam", B_FLOAT_TYPE, 0 /* offset */, &spamRatio,
		sizeof(spamRatio));

	// Also add it to the subject, if requested.
	if (fAddSpamToSubject && spamRatio >= fSpamCutoffRatio)
		_AddSpamToSubject(file, spamRatio);

	// Beep using different sounds for spam and genuine, as Jeremy Friesner
	// nudged me to get around to implementing.  And add uncertain to that, as
	// "BiPolar" suggested.  If the user doesn't want to hear the sound, they
	// can turn it off in the system sound preferences.

	if (spamRatio >= fSpamCutoffRatio) {
		system_beep(kAGMSBayesBeepSpamName);
	} else if (spamRatio < fGenuineCutoffRatio) {
		system_beep(kAGMSBayesBeepGenuineName);
	} else {
		system_beep(kAGMSBayesBeepUncertainName);
	}

	return B_OK;
}


status_t
AGMSBayesianSpamFilter::_CheckForSpamServer()
{
	// Make sure the server is running.
	if (be_roster->IsRunning (kServerSignature))
		return B_OK;

	status_t errorCode = be_roster->Launch (kServerSignature);
	if (errorCode == B_OK)
		return errorCode;

	BPath path;
	entry_ref ref;
	directory_which places[] = {B_COMMON_BIN_DIRECTORY,B_BEOS_BIN_DIRECTORY};
	for (int32 i = 0; i < 2; i++) {
		find_directory(places[i],&path);
		path.Append("spamdbm");
		if (!BEntry(path.Path()).Exists())
			continue;
		get_ref_for_path(path.Path(),&ref);
		if ((errorCode =  be_roster->Launch(&ref)) == B_OK)
			break;
	}

	return errorCode;
}


status_t
AGMSBayesianSpamFilter::_GetTokenizeMode()
{
	if (fLaunchAttemptCount > 3)
		return B_ERROR; // Don't try to start the server too many times.
	fLaunchAttemptCount++;

	// Make sure the server is running.
	status_t errorCode = _CheckForSpamServer();
	if (errorCode != B_OK)
		return errorCode;

	// Set up the messenger to the database server.
	fMessengerToServer = BMessenger(kServerSignature);
	if (!fMessengerToServer.IsValid ())
		return B_ERROR;

	// Check if the server is running in headers only mode.  If so, we only
	// need to download the header rather than the entire message.
	BMessage scriptingMessage(B_GET_PROPERTY);
	scriptingMessage.AddSpecifier("TokenizeMode");
	BMessage replyMessage;
	if ((errorCode = fMessengerToServer.SendMessage (&scriptingMessage,
		&replyMessage)) != B_OK)
		return errorCode;
	status_t tempErrorCode;
	if ((errorCode = replyMessage.FindInt32 ("error", &tempErrorCode))
		!= B_OK)
		return errorCode;
	if ((errorCode = tempErrorCode) != B_OK)
		return errorCode;

	const char  *tokenizeModeStringPntr;
	if ((errorCode = replyMessage.FindString ("result",
		&tokenizeModeStringPntr)) != B_OK)
		return errorCode;
	fHeaderOnly = (tokenizeModeStringPntr != NULL
		&& strcmp (tokenizeModeStringPntr, "JustHeader") == 0);
	return B_OK;
}


status_t
AGMSBayesianSpamFilter::_GetSpamRatio(const char* stringBuffer, off_t dataSize,
	float& ratio)
{
	// Send off a scripting command to the database server, asking it to
	// evaluate the string for spaminess.  Note that it can return ENOMSG
	// when there are no words (a good indicator of spam which is pure HTML
	// if you are using plain text only tokenization), so we could use that
	// as a spam marker too.  Code copied for the reevaluate stuff below.

	BMessage scriptingMessage(B_SET_PROPERTY);
	scriptingMessage.AddSpecifier("EvaluateString");
	status_t errorCode = scriptingMessage.AddData("data", B_STRING_TYPE,
		stringBuffer, dataSize + 1, false /* fixed size */);
	if (errorCode != B_OK)
		return errorCode;
	BMessage replyMessage;
	errorCode = fMessengerToServer.SendMessage(&scriptingMessage,
		&replyMessage);
	if (errorCode != B_OK
		|| replyMessage.FindInt32("error", &errorCode) != B_OK)
		return errorCode; // Unable to read the return code.
	if (errorCode == ENOMSG && fNoWordsMeansSpam)
		ratio = fSpamCutoffRatio; // Yes, no words and that means spam.
	else if (errorCode != B_OK
		|| replyMessage.FindFloat("result", &ratio) != B_OK)
		return errorCode; // Classification failed in one of many ways.

	return errorCode;
}


status_t
AGMSBayesianSpamFilter::_TrainServer(const char* stringBuffer, off_t dataSize,
	float spamRatio)
{
	BMessage scriptingMessage(B_SET_PROPERTY);
	scriptingMessage.AddSpecifier((spamRatio >= fSpamCutoffRatio)
		? "SpamString" : "GenuineString");
	status_t errorCode = scriptingMessage.AddData ("data", B_STRING_TYPE,
		stringBuffer, dataSize + 1, false /* fixed size */);
	if (errorCode != B_OK)
		return errorCode;
	BMessage replyMessage;
	errorCode = fMessengerToServer.SendMessage (&scriptingMessage,
		&replyMessage);
	if (errorCode != B_OK)
		return errorCode;
	errorCode = replyMessage.FindInt32("error", &errorCode);

	return errorCode;
}


status_t
AGMSBayesianSpamFilter::_AddSpamToSubject(BNode* file, float spamRatio)
{
	attr_info info;
	if (file->GetAttrInfo("Subject", &info) != B_OK)
		return B_ERROR;
	if (info.type != B_STRING_TYPE)
		return B_ERROR;

	char* buffer = new char[info.size];
	if (file->ReadAttr("Subject", B_STRING_TYPE, 0, buffer, info.size) < 0) {
		delete[] buffer;
		return B_ERROR;
	}
                     
	BString newSubjectString;
	newSubjectString.SetTo("[Spam ");
	char percentageString[30];
	sprintf(percentageString, "%05.2f", spamRatio * 100.0);
	newSubjectString << percentageString << "%] ";
	newSubjectString << buffer;
	delete[] buffer;

	if (file->WriteAttr("Subject", B_STRING_TYPE, 0, newSubjectString.String(),
		newSubjectString.Length()) < 0)
		return B_ERROR;

	return B_OK;
}


BString
descriptive_name()
{
	return B_TRANSLATE("Spam Filter (AGMS Bayesian)");
}


MailFilter*
instantiate_mailfilter(MailProtocol& protocol, AddonSettings* settings)
{
	return new AGMSBayesianSpamFilter(protocol, settings);
}
