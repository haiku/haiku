#include "InputServerFilter.h"
#include "Rect.h"
#include "Region.h"

const int CORNER_SIZE=10;

class SSISFilter: public BInputServerFilter
{
public:
	SSISFilter();
	virtual ~SSISFilter();
	virtual filter_result Filter(BMessage *msg, BList *outList);
private:
	uint32 first;
	bool enabled;
	BMessenger *ssApp;
	BRect topLeft,topRight,bottomLeft,bottomRight;
	//enum cornerPos {MIDDLE=(int32)'ENDC',TOPL=(int32)'TOPL',TOPR=(int32)'TOPR',BOTL=(int32)'BOTL',BOTR=(int32)'BOTR'};
	enum cornerPos {MIDDLE='ENDC',TOPL='TOPL',TOPR='TOPR',BOTL='BOTL',BOTR='BOTR'};
	void UpdateRectangles(void);
	void Cornered(cornerPos pos);
};

