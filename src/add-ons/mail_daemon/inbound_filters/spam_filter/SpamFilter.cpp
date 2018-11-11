/*
 * Copyright 2002-2013, Haiku, Inc. All rights reserved.
 * Copyright 2002 Alexander G. M. Smith.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */

/*!	Uses Bayesian statistics to evaluate the spaminess of a message.
	The evaluation is done by a separate server, this add-on just gets
	the text and uses scripting commands to get an evaluation from the server.
	If the server isn't running, it will be found and started up.  Once the
	evaluation has been received, it is added to the message as an attribute and
	optionally as an addition to the subject.  Some other add-on later in the
	pipeline will use the attribute to delete the message or move it to some
	other folder.
*/


#include "SpamFilter.h"

#include <stdlib.h>
#include <stdio.h>

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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SpamFilter"


// The names match the ones set up by spamdbm for sound effects.
static const char* kAGMSBayesBeepGenuineName = "SpamFilter-Genuine";
static const char* kAGMSBayesBeepSpamName = "SpamFilter-Spam";
static const char* kAGMSBayesBeepUncertainName = "SpamFilter-Uncertain";

static const char* kServerSignature = "application/x-vnd.agmsmith.spamdbm";


SpamFilter::SpamFilter(BMailProtocol& protocol,
	const BMailAddOnSettings& settings)
	:
	BMailFilter(protocol, &settings)
{
	fAddSpamToSubject = settings.GetBool("AddMarkerToSubject", false);
	fAutoTraining = settings.GetBool("AutoTraining", true);
	fGenuineCutoffRatio = settings.GetFloat("GenuineCutoffRatio", 0.01f);
	fNoWordsMeansSpam = settings.GetBool("NoWordsMeansSpam", true);
	fQuitServerWhenFinished = settings.GetBool("QuitServerWhenFinished", false);
	fSpamCutoffRatio = settings.GetFloat("SpamCutoffRatio", 0.99f);
}


SpamFilter::~SpamFilter()
{
	if (fQuitServerWhenFinished)
		fMessengerToServer.SendMessage(B_QUIT_REQUESTED);
}


BMailFilterAction
SpamFilter::HeaderFetched(entry_ref& ref, BFile& file, BMessage& attributes)
{
	_CheckForSpam(file);
	return B_NO_MAIL_ACTION;
}


void
SpamFilter::BodyFetched(const entry_ref& ref, BFile& file, BMessage& attributes)
{
	if (fHeaderOnly)
		return;

	// See if the message has already been classified.  Happens for messages
	// which are partially downloaded when you have auto-training on.  Could
	// untrain the partial part before training on the complete message, but we
	// don't know how big it was, so instead just ignore the message.
	attr_info attributeInfo;
	if (file.GetAttrInfo("MAIL:classification", &attributeInfo) == B_OK)
		return;

	_CheckForSpam(file);
}


status_t
SpamFilter::_CheckForSpam(BFile& file)
{
	// Get a connection to the spam database server.  Launch if needed, should
	// only need it once, unless another e-mail thread shuts down the server
	// inbetween messages.  This code used to be in InitCheck, but apparently
	// that isn't called.
	printf("Checking for Spam Server.\n");
	if (fLaunchAttemptCount == 0 || !fMessengerToServer.IsValid()) {
		if (_GetTokenizeMode() != B_OK)
			return B_ERROR;
	}

	off_t dataSize;
	file.GetSize(&dataSize);
	char* stringBuffer = new char[dataSize + 1];
	file.Read(stringBuffer, dataSize);
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
	BString classificationString = spamRatio >= fSpamCutoffRatio ? "Spam"
		: spamRatio < fGenuineCutoffRatio ? "Genuine" : "Uncertain";
	file.WriteAttrString("MAIL:classification", &classificationString);

	// Store the spam ratio in an attribute called MAIL:ratio_spam,
	// attached to the eventual output file.
	file.WriteAttr("MAIL:ratio_spam", B_FLOAT_TYPE, 0 /* offset */, &spamRatio,
		sizeof(spamRatio));

	// Also add it to the subject, if requested.
	if (fAddSpamToSubject && spamRatio >= fSpamCutoffRatio)
		_AddSpamToSubject(file, spamRatio);

	// Beep using different sounds for spam and genuine, as Jeremy Friesner
	// nudged me to get around to implementing.  And add uncertain to that, as
	// "BiPolar" suggested.  If the user doesn't want to hear the sound, they
	// can turn it off in the system sound preferences.

	if (spamRatio >= fSpamCutoffRatio)
		system_beep(kAGMSBayesBeepSpamName);
	else if (spamRatio < fGenuineCutoffRatio)
		system_beep(kAGMSBayesBeepGenuineName);
	else
		system_beep(kAGMSBayesBeepUncertainName);

	return B_OK;
}


