#ifndef PRINTTESTVIEW_HPP
#define PRINTTESTVIEW_HPP

class PrintTestView;

#include <View.h>

class PrintTestView : public BView
{
	typedef BView Inherited;
public:
	PrintTestView(BRect frame);
	void Draw(BRect updateRect);
};

#endif
