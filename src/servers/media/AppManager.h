
#include <List.h>
#include <Locker.h>

class AppManager
{
public:
	AppManager();
	~AppManager();
	bool HasTeam(team_id);
	status_t RegisterTeam(team_id, BMessenger);
	status_t UnregisterTeam(team_id);
	void BroadcastMessage(BMessage *msg, bigtime_t timeout);
	void HandleBroadcastError(BMessage *, BMessenger &, team_id team, bigtime_t timeout);
	status_t LoadState();
	status_t SaveState();
private:
	struct ListItem {
		team_id team;
		BMessenger messenger;
	};
	BList 	mList;
	BLocker	mLocker;
};
