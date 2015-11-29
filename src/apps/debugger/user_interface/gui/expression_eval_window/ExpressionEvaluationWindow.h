/*
 * Copyright 2014-2016, Rene Gollent, rene@gollent.com.
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

	// Team::Listener
	virtual	void				ThreadAdded(const Team::ThreadEvent& event);
	virtual	void				ThreadRemoved(const Team::ThreadEvent& event);

	virtual	void				ThreadStateChanged(
									const Team::ThreadEvent& event);
	virtual	void				ThreadStackTraceChanged(
									const Team::ThreadEvent& event);

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

			void				_HandleThreadAdded(int32 threadID);
			void				_HandleThreadRemoved(int32 threadID);
			void				_HandleThreadStateChanged(int32 threadID);
			void				_HandleThreadStackTraceChanged(int32 threadID);

			void				_UpdateThreadList();
			void				_UpdateFrameList();

			status_t			_CreateThreadMenuItem(::Thread* thread,
									BMenuItem*& _item) const;

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
