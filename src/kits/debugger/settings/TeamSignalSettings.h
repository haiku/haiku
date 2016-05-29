/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_SIGNAL_SETTINGS_H
#define TEAM_SIGNAL_SETTINGS_H

#include <Message.h>

#include "SignalDispositionTypes.h"


class TeamSignalSettings {
public:
								TeamSignalSettings();
	virtual						~TeamSignalSettings();

			TeamSignalSettings&
								operator=(
									const TeamSignalSettings& other);
									// throws std::bad_alloc;

			const char*			ID() const;
			status_t			SetTo(const BMessage& archive);
			status_t			WriteTo(BMessage& archive) const;
			void				Unset();

			void				SetDefaultSignalDisposition(int32 disposition);
			int32				DefaultSignalDisposition() const;

			int32				CountCustomSignalDispositions() const;
			status_t			AddCustomSignalDisposition(int32 signal,
									int32 disposition);
			status_t			RemoveCustomSignalDispositionAt(int32 index);
			status_t			GetCustomSignalDispositionAt(int32 index,
									int32& signal, int32& disposition) const;

	virtual	TeamSignalSettings*
								Clone() const;

private:
	BMessage					fValues;
};


#endif	// TEAM_SIGNAL_SETTINGS_H
