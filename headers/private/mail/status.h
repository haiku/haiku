#ifndef ZOIDBERG_STATUS_WINDOW_H
#define ZOIDBERG_STATUS_WINDOW_H
/* StatusWindow - the status window while fetching/sending mails
**
** Copyright 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Window.h>
#include <Box.h>
#include <List.h>
#include <Alert.h>

class BStatusBar;
class BStringView;
class BMailStatusView;

class BMailStatusWindow : public BWindow {
	public:
		BMailStatusWindow(BRect rect, const char *name, uint32 show_when);
		~BMailStatusWindow();

		virtual	void FrameMoved(BPoint origin);
		virtual void WorkspaceActivated(int32 workspace, bool active);
		virtual void MessageReceived(BMessage *msg);

		BMailStatusView *NewStatusView(const char *description, bool upstream);
		void	RemoveView(BMailStatusView *view);
		int32	CountVisibleItems();

		bool	HasItems(void);
		void	SetShowCriterion(uint32);
		void	SetDefaultMessage(const BString &message);

	private:
		friend class BMailStatusView;

		void	SetBorderStyle(int32 look);
		void	ActuallyAddStatusView(BMailStatusView *status);

		BList		fStatusViews;
		uint32		fShowMode;
		BView		*fDefaultView;
		BStringView	*fMessageView;
		float		fMinWidth;
		float		fMinHeight;
		int32		fWindowMoved;
		int32		fLastWorkspace;
		BRect		fFrame;
		
		uint32		_reserved[5];
};

class BMailStatusView : public BBox {
	public:
				void	AddProgress(int32 how_much);
				void	SetMessage(const char *msg);
				void	SetMaximum(int32 max_bytes);
				int32	CountTotalItems();
				void	SetTotalItems(int32 items);
				void	AddItem(void);
				void	Reset(bool hide = true);
		
		virtual			~BMailStatusView();

	private:
		friend class	BMailStatusWindow;
		
						BMailStatusView(BRect rect,const char *description,bool upstream);
				void	AddSelfToWindow();
		
		BStatusBar		*status;
		BMailStatusWindow	*window;
		int32			items_now;
		int32			total_items;
		bool			is_upstream;
		bool			by_bytes;
		char 			pre_text[255];

		uint32			_reserved[5];
};

#endif	/* ZOIDBERG_STATUS_WINDOW_H */
