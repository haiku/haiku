/*
 * Copyright 2002-2013, Haiku, Inc. All rights reserved.
 * Copyright 2002 Alexander G. M. Smith.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef SPAM_FILTER_H
#define SPAM_FILTER_H


#include <MailFilter.h>
#include <Messenger.h>


class SpamFilter : public BMailFilter {
public:
								SpamFilter(BMailProtocol& protocol,
									const BMailAddOnSettings& settings);
	virtual						~SpamFilter();

	virtual	BMailFilterAction	HeaderFetched(entry_ref& ref, BFile& file,
									BMessage& attributes);
	virtual	void				BodyFetched(const entry_ref& ref, BFile& file,
									BMessage& attributes);

private:
			status_t			_CheckForSpam(BFile& file);
			//! If the server is not running start it
			status_t			_CheckForSpamServer();
			status_t			_GetTokenizeMode();
			status_t			_GetSpamRatio(const char* data, off_t dataSize,
									float& ratio);
			status_t			_TrainServer(const char* data, off_t dataSize,
									float spamRatio);
			status_t			_AddSpamToSubject(BNode& file, float spamRatio);

private:
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


#endif	// SPAM_FILTER_H
