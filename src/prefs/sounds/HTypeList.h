#ifndef __HTYPELIST_H__
#define __HTYPELIST_H__

#include <ListView.h>

enum{
	M_TYPE_CHANGED = 'TCHD'
};

class HTypeList :public BListView{
public:
						HTypeList(BRect rect,const char* name = "TypeList");
		virtual			~HTypeList();
protected:
				void	AddType(const char* name);
				void	Init();
				void	RemoveAll();
		virtual void	SelectionChanged();
private:
				BList	fPointerList;		
};
#endif 