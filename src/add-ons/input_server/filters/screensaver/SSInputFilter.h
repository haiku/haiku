#include "InputServerFilter.h"
#include "Rect.h"
#include "Region.h"
#include "Messenger.h"
#include "MessageFilter.h"
#include "MessageRunner.h"

const int CORNER_SIZE=10;
const int timeUp='TMUP';

class SSISFilter: public BInputServerFilter
{
public:
	SSISFilter();
	virtual ~SSISFilter();
	virtual filter_result Filter(BMessage *msg, BList *outList);
	void CheckTime(void);
	enum cornerPos {MIDDLE='ENDC',TOPL='TOPL',TOPR='TOPR',BOTL='BOTL',BOTR='BOTR'};
private:
	cornerPos blank, keep, current; 
	uint32 first,blankTime;
	bool enabled;
	BMessenger *ssApp;
	BRect topLeft,topRight,bottomLeft,bottomRight;
	BMessageRunner *runner;
	BMessageFilter *filter;
	BMessenger *messenger;
	BMessage *timeMsg;

	void UpdateRectangles(void);
	void Cornered(cornerPos pos);
	void Invoke(void);
	void Banish(void);
};

