// BMPView.h

#ifndef BMPVIEW_H
#define BMPVIEW_H

#include <View.h>
#include <MenuField.h>
#include <MenuItem.h>

class BMPView : public BView
{
public:
	BMPView(const BRect &frame, const char *name, uint32 resize, uint32 flags);
	~BMPView();

	virtual	void Draw(BRect area);
};

#endif // #ifndef BMPVIEW_H
