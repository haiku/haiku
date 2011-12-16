/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_FUNCTIONS_VIEW_H
#define IMAGE_FUNCTIONS_VIEW_H

#include <GroupView.h>

#include "table/TreeTable.h"
#include "Team.h"


class FunctionInstance;


class ImageFunctionsView : public BGroupView, private TreeTableListener {
public:
	class Listener;

public:
								ImageFunctionsView(Listener* listener);
								~ImageFunctionsView();

	static	ImageFunctionsView*	Create(Listener* listener);
									// throws

			void				UnsetListener();

			void				SetImageDebugInfo(
									ImageDebugInfo* imageDebugInfo);
			void				SetFunction(FunctionInstance* function);

			void				LoadSettings(const BMessage& settings);
			status_t			SaveSettings(BMessage& settings);

private:
			class FunctionsTableModel;

private:
	// TreeTableListener
	virtual	void				TreeTableSelectionChanged(TreeTable* table);

			void				_Init();

private:
			ImageDebugInfo*		fImageDebugInfo;
			TreeTable*			fFunctionsTable;
			FunctionsTableModel* fFunctionsTableModel;
			Listener*			fListener;
};


class ImageFunctionsView::Listener {
public:
	virtual						~Listener();

	virtual	void				FunctionSelectionChanged(
									FunctionInstance* function) = 0;
};


#endif	// IMAGE_FUNCTIONS_VIEW_H
