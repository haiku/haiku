//--------------------------------------------------------------------
//	
//	TToolTip.h
//
//	Written by: Robert Polic
//	
//--------------------------------------------------------------------

#ifndef T_TOOL_TIPS_H
#define T_TOOL_TIPS_H

#include <Font.h>
#include <Window.h>
#include <View.h>

enum	TOOL_TIP_MESSAGES		{eToolTipStart = 'ttGo',
							 	 eToolTipStop = 'ttSp'};

// ui settings
struct tool_tip_settings {
	bool				enabled;		// flag whether tips are enables or not
	bool				one_time_only;	// flag to only display the tip once per time in view
	bigtime_t			delay;			// delay before tip is shown in microseconds
	bigtime_t			hold;			// amount of time tip is displayed in microseconds
	BFont				font;			// font tip is drawn in
};

// internal settings
struct tool_tip {
	bool				app_active;
	bool				quit;
	bool				stop;
	bool				stopped;
	bool				reset;
	bool				shown;
	bool				showing;
	bool				tip_timed_out;
	BPoint				start;
	BRect				bounds;
	class TToolTipView	*tool_tip_view;
	BWindow				*tool_tip_window;
	bigtime_t			start_time;
	bigtime_t			expire_time;
	tool_tip_settings	settings;
};


//====================================================================

class TToolTip : public BWindow {

public:
						TToolTip(tool_tip_settings *settings = NULL);
	virtual void		MessageReceived(BMessage*);

	void				GetSettings(tool_tip_settings*);
	void				SetSettings(tool_tip_settings*);

private:
	class TToolTipView	*fView;
};


//====================================================================

class TToolTipView : public BView {

public:
						TToolTipView(tool_tip_settings *settings = NULL);
						~TToolTipView();
	virtual void		AllAttached();
	virtual void		Draw(BRect);
	virtual void		MessageReceived(BMessage*);

	void				GetSettings(tool_tip_settings*);
	void				SetSettings(tool_tip_settings*);

private:
	void				AdjustWindow();
	static status_t		ToolTipThread(tool_tip*);

	char				*fString;
	thread_id			fThread;
	tool_tip			fTip;
};
#endif
