#include "InputServerFilter.h"
#include "Rect.h"
#include "Region.h"
#include "Messenger.h"
#include "MessageFilter.h"
#include "MessageRunner.h"
#include "ScreenSaverPrefs.h"

const int CORNER_SIZE=10;
const int timeUp='TMUP';

int32 threadFunc(void *);

class SSISFilter: public BInputServerFilter
{
public:
	SSISFilter();
	virtual ~SSISFilter();
	virtual filter_result Filter(BMessage *msg, BList *outList);
	void CheckTime(void);
	uint32 getSnoozeTime(void) {return snoozeTime;}
private:
	uint32 lastEventTime,blankTime,snoozeTime,rtc;
	arrowDirection blank, keep, current; 
	bool enabled;
	BRect topLeft,topRight,bottomLeft,bottomRight;
	ScreenSaverPrefs pref;
	thread_id watcher;
	char frameNum; // Used so that we don't update the screen coord's so often
				// Ideally, we would get a message when the screen changes. R5 doesn't do this.

	void UpdateRectangles(void);
	void Cornered(arrowDirection pos);
	void Invoke(void);
	void Banish(void);
};

