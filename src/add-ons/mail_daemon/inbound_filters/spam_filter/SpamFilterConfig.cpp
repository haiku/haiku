/******************************************************************************
 * AGMSBayesianSpamFilter's configuration view.  Lets the user change various
 * settings related to the add-on, but not the server.
 *
 * $Log: SpamFilterConfig.cpp,v $
 * Revision 1.1  2004/10/30 22:23:26  brunoga
 * AGMS Spam Filter.
 *
 * Revision 1.9  2004/09/20 15:57:30  nwhitehorn
 * Mostly updated the tree to Be/Haiku style identifier naming conventions. I have a few more things to work out, mostly in mail_util.h, and then I'm proceeding to jamify the build system. Then we go into Haiku CVS.
 *
 * Revision 1.8  2003/07/08 21:12:47  agmsmith
 * Changed other spam filter defaults to values I find useful.
 *
 * Revision 1.7  2003/07/08 20:56:40  agmsmith
 * Turn on auto-training for the spam filter by default.
 *
 * Revision 1.6  2003/02/08 21:54:17  agmsmith
 * Updated the AGMSBayesianSpamServer documentation to match the current
 * version.  Also removed the Beep options from the spam filter, now they
 * are turned on or off in the system sound preferences.
 *
 * Revision 1.5  2002/12/18 02:27:45  agmsmith
 * Added uncertain classification as suggested by BiPolar.
 *
 * Revision 1.4  2002/12/16 16:03:27  agmsmith
 * Changed spam cutoff to 0.95 to work with default Chi-Squared scoring.
 *
 * Revision 1.3  2002/12/13 22:04:43  agmsmith
 * Changed default to turn on the Spam marker in the subject.
 *
 * Revision 1.2  2002/12/12 00:56:28  agmsmith
 * Added some new spam filter options - self training (not implemented yet)
 * and a button to edit the server settings.
 *
 * Revision 1.1  2002/11/03 02:06:15  agmsmith
 * Added initial version.
 *
 * Revision 1.7  2002/10/21 16:13:27  agmsmith
 * Added option to have no words mean spam.
 *
 * Revision 1.6  2002/10/11 20:01:28  agmsmith
 * Added sound effects (system beep) for genuine and spam, plus config option for it.
 *
 * Revision 1.5  2002/10/01 00:45:34  agmsmith
 * Changed default spam ratio to 0.56 from 0.9, for use with
 * the Gary Robinson method in AGMSBayesianSpamServer 1.49.
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
 * Revision 1.1  2002/09/21 20:48:11  agmsmith
 * Initial revision
 */

#include <stdlib.h>
#include <stdio.h>

#include <Alert.h>
#include <Button.h>
#include <CheckBox.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>
#include <StringView.h>
#include <TextControl.h>

#include <MailAddon.h>
#include <FileConfigView.h>

static const char *kServerSignature =
	"application/x-vnd.agmsmith.AGMSBayesianSpamServer";

class AGMSBayesianSpamFilterConfig : public BView {
	public:
		AGMSBayesianSpamFilterConfig (BMessage *settings);

		virtual	void MessageReceived (BMessage *msg);
		virtual	void AttachedToWindow ();
		virtual	status_t Archive (BMessage *into, bool deep = true) const;
		virtual	void GetPreferredSize (float *width, float *height);

	private:
		void ShowSpamServerConfigurationWindow ();

		bool fAddSpamToSubject;
		BCheckBox *fAddSpamToSubjectCheckBoxPntr;
		bool fAutoTraining;
		BCheckBox *fAutoTrainingCheckBoxPntr;
		float fGenuineCutoffRatio;
		BTextControl *fGenuineCutoffRatioTextBoxPntr;
		bool fNoWordsMeansSpam;
		BCheckBox *fNoWordsMeansSpamCheckBoxPntr;
		bool fQuitServerWhenFinished;
		BCheckBox *fQuitServerWhenFinishedCheckBoxPntr;
		BButton *fServerSettingsButtonPntr;
		float fSpamCutoffRatio;
		BTextControl *fSpamCutoffRatioTextBoxPntr;
		static const uint32 kAddSpamToSubjectPressed = 'ASbj';
		static const uint32 kAutoTrainingPressed = 'AuTr';
		static const uint32 kNoWordsMeansSpam = 'NoWd';
		static const uint32 kQuitWhenFinishedPressed = 'QuWF';
		static const uint32 kServerSettingsPressed = 'SrvS';
};


