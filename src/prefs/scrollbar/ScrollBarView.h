#ifndef SCROLLBAR_VIEW
#define SCROLLBAR_VIEW

#include <View.h>
#include <Box.h>
#include <StringView.h>
#include <Button.h>
#include <ScrollBar.h>


class ScrollBarView : public BView
{
public:
	ScrollBarView(void);
	~ScrollBarView(void);
	void MessageReceived(BMessage *msg);

	BBox *arrowstyleBox, *knobtypeBox, *knobstyleBox, *minknobsizeBox;
	BStringView *singleStringView,*doubleStringView, *proportionalStringView,
		*fixedStringView;
	BButton *defaultsButton, *revertButton;
	
	BScrollBar *sbsingle, *sbdouble, *sbproportional, *sbfixed,
		*sbflat, *sbdots, *sblines;
};

#endif
