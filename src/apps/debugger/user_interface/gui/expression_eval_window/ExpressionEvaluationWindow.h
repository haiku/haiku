/*
 * Copyright 2014-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXPRESSION_EVALUATION_WINDOW_H
#define EXPRESSION_EVALUATION_WINDOW_H


#include <Window.h>

#include "Team.h"
#include "VariablesView.h"


class BButton;
class BMenuField;
class BTextControl;
class StackFrame;
class Team;
class UserInterfaceListener;
class SourceLanguage;

class ExpressionEvaluationWindow : public BWindow, private Team::Listener,
	private VariablesView::Listener {
public:
								ExpressionEvaluationWindow(
									BHandler* fCloseTarget,
									::Team* team,
									UserInterfaceListener* listener);

								~ExpressionEvaluationWindow();

	static	ExpressionEvaluationWindow* Create(BHandler* fCloseTarget,
									::Team* team,
									UserInterfaceListener* listener);
									// throws


	virtual	void				Show();
	virtual	bool				QuitRequested();

	virtual	void				MessageReceived(BMessage* message);

	// VariablesView::Listener
	virtual	void				ValueNodeValueRequested(CpuState* cpuState,
									ValueNodeContainer* container,
									ValueNode* valueNode);

	virtual	void				ExpressionEvaluationRequested(
									ExpressionInfo* info,
									StackFrame* frame,
									::Thread* thread);

	virtual	void				ValueNodeWriteRequested(
									ValueNode* node,
									CpuState* state,
									Value* newValue);

private:
			void	 			_Init();

			void				_HandleThreadSelectionChanged(int32 threadID);
			void				_HandleFrameSelectionChanged(int32 index);

			void				_UpdateThreadList();
			void				_UpdateFrameList();

private:
			BTextControl*		fExpressionInput;
			BMenuField*			fThreadList;
			BMenuField*			fFrameList;
			VariablesView*		fVariablesView;
			BButton*			fCloseButton;
			BButton*			fEvaluateButton;
			BHandler*			fCloseTarget;
			SourceLanguage*		fCurrentLanguage;
			SourceLanguage*		fFallbackLanguage;
			::Team*				fTeam;
			::Thread*			fSelectedThread;
			StackFrame*			fSelectedFrame;
			UserInterfaceListener* fListener;
};

#endif // EXPRESSION_EVALUATION_WINDOW_H