AGMSBayesianSpamFilterConfig::AGMSBayesianSpamFilterConfig (BMessage *settings)
	:	BView (BRect (0,0,260,130), "agmsbayesianspamfilter_config",
			B_FOLLOW_LEFT | B_FOLLOW_TOP, 0),
		fAddSpamToSubject (false),
		fAddSpamToSubjectCheckBoxPntr (NULL),
		fAutoTraining (true),
		fAutoTrainingCheckBoxPntr (NULL),
		fGenuineCutoffRatio (0.01f),
		fGenuineCutoffRatioTextBoxPntr (NULL),
		fNoWordsMeansSpam (true),
		fNoWordsMeansSpamCheckBoxPntr (NULL),
		fQuitServerWhenFinished (false),
		fQuitServerWhenFinishedCheckBoxPntr (NULL),
		fServerSettingsButtonPntr (NULL),
		fSpamCutoffRatio (0.99f),
		fSpamCutoffRatioTextBoxPntr (NULL)
{
	bool	tempBool;
	float	tempFloat;

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


void AGMSBayesianSpamFilterConfig::AttachedToWindow ()
{
	float		 deltaX;
	BStringView	*labelViewPntr;
	char		 numberString [30];
	BRect		 tempRect;
	char		*tempStringPntr;

	SetViewColor (ui_color (B_PANEL_BACKGROUND_COLOR));

	// Make the checkbox for choosing whether the spam is marked by a
	// modification to the subject of the mail message.

	tempRect = Bounds ();
	fAddSpamToSubjectCheckBoxPntr = new BCheckBox (
		tempRect,
		"AddToSubject",
		"Add \"[Spam %]\" in front of Subject.",
		new BMessage (kAddSpamToSubjectPressed));
	AddChild (fAddSpamToSubjectCheckBoxPntr);
	fAddSpamToSubjectCheckBoxPntr->ResizeToPreferred ();
	fAddSpamToSubjectCheckBoxPntr->SetValue (fAddSpamToSubject);
	fAddSpamToSubjectCheckBoxPntr->SetTarget (this);

	tempRect = Bounds ();
	tempRect.top = fAddSpamToSubjectCheckBoxPntr->Frame().bottom + 1;
	tempRect.bottom = tempRect.top + 20;

	// Add the checkbox on the right for the no words means spam option.

	fNoWordsMeansSpamCheckBoxPntr = new BCheckBox (
		tempRect,
		"NoWordsMeansSpam",
		"or no words found.",
		new BMessage (kNoWordsMeansSpam));
	AddChild (fNoWordsMeansSpamCheckBoxPntr);
	fNoWordsMeansSpamCheckBoxPntr->ResizeToPreferred ();
	fNoWordsMeansSpamCheckBoxPntr->MoveBy (
		floorf (tempRect.right - fNoWordsMeansSpamCheckBoxPntr->Frame().right),
		0.0);
	fNoWordsMeansSpamCheckBoxPntr->SetValue (fNoWordsMeansSpam);
	fNoWordsMeansSpamCheckBoxPntr->SetTarget (this);

	// Add the box displaying the spam cutoff ratio to the left, in the space
	// remaining between the left edge and the no words checkbox.

	tempRect.right = fNoWordsMeansSpamCheckBoxPntr->Frame().left -
		be_plain_font->StringWidth ("a");
	tempStringPntr = "Spam above:";
	sprintf (numberString, "%06.4f", (double) fSpamCutoffRatio);
	fSpamCutoffRatioTextBoxPntr	= new BTextControl (
		tempRect,
		"spamcutoffratio",
		tempStringPntr,
		numberString,
		NULL /* BMessage */);
	AddChild (fSpamCutoffRatioTextBoxPntr);
	fSpamCutoffRatioTextBoxPntr->SetDivider (
		be_plain_font->StringWidth (tempStringPntr) +
		1 * be_plain_font->StringWidth ("a"));

	tempRect = Bounds ();
	tempRect.top = fSpamCutoffRatioTextBoxPntr->Frame().bottom + 1;
	tempRect.bottom = tempRect.top + 20;

	// Add the box displaying the genuine cutoff ratio, on a line by itself.

	tempStringPntr = "Genuine below and uncertain above:";
	sprintf (numberString, "%06.4f", (double) fGenuineCutoffRatio);
	fGenuineCutoffRatioTextBoxPntr = new BTextControl (
		tempRect,
		"genuinecutoffratio",
		tempStringPntr,
		numberString,
		NULL /* BMessage */);
	AddChild (fGenuineCutoffRatioTextBoxPntr);
	fGenuineCutoffRatioTextBoxPntr->SetDivider (
		be_plain_font->StringWidth (tempStringPntr) +
		1 * be_plain_font->StringWidth ("a"));

	tempRect = Bounds ();
	tempRect.top = fGenuineCutoffRatioTextBoxPntr->Frame().bottom + 1;
	tempRect.bottom = tempRect.top + 20;

    // Checkbox for automatically training on incoming mail.

	fAutoTrainingCheckBoxPntr = new BCheckBox (
		tempRect,
		"autoTraining",
		"Self-training on Incoming Messages.",
		new BMessage (kAutoTrainingPressed));
	AddChild (fAutoTrainingCheckBoxPntr);
	fAutoTrainingCheckBoxPntr->ResizeToPreferred ();
	fAutoTrainingCheckBoxPntr->SetValue (fAutoTraining);
	fAutoTrainingCheckBoxPntr->SetTarget (this);

	tempRect = Bounds ();
	tempRect.top = fAutoTrainingCheckBoxPntr->Frame().bottom + 1;
	tempRect.bottom = tempRect.top + 20;

	// Button for editing the server settings.

	fServerSettingsButtonPntr = new BButton (
		tempRect,
		"serverSettings",
		"Edit Server Settings",
		new BMessage (kServerSettingsPressed));
	AddChild (fServerSettingsButtonPntr);
	fServerSettingsButtonPntr->ResizeToPreferred ();
	fServerSettingsButtonPntr->SetTarget (this);

	tempRect = Bounds ();
	tempRect.top = fServerSettingsButtonPntr->Frame().bottom + 1;
	tempRect.bottom = tempRect.top + 20;

    // Checkbox for closing the server when done.

	fQuitServerWhenFinishedCheckBoxPntr = new BCheckBox (
		tempRect,
		"quitWhenFinished",
		"Close AGMSBayesianSpamServer when Finished.",
		new BMessage (kQuitWhenFinishedPressed));
	AddChild (fQuitServerWhenFinishedCheckBoxPntr);
	fQuitServerWhenFinishedCheckBoxPntr->ResizeToPreferred ();
	fQuitServerWhenFinishedCheckBoxPntr->SetValue (fQuitServerWhenFinished);
	fQuitServerWhenFinishedCheckBoxPntr->SetTarget (this);

	tempRect = Bounds ();
	tempRect.top = fQuitServerWhenFinishedCheckBoxPntr->Frame().bottom + 1;
	tempRect.bottom = tempRect.top + 20;
}


status_t
AGMSBayesianSpamFilterConfig::Archive (BMessage *into, bool deep) const
{
	status_t	errorCode;
	float		tempFloat;

	into->MakeEmpty();
	errorCode = into->AddBool ("AddMarkerToSubject", fAddSpamToSubject);

	if (errorCode == B_OK)
		errorCode = into->AddBool ("AutoTraining", fAutoTraining);

	if (errorCode == B_OK)
		errorCode = into->AddBool ("QuitServerWhenFinished", fQuitServerWhenFinished);

	if (errorCode == B_OK)
		errorCode = into->AddBool ("NoWordsMeansSpam", fNoWordsMeansSpam);

	if (errorCode == B_OK) {
		tempFloat = fGenuineCutoffRatio;
		if (fGenuineCutoffRatioTextBoxPntr != NULL)
			tempFloat = atof (fGenuineCutoffRatioTextBoxPntr->Text());
		errorCode = into->AddFloat ("GenuineCutoffRatio", tempFloat);
	}

	if (errorCode == B_OK) {
		tempFloat = fSpamCutoffRatio;
		if (fSpamCutoffRatioTextBoxPntr != NULL)
			tempFloat = atof (fSpamCutoffRatioTextBoxPntr->Text());
		errorCode = into->AddFloat ("SpamCutoffRatio", tempFloat);
	}

	return errorCode;
}


void
AGMSBayesianSpamFilterConfig::GetPreferredSize (float *width, float *height) {
	*width = 260;
	*height = 130;
}


void
AGMSBayesianSpamFilterConfig::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case kAddSpamToSubjectPressed:
			fAddSpamToSubject = fAddSpamToSubjectCheckBoxPntr->Value ();
			break;
		case kAutoTrainingPressed:
			fAutoTraining = fAutoTrainingCheckBoxPntr->Value ();
			break;
		case kNoWordsMeansSpam:
			fNoWordsMeansSpam = fNoWordsMeansSpamCheckBoxPntr->Value ();
			break;
		case kQuitWhenFinishedPressed:
			fQuitServerWhenFinished =
				fQuitServerWhenFinishedCheckBoxPntr->Value ();
			break;
		case kServerSettingsPressed:
			ShowSpamServerConfigurationWindow ();
			break;
		default:
			BView::MessageReceived (msg);
	}
}