status_t
SpamFilter::_CheckForSpamServer()
{
	// Make sure the server is running.
	if (be_roster->IsRunning (kServerSignature))
		return B_OK;

	status_t status = be_roster->Launch (kServerSignature);
	if (status == B_OK)
		return status;

	BPath path;
	entry_ref ref;
	const directory_which kPlaces[] = {
		B_SYSTEM_NONPACKAGED_BIN_DIRECTORY,
		B_SYSTEM_BIN_DIRECTORY};
	for (size_t i = 0; i < sizeof(kPlaces) / sizeof(kPlaces[0]); i++) {
		find_directory(kPlaces[i], &path);
		path.Append("spamdbm");
		if (!BEntry(path.Path()).Exists())
			continue;
		get_ref_for_path(path.Path(), &ref);
		if ((status = be_roster->Launch(&ref)) == B_OK)
			break;
	}

	return status;
}


status_t
SpamFilter::_GetTokenizeMode()
{
	if (fLaunchAttemptCount > 3)
		return B_ERROR; // Don't try to start the server too many times.
	fLaunchAttemptCount++;

	// Make sure the server is running.
	status_t status = _CheckForSpamServer();
	if (status != B_OK)
		return status;

	// Set up the messenger to the database server.
	fMessengerToServer = BMessenger(kServerSignature);
	if (!fMessengerToServer.IsValid())
		return B_ERROR;

	// Check if the server is running in headers only mode.  If so, we only
	// need to download the header rather than the entire message.
	BMessage scriptingMessage(B_GET_PROPERTY);
	scriptingMessage.AddSpecifier("TokenizeMode");
	BMessage replyMessage;
	if ((status = fMessengerToServer.SendMessage(&scriptingMessage,
			&replyMessage)) != B_OK)
		return status;
	status_t errorCode;
	if ((status = replyMessage.FindInt32("error", &errorCode)) != B_OK)
		return status;
	if (errorCode != B_OK)
		return errorCode;

	const char* tokenizeMode;
	if ((status = replyMessage.FindString("result", &tokenizeMode)) != B_OK)
		return status;

	fHeaderOnly = tokenizeMode != NULL && !strcmp(tokenizeMode, "JustHeader");
	return B_OK;
}


status_t
SpamFilter::_GetSpamRatio(const char* stringBuffer, off_t dataSize,
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
SpamFilter::_TrainServer(const char* stringBuffer, off_t dataSize,
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
SpamFilter::_AddSpamToSubject(BNode& file, float spamRatio)
{
	attr_info info;
	if (file.GetAttrInfo("Subject", &info) != B_OK)
		return B_ERROR;
	if (info.type != B_STRING_TYPE)
		return B_ERROR;

	char* buffer = new char[info.size];
	if (file.ReadAttr("Subject", B_STRING_TYPE, 0, buffer, info.size) < 0) {
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

	if (file.WriteAttrString("Subject", &newSubjectString) < 0)
		return B_ERROR;

	return B_OK;
}


// #pragma mark -


BString
filter_name(const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings* addOnSettings)
{
	return B_TRANSLATE("Bayesian Spam Filter");
}


BMailFilter*
instantiate_filter(BMailProtocol& protocol, const BMailAddOnSettings& settings)
{
	return new SpamFilter(protocol, settings);
}
