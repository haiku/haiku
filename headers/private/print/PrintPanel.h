/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun, <host.haiku@gmx.de
 */
#ifndef _PRINT_PANEL_H_
#define _PRINT_PANEL_H_


#include <MessageFilter.h>
#include <String.h>
#include <Window.h>


class BArchivable;
class BGroupView;
class BHandler;
class BMessage;
class BView;


namespace BPrivate {
	namespace Print {


class BPrintPanel : public BWindow {
public:
							BPrintPanel(const BString& title);
	virtual					~BPrintPanel();

							BPrintPanel(BMessage* data);
	static	BArchivable*	Instantiate(BMessage* data);
	virtual	status_t		Archive(BMessage* data, bool deep = true) const;

	virtual	status_t		Go() = 0;

			BView*			Panel() const;
			void			AddPanel(BView* panel);
			bool			RemovePanel(BView* child);

	virtual	void			MessageReceived(BMessage* message);
	virtual	void			FrameResized(float newWidth, float newHeight);

	virtual BHandler*		ResolveSpecifier(BMessage* message, int32 index,
								BMessage* specifier, int32 form,
								const char* property);
	virtual	status_t		GetSupportedSuites(BMessage* data);
	virtual status_t		Perform(perform_code d, void* arg);

	virtual	void			Quit();
	virtual	bool			QuitRequested();
	virtual void			DispatchMessage(BMessage* message, BHandler* handler);

protected:
	virtual status_t		ShowPanel();

private:
			void			AddChild(BView* child, BView* before = NULL);
			bool			RemoveChild(BView* child);
			BView*			ChildAt(int32 index) const;

	class	_BPrintPanelFilter_ : public BMessageFilter {
	public:
							_BPrintPanelFilter_(BPrintPanel* panel);
			filter_result	Filter(BMessage* msg, BHandler** target);

	private:
			BPrintPanel*	fPrintPanel;
	};

private:
	virtual	void			_ReservedBPrintPanel1();
	virtual	void			_ReservedBPrintPanel2();
	virtual	void			_ReservedBPrintPanel3();
	virtual	void			_ReservedBPrintPanel4();
	virtual	void			_ReservedBPrintPanel5();

private:
			BGroupView*		fPanel;
			sem_id			fPrintPanelSem;
			status_t		fPrintPanelResult;

			uint32			_reserved[5];
};


	}	// namespace Print
}	// namespace BPrivate


#endif	// _PRINT_PANEL_H_