void
AGMSBayesianSpamFilterConfig::ShowSpamServerConfigurationWindow () {
	status_t    errorCode = B_OK;
	BMessage    maximizeCommand;
	BMessenger	messengerToServer;
	BMessage    replyMessage;
	team_id		serverTeam;

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
	messengerToServer =
		BMessenger (kServerSignature, serverTeam, &errorCode);
	if (!messengerToServer.IsValid ())
		goto ErrorExit;

	// Wait for the server to finish starting up, and for it to create the window.
	snooze (2000000);

	// Tell it to show its main window, in case it is hidden in server mode.
	maximizeCommand.what = B_SET_PROPERTY;
	maximizeCommand.AddBool ("data", false);
	maximizeCommand.AddSpecifier ("Minimize");
	maximizeCommand.AddSpecifier ("Window", 0L);
	errorCode = messengerToServer.SendMessage (&maximizeCommand, &replyMessage);
	if (errorCode != B_OK)
		goto ErrorExit;
	return; // Successful.

ErrorExit:
	(new BAlert ("SpamFilterConfig Error", "Sorry, unable to launch the "
		"AGMSBayesianSpamServer program to let you edit the server "
		"settings.", "Close"))->Go ();
	return;
}


BView *
instantiate_config_panel (BMessage *settings, BMessage *metadata)
{
	return new AGMSBayesianSpamFilterConfig (settings);
}
