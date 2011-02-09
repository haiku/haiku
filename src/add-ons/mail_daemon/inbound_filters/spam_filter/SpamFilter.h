/*
 * Copyright 2002-2011, Haiku, Inc. All rights reserved.
 * Copyright 2002 Alexander G. M. Smith.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef AGMS_BAYESIAN_SPAM_FILTER_H
#define AGMS_BAYESIAN_SPAM_FILTER_H
/******************************************************************************
 * $Id: SpamFilter.h 13952 2005-08-13 19:31:42Z agmsmith $
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
 * r9669 | brunoga | 2004-10-30 18:23:26 -0400 (Sat, 30 Oct 2004) | 2 lines
 * AGMS Spam Filter.
 *
 * Revision 1.8  2004/09/20 15:57:30  nwhitehorn
 * Mostly updated the tree to Be/Haiku style identifier naming conventions. I have a few more things to work out, mostly in mail_util.h, and then I'm proceeding to jamify the build system. Then we go into Haiku CVS.
 *
 * Revision 1.7  2003/05/27 17:12:59  nwhitehorn
 * Massive refactoring of the Protocol/ChainRunner/Filter system. You can probably
 * examine its scope by examining the number of files changed. Regardless, this is
 * preparation for lots of new features, and REAL WORKING IMAP. Yes, you heard me.
 * Enjoy, and prepare for bugs (although I've fixed all the ones I've found, I susp
 * ect there are some memory leaks in ChainRunner).
 *
 * Revision 1.6  2003/02/08 21:54:17  agmsmith
 * Updated the AGMSBayesianSpamServer documentation to match the current
 * version.  Also removed the Beep options from the spam filter, now they
 * are turned on or off in the system sound preferences.
 *
 * Revision 1.5  2002/12/18 02:27:45  agmsmith
 * Added uncertain classification as suggested by BiPolar.
 *
 * Revision 1.4  2002/12/12 00:56:28  agmsmith
 * Added some new spam filter options - self training (not implemented yet)
 * and a button to edit the server settings.
 *
 * Revision 1.3  2002/11/28 20:20:57  agmsmith
 * Now checks if the spam database is running in headers only mode, and
 * then only downloads headers if that is the case.
 *
 * Revision 1.2  2002/11/10 19:36:27  agmsmith
 * Retry launching server a few times, but not too many.
 *
 * Revision 1.1  2002/11/03 02:06:15  agmsmith
 * Added initial version.
 *
 * Revision 1.5  2002/10/21 16:13:59  agmsmith
 * Added option to have no words mean spam.
 *
 * Revision 1.4  2002/10/11 20:01:28  agmsmith
 * Added sound effects (system beep) for genuine and spam, plus config option
 * for it.
 *
 * Revision 1.3  2002/09/23 19:14:13  agmsmith
 * Added an option to have the server quit when done.
 *
 * Revision 1.2  2002/09/23 03:33:34  agmsmith
 * First working version, with cutoff ratio and subject modification,
 * and an attribute added if a patch is made to the Folder filter.
 *
 * Revision 1.1  2002/09/21 20:47:57  agmsmith
 * Initial revision
 */

#include <Message.h>
#include <List.h>
#include <MailAddon.h>


class AGMSBayesianSpamFilter : public MailFilter {
public:
								AGMSBayesianSpamFilter(MailProtocol& protocol,
									AddonSettings* settings);
								~AGMSBayesianSpamFilter();

			void				HeaderFetched(const entry_ref& ref, BFile* file);
			void				BodyFetched(const entry_ref& ref, BFile* file);

private:
			status_t			_CheckForSpam(BFile* file);
			//! if the server is not running start it
			status_t			_CheckForSpamServer();
			status_t			_GetTokenizeMode();
			status_t			_GetSpamRatio(const char* data, off_t dataSize,
									float& ratio);
			status_t			_TrainServer(const char* data, off_t dataSize,
									float spamRatio);
			status_t			_AddSpamToSubject(BNode* file, float spamRatio);

			bool				fAddSpamToSubject;
			bool				fAutoTraining;
			float				fGenuineCutoffRatio;
			bool				fHeaderOnly;
			int					fLaunchAttemptCount;
			BMessenger			fMessengerToServer;
			bool				fNoWordsMeansSpam;
			bool				fQuitServerWhenFinished;
			float				fSpamCutoffRatio;
};

#endif	/* AGMS_BAYESIAN_SPAM_FILTER_H */
