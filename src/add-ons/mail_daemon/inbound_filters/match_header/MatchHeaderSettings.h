/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MATCH_HEADER_SETTINGS_H
#define MATCH_HEADER_SETTINGS_H


#include <MailSettings.h>


enum rule_action {
	ACTION_MOVE_TO,
	ACTION_SET_FLAGS_TO,
	ACTION_DELETE_MESSAGE,
	ACTION_REPLY_WITH,
	ACTION_SET_AS_READ
};


class MatchHeaderSettings {
public:
	MatchHeaderSettings(const BMailAddOnSettings& settings)
		:
		fSettings(settings)
	{
	}

	rule_action Action() const
	{
		return (rule_action)fSettings.GetInt32("action", -1);
	}

	const char* Attribute() const
	{
		return fSettings.FindString("attribute");
	}

	const char* Expression() const
	{
		return fSettings.FindString("regex");
	}

	const char* MoveTarget() const
	{
		return fSettings.FindString("regex");
	}

	const char* SetFlagsTo() const
	{
		return fSettings.FindString("set flags");
	}

	const int32 ReplyAccount() const
	{
		return fSettings.GetInt32("account", -1);
	}

private:
			const BMailAddOnSettings& fSettings;
};


#endif	// MATCH_HEADER_SETTINGS_H
