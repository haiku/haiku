#ifndef __HEVENTITEM_H__
#define __HEVENTITEM_H__

#include "CLVEasyItem.h"
#include <String.h>
#include <Entry.h>


class HEventItem :public CLVEasyItem {
public:
						HEventItem(const char* event_name
									,const char* path);
		virtual			~HEventItem();
		
		const char*		Name()const {return fName.String();}
		const char*		Path()const;
			void		Remove();
			void		SetPath(const char* path);
protected:

private:
			BString		fName;
			BString		fPath;
			float		fText_offset;
};
#endif
