#ifndef TERMINAL_VIEW_H
#define TERMINAL_VIEW_H

#include <File.h>
#include <TextView.h>
#include <DataIO.h>

class TerminalView : public BTextView {
public:
	TerminalView(BRect viewframe, BRect textframe, BHandler *handler);
	~TerminalView();

	virtual void Select(int32 start, int32 finish);
			
private:
	BHandler	*fHandler;
	BMessage	*fChangeMessage;
	BMessenger 	*fMessenger;
};

#endif // TERMINAL_VIEW_H
