/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 */
#ifndef MESSENGER_PRIVATE_H
#define MESSENGER_PRIVATE_H


#include <Messenger.h>


class BMessenger::Private {
	public:
		Private(BMessenger* messenger) : fMessenger(messenger) {}
		Private(BMessenger& messenger) : fMessenger(&messenger) {}

		port_id	Port()
			{ return fMessenger->fPort; }
		int32 Token()
			 { return fMessenger->fHandlerToken; }
		team_id	Team()
			{ return fMessenger->fTeam; }
		bool IsPreferredTarget()
			{ return fMessenger->fPreferredTarget; }

		void SetTo(team_id team, port_id port, int32 token, bool preferred)
			{ fMessenger->SetTo(team, port, token, preferred); }

	private:
		BMessenger* fMessenger;
};

#endif	// MESSENGER_PRIVATE_H
