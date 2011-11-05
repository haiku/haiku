/**App window for the FileSoundWindow test
	@author Tim de Jong
	@date 21/09/2002
	@version 1.0
 */


#ifndef FILE_SOUND_WINDOW
#define FILE_SOUND_WINDOW


#include <Button.h>
#include <CheckBox.h>
#include <Entry.h>
#include <FileGameSound.h>
#include <FilePanel.h>
#include <Message.h>
#include <Rect.h>
#include <TextControl.h>
#include <Window.h>


//message constants
#define BROWSE_MSG 1
#define PLAY_MSG 2
#define PAUSE_MSG 3 
#define LOOP_MSG 4
#define PRELOAD_MSG 5
#define DELAY_MSG 6

class FileSoundWindow : public BWindow
{
	public:
		FileSoundWindow(BRect windowBounds);
		virtual ~FileSoundWindow();
		virtual void MessageReceived(BMessage *message);
		virtual bool QuitRequested();
	private:
		BCheckBox *loopCheck;
		BCheckBox *preloadCheck;
		BFileGameSound *fileSound;
		BFilePanel *openPanel;
		BTextControl *textControl;
		BButton *playButton;
		BButton *pauseButton;
		BTextControl *delayControl;
		//private variables
		bool loop;
		bool paused;
		bool preload;
		bool playing;
		bigtime_t rampTime;
		entry_ref fileref;	
};
#endif
