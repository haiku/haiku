#ifndef TERMINAL_TEXT_VIEW_H
#define TERMINAL_TEXT_VIEW_H

#include <TextView.h>

class TerminalTextView : public BTextView {
public:
	TerminalTextView(BRect viewframe, BRect textframe,
	                 const BFont * initialFont, const rgb_color * initialColor,
	                 BHandler *handler);
	~TerminalTextView();

	virtual void Select(int32 start, int32 finish);
	virtual void FrameResized(float width, float height);

private:
	BHandler	*fHandler;
	BMessenger 	*fMessenger;
};

#endif // TERMINAL_TEXT_VIEW_H
