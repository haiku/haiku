/*
 * Copyright 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2004-2007, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef ZOIDBERG_STATUS_WINDOW_H
#define ZOIDBERG_STATUS_WINDOW_H


#include <Alert.h>
#include <Box.h>
#include <List.h>
#include <Node.h>
#include <Window.h>

class BStatusBar;
class BStringView;
class MailStatusView;

class MailStatusWindow : public BWindow {
	public:
		MailStatusWindow(BRect rect, const char *name, uint32 showMode);
		~MailStatusWindow();

		virtual	void FrameMoved(BPoint origin);
		virtual void WorkspaceActivated(int32 workspace, bool active);
		virtual void MessageReceived(BMessage *msg);

		MailStatusView *NewStatusView(const char *description, bool upstream);
		void	RemoveView(MailStatusView *view);
		int32	CountVisibleItems();

		bool	HasItems(void);
		void	SetShowCriterion(uint32);
		void	SetDefaultMessage(const BString &message);

	private:
		friend class MailStatusView;

		void	_CheckChains();
		void	SetBorderStyle(int32 look);
		void	ActuallyAddStatusView(MailStatusView *status);

		node_ref	fChainDirectory;
		BButton*	fCheckNowButton;
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

class MailStatusView : public BBox {
public:
								~MailStatusView();

			void				AddProgress(int32 how_much);
			void				SetMessage(const char *msg);
			void				SetMaximum(int32 max_bytes);
			int32				CountTotalItems();
			void				SetTotalItems(int32 items);
			void				AddItem(void);
			void				Reset(bool hide = true);
			int32				ItemsNow() { return items_now; }

private:
		friend class	MailStatusWindow;
		
						MailStatusView(BRect rect,const char *description,bool upstream);
				void	AddSelfToWindow();
		
		BStatusBar		*status;
		MailStatusWindow	*window;
		int32			items_now;
		int32			total_items;
		bool			is_upstream;
		bool			by_bytes;
		char 			pre_text[255];

		uint32			_reserved[5];
};

#endif	/* ZOIDBERG_STATUS_WINDOW_H */
