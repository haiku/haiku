#ifndef __HEVENTLIST_H__
#define __HEVENTLIST_H__

#include "ColumnListView.h"

class HEventItem;

enum{
	M_EVENT_CHANGED = 'SCAG'
};

class HEventList :public ColumnListView {
public:
					HEventList(BRect rect
						,CLVContainerView** ContainerView
						,const char* name="EventList");
		virtual			~HEventList();
				void	RemoveAll();
				void	AddEvent(HEventItem *item);
				void	RemoveEvent(HEventItem *item);
				
				void	SetType(const char* type);
protected:
		virtual void	MouseDown(BPoint point);
		virtual void	SelectionChanged();
private:
				BList	fPointerList;
				
};
#endif
