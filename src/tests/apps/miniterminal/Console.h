#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <SupportDefs.h>

#define TAB_SIZE 8
#define TAB_MASK 7

#define FMASK 0x0f
#define BMASK 0x70

typedef enum {
	CONSOLE_STATE_NORMAL = 0,
	CONSOLE_STATE_GOT_ESCAPE,
	CONSOLE_STATE_SEEN_BRACKET,
	CONSOLE_STATE_NEW_ARG,
	CONSOLE_STATE_PARSING_ARG,
} console_state;

typedef enum {
	SCREEN_ERASE_WHOLE,
	SCREEN_ERASE_UP,
	SCREEN_ERASE_DOWN
} erase_screen_mode;

typedef enum {
	LINE_ERASE_WHOLE,
	LINE_ERASE_LEFT,
	LINE_ERASE_RIGHT
} erase_line_mode;

#define MAX_ARGS 8

class ViewBuffer;

class Console {
public:
							Console(ViewBuffer *output);
							~Console();

		void				ResetConsole();

		void				SetScrollRegion(int top, int bottom);
		void				ScrollUp();
		void				ScrollDown();

		void				LineFeed();
		void				RLineFeed();
		void				CariageReturn();

		void				Delete();
		void				Tab();

		void				EraseLine(erase_line_mode mode);
		void				EraseScreen(erase_screen_mode mode);

		void				SaveCursor(bool save_attrs);
		void				RestoreCursor(bool restore_attrs);
		void				UpdateCursor(int x, int y);
		void				GotoXY(int new_x, int new_y);

		void				PutChar(const char c);

		void				SetVT100Attributes(int32 *args, int32 argCount);
		bool				ProcessVT100Command(const char c, bool seen_bracket, int32 *args, int32 argCount);

		void				Write(const void *buf, size_t len);

private:
static	void				ResizeCallback(int32 width, int32 height, void *data);

		int32				fLines;
		int32				fColumns;

		uint8				fAttr;
		uint8				fSavedAttr;
		bool				fBrightAttr;
		bool				fReverseAttr;

		int32				fX;				// current x coordinate
		int32				fY;				// current y coordinate
		int32				fSavedX;		// used to save x and y
		int32				fSavedY;

		int32				fScrollTop;		// top of the scroll region
		int32				fScrollBottom;	// bottom of the scroll region

		/* state machine */
		console_state		fState;
		int32				fArgCount;
		int32				fArgs[MAX_ARGS];

		ViewBuffer			*fOutput;
};

#endif
