//Name:		TextEntryAlert.h
//Author:	Brian Tietz
//Conventions:
//	Global constants (declared with const) and #defines - begin with "c_" followed by lowercase
//		words separated by underscores.
//		(E.G., #define c_my_constant 5).
//		(E.G., const int c_my_constant = 5;).
//	Global variables - begin with "g_" followed by lowercase words separated by underscores.
//		(E.G., int g_my_global;).
//	New data types (classes, structs, typedefs, etc.) - begin with an uppercase letter followed by
//		lowercase words separated by uppercase letters.  Enumerated constants contain a prefix
//		associating them with a particular enumerated set.
//		(E.G., typedef int MyTypedef;).
//		(E.G., enum MyEnumConst {c_mec_one, c_mec_two};)
//	Private member variables - begin with "m_" followed by lowercase words separated by underscores.
//		(E.G., int m_my_member;).
//	Public or friend-accessible member variables - all lowercase words separated by underscores.
//		(E.G., int public_member;).
//	Argument and local variables - begin with a lowercase letter followed by
//		lowercase words separated by underscores.  If the name is already taken by a public member
//		variable, prefix with a_ or l_
//		(E.G., int my_local; int a_my_arg, int l_my_local).
//	Functions (member or global) - begin with an uppercase letter followed by lowercase words
//		separated by uppercase letters.
//		(E.G., void MyFunction(void);).


#ifndef _TEXT_ENTRY_ALERT_H_
#define _TEXT_ENTRY_ALERT_H_


//******************************************************************************************************
//**** System header files
//******************************************************************************************************
#include <Window.h>
#include <TextView.h>
#include <MessageFilter.h>


//******************************************************************************************************
//**** Forward name declarations
//******************************************************************************************************
class TextEntryAlertTextEntryView;


//******************************************************************************************************
//**** TextEntryAlert
//******************************************************************************************************
class TextEntryAlert : public BWindow
{
	public:
		TextEntryAlert(const char* title, const char* info_text, const char* initial_entry_text, 
			const char* button_0_label, const char* button_1_label = NULL,
			bool inline_label = true, float min_text_box_width = 120, int min_text_box_rows = 1,
			button_width width_style = B_WIDTH_AS_USUAL, bool multi_line = false,
			const BRect* frame = NULL, window_look look = B_MODAL_WINDOW_LOOK,
			window_feel feel = B_MODAL_APP_WINDOW_FEEL,
			uint32 flags = B_NOT_RESIZABLE|B_ASYNCHRONOUS_CONTROLS);
			//The title is not displayed unless you use a window_look other than the default.  info_text
			//is the label applied to the text entry box.  If inline_label is true, then the label is
			//placed to the left of the text entry box, otherwise it is placed above it.  If a frame is
			//not specified, the TextEntryAlert will be auto-sized and auto-positioned to accommodate
			//the text.  The minimum text entry box size can be specified using min_text_box_width and
			//text_box_rows.  Multi-line specifies whether the user can hit return and enter multiple
			//lines of text, regardless of the height of the text box.
		~TextEntryAlert();

		int32 Go(char* text_entry_buffer, int32 buffer_size);
			//Synchronous version: The function doesn't return until the user has clicked a button and
			//the panel has been removed from the screen.  The value it returns is the index of 
			//the clicked button (0 or 1, left-to-right).  The user-entered (or unchanged) text is
			//stored in text_entry_buffer.  The TextEntryAlert is deleted before it returns, and should
			//be considered invalid after Go is called.  If the TextEntryAlert is sent a
			//B_QUIT_REQUESTED message while the panel is still on-screen, it returns -1.
		status_t Go(BInvoker* invoker);
			//Asynchronous version: The function returns immediately (with B_OK) and the button index is
			//delivered as the int32 "which" field of the BMessage that's sent to the BInvoker's target,
			//and the text message is delivered as the string "entry_text" field of the BMessage.  The
			//TextEntryBox will take posession of the BInvoker and delete it when finished.  The
			//TextEntryAlert is deleted when the user hits any of the buttons, and should be considered
			//invalid after Go is called.  If the TextEntryAlert is sent a B_QUIT_REQUESTED message
			//while the panel is still on-screen, it suppresses sending of the message.

		void SetShortcut(int32 index, char shortcut);
		char Shortcut(int32 index) const;
			//These functions set and return the shortcut character that's mapped to the button at
			//index. A given button can have only one shortcut except for the rightmost button, which, 
			//in addition to the shortcut that you give it here, is always mapped to B_ENTER.  If you 
			//create a "Cancel" button, you should give it a shortcut of B_ESCAPE.   If multi_line was
			//specified as true in the TextEntryAlert constructor, no shortcuts are allowed, including
			//the built-in B_ENTER shortcut on the rightmost button.

		void SetTabAllowed(bool allow);
			//By default, tab is not allowed, being used for navigation instead.  Set this to true to
			//allow the user to actually enter a tab character in the text entry box.
		bool TabAllowed();

		BTextView* LabelView(void) const;
		BTextView* TextEntryView(void) const;
			//Returns a pointer to the BTextView object that contains the textual information or the
			//user-editable text that's displayed in the panel, respectively. You can fiddle with these
			//objects but you mustn't delete them.

		//BWindow overrides
		virtual void MessageReceived(BMessage* message);
		virtual void Quit();

	private:
		BTextView* m_label_view;
		TextEntryAlertTextEntryView* m_text_entry_view;
		static filter_result KeyDownFilterStatic(BMessage* message, BHandler** target,
			BMessageFilter* filter);
		filter_result KeyDownFilter(BMessage* message);
		BButton* m_buttons[2];
		char m_shortcut[2];

		//For the synchronous version (pointers point to data areas owned by thread that called Go)
		sem_id m_done_mutex;		//Mutex to release when the user hits a button or the window closes
		char* m_text_entry_buffer;	//Buffer to store the user-entered text when the user hits a button
		int32* m_button_pressed;	//Place to store the button index that the user hit
		int32 m_buffer_size;

		//For the asynchronous version
		BInvoker *m_invoker;		//I own this object and will delete it when done.
};


//******************************************************************************************************
//**** TextEntryAlertBackgroundView
//******************************************************************************************************
class TextEntryAlertBackgroundView : public BView
{
	public:
		TextEntryAlertBackgroundView(BRect frame, BRect entry_text_rect);
		virtual void Draw(BRect update_rect);
		virtual void FrameResized(float width, float heigh);

	private:
		BRect m_entry_text_rect;
		BRect m_cached_bounds;
		rgb_color m_dark_1_color;
		rgb_color m_dark_2_color;
};


//******************************************************************************************************
//**** TextEntryAlertTextEntryView
//******************************************************************************************************
class TextEntryAlertTextEntryView : public BTextView
{
	public:
		TextEntryAlertTextEntryView(BRect frame, bool multi_line);

		void SetTabAllowed(bool allow);
		inline bool TabAllowed() {return m_tab_allowed;}
		virtual void KeyDown(const char* bytes, int32 num_bytes);
		virtual void AttachedToWindow();

	private:
		bool m_tab_allowed;
		bool m_multi_line;
};


#endif //_TEXT_ENTRY_ALERT_H_
