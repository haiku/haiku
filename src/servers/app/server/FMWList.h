#ifndef _FMWLIST_H_
#define _FMWLIST_H_

#include <List.h>
#include <Window.h>

class FMWList{
public:
							FMWList();
	virtual					~FMWList();
			void			AddItem(void* item);
			bool			HasItem(void* item) const;
			bool			RemoveItem(void* item);
			void*			ItemAt(int32 i) const;
			int32			CountItems() const;
			void*			LastItem() const;
			void*			FirstItem() const;
			int32			IndexOf(void *item);
	// special
			void			AddFMWList(FMWList *list);
	// debugging methods
			void			PrintToStream() const;

private:

			BList			fList;
};

#endif
