/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_VIEW_H
#define SOURCE_VIEW_H

#include <Font.h>
#include <View.h>


class SourceCode;
class StackFrame;
class TeamDebugModel;


class SourceView : public BView {
public:
	class Listener;

public:
								SourceView(TeamDebugModel* debugModel,
									Listener* listener);
								~SourceView();

	static	SourceView*			Create(TeamDebugModel* debugModel,
									Listener* listener);
									// throws

			void				UnsetListener();

			void				SetStackFrame(StackFrame* frame);
			void				UpdateSourceCode();

	virtual	void				TargetedByScrollView(BScrollView* scrollView);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	void				DoLayout();

private:
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
			TeamDebugModel*		fDebugModel;
			StackFrame*			fStackFrame;
			SourceCode*			fSourceCode;
			MarkerView*			fMarkerView;
			TextView*			fTextView;
			FontInfo			fFontInfo;
			Listener*			fListener;
};


class SourceView::Listener {
public:
	virtual						~Listener();

//	virtual	void				StackFrameSelectionChanged(
//									StackFrame* frame) = 0;
};


#endif	// SOURCE_VIEW_H
