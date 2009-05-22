/*
 * Copyright 2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_FILE_PANEL_H
#define _FILE_PANEL_H


#include <Directory.h>
#include <Entry.h>
#include <Node.h>

class BMessage;
class BMessenger;
class BWindow;
struct stat;
struct stat_beos;


class BRefFilter {
	public:
#if __GNUC__ > 2
		virtual		 ~BRefFilter() {};
#endif
		virtual	bool Filter(const entry_ref* ref, BNode* node,
						struct stat_beos* stat, const char* mimeType) = 0;
};


enum file_panel_mode {
	B_OPEN_PANEL,
	B_SAVE_PANEL
};

enum file_panel_button {
	B_CANCEL_BUTTON,
	B_DEFAULT_BUTTON
};


class BFilePanel {
	public:
		BFilePanel(file_panel_mode mode = B_OPEN_PANEL,
			BMessenger* target = NULL, const entry_ref* directory = NULL,
			uint32 nodeFlavors = 0, bool allowMultipleSelection = true,
			BMessage* message = NULL, BRefFilter* refFilter = NULL,
			bool modal = false, bool hideWhenDone = true);
		virtual	~BFilePanel();

		void			Show();
		void			Hide();
		bool			IsShowing() const;

		virtual	void	WasHidden();
		virtual	void	SelectionChanged();
		virtual	void	SendMessage(const BMessenger* target, BMessage* message);

		BWindow*		Window() const;
		BMessenger		Messenger() const;
		BRefFilter*		RefFilter() const;

		file_panel_mode	PanelMode() const;

		void			SetTarget(BMessenger target);
		void			SetMessage(BMessage* message);

		void			SetRefFilter(BRefFilter* filter);
		void			SetSaveText(const char* text);
		void			SetButtonLabel(file_panel_button button, const char* label);

		void			SetPanelDirectory(const BEntry* newDirectory);
		void			SetPanelDirectory(const BDirectory* newDirectory);
		void			SetPanelDirectory(const entry_ref* newDirectory);
		void			SetPanelDirectory(const char* newDirectory);
		void			GetPanelDirectory(entry_ref* ref) const;

		void			SetHideWhenDone(bool hideWhenDone);
		bool			HidesWhenDone() const;

		void			Refresh();
		void			Rewind();
		status_t		GetNextSelectedRef(entry_ref* ref);

	private:
		virtual	void	_ReservedFilePanel1();
		virtual	void	_ReservedFilePanel2();
		virtual	void	_ReservedFilePanel3();
		virtual	void	_ReservedFilePanel4();
		virtual	void	_ReservedFilePanel5();
		virtual	void	_ReservedFilePanel6();
		virtual	void	_ReservedFilePanel7();
		virtual	void	_ReservedFilePanel8();

		BWindow*		fWindow;
		uint32			_reserved[10];
};

#endif	/* _FILE_PANEL_H */
