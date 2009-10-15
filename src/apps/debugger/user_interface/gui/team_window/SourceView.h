/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_VIEW_H
#define SOURCE_VIEW_H


#include <Font.h>
#include <View.h>

#include "Types.h"


class Breakpoint;
class SourceCode;
class StackFrame;
class StackTrace;
class Statement;
class Team;
class UserBreakpoint;


class SourceView : public BView {
public:
	class Listener;

public:
								SourceView(Team* team, Listener* listener);
								~SourceView();

	static	SourceView*			Create(Team* team, Listener* listener);
									// throws

			void				UnsetListener();

			void				SetStackTrace(StackTrace* stackTrace);
			void				SetStackFrame(StackFrame* stackFrame);
			void				SetSourceCode(SourceCode* sourceCode);

			void				UserBreakpointChanged(
									UserBreakpoint* breakpoint);

			bool				ScrollToAddress(target_addr_t address);
			bool				ScrollToLine(uint32 line);

			void				HighlightBorder(bool state);
	virtual	void				TargetedByScrollView(BScrollView* scrollView);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	void				DoLayout();

private:
			class BaseView;
			class MarkerManager;
			class MarkerView;
			class TextView;

			struct FontInfo {
				BFont		font;
				font_height	fontHeight;
				float		lineHeight;
			};

private:
			void				_Init();
			void				_UpdateScrollBars();
			BSize				_DataRectSize() const;

private:
			Team*				fTeam;
			StackTrace*			fStackTrace;
			StackFrame*			fStackFrame;
			SourceCode*			fSourceCode;
			MarkerManager*		fMarkerManager;
			MarkerView*			fMarkerView;
			TextView*			fTextView;
			FontInfo			fFontInfo;
			Listener*			fListener;
};


class SourceView::Listener {
public:
	virtual						~Listener();

	virtual	void				SetBreakpointRequested(
									target_addr_t address, bool enabled) = 0;
	virtual	void				ClearBreakpointRequested(
									target_addr_t address) = 0;
};


#endif	// SOURCE_VIEW_H
