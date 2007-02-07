#ifndef INLINE_EDITOR
#define INLINE_EDITOR

#include <Window.h>
#include <TextControl.h>
#include <Messenger.h>

#define M_INLINE_TEXT 'intx'

class InlineEditor : public BWindow
{
public:
			InlineEditor(BMessenger target, const BRect &frame,
						const char *text);
	bool	QuitRequested(void);
	void	SetMessage(BMessage *msg);
	void	MessageReceived(BMessage *msg);
	void	WindowActivated(bool active);
	
private:
	BTextControl	*fTextBox;
	BMessenger		fMessenger;
	uint32			fCommand;
};


#endif
