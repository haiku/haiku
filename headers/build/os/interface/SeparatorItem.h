#ifndef _SEPARATOR_ITEM_H
#define _SEPARATOR_ITEM_H
 
#include <MenuItem.h>

class BMessage;
class BSeparatorItem : public BMenuItem {
public:
	BSeparatorItem();
	BSeparatorItem(BMessage *data);
	
	virtual ~BSeparatorItem();
	virtual	status_t Archive(BMessage *data, bool deep = true) const;

	static BArchivable *Instantiate(BMessage *data);
	virtual	void SetEnabled(bool state);

protected:
	virtual	void GetContentSize(float *width, float *height);
	virtual	void Draw();

private:
	virtual	void _ReservedSeparatorItem1();
	virtual	void _ReservedSeparatorItem2();

	BSeparatorItem &operator=(const BSeparatorItem &);

	uint32 _reserved[1];
};


#endif /* _SEPARATOR_ITEM_H */
