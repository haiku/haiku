#ifndef MESSENGERPRIVAGE_H
#define MESSENGERPRIVATE_H

#include <Messenger.h>

class BMessenger::Private
{
	public:
		Private(BMessenger* msnger) : fMessenger(msnger) {;}
		Private(BMessenger& msnger) : fMessenger(&msnger) {;}

		port_id	Port()
			{ return fMessenger->fPort; }
		int32	Token()
			 { return fMessenger->fHandlerToken; }
		team_id	Team()
			{ return fMessenger->fTeam; }
		bool	IsPreferredTarget()
			{ return fMessenger->fPreferredTarget; }

	private:
		BMessenger* fMessenger;
};

#endif	// MESSENGERPRIVATE_H
