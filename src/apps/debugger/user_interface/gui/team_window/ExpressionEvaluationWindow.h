/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXPRESSION_EVALUATION_WINDOW_H
#define EXPRESSION_EVALUATION_WINDOW_H


#include <Window.h>

#include "ExpressionInfo.h"
#include "types/Types.h"


class BMenu;
class BMenuItem;
class BButton;
class BStringView;
class BTextControl;
class Thread;
class PrimitiveType;
class SourceLanguage;
class StackFrame;
class UserInterfaceListener;


class ExpressionEvaluationWindow : public BWindow,
	private ExpressionInfo::Listener
{
public:
								ExpressionEvaluationWindow(
									SourceLanguage* language,
									StackFrame* frame,
									::Thread* thread,
									UserInterfaceListener* listener,
									BHandler* target);

								~ExpressionEvaluationWindow();

	static	ExpressionEvaluationWindow* Create(
									SourceLanguage* language,
									StackFrame* frame,
									::Thread* thread,
									UserInterfaceListener* listener,
									BHandler* target);
									// throws


	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();
	virtual	bool				QuitRequested();

private:
			void	 			_Init();
			BMenu*				_BuildTypesMenu();
			BMenuItem*			_AddMenuItemForType(BMenu* menu,
									type_code type);

	// ExpressionInfo::Listener
	virtual	void				ExpressionEvaluated(ExpressionInfo* info,
									status_t result, Value* value);

private:
			SourceLanguage*		fLanguage;
			ExpressionInfo*		fExpressionInfo;
			BTextControl*		fExpressionInput;
			BStringView*		fExpressionOutput;
			BButton*			fEvaluateButton;
			UserInterfaceListener* fListener;
			StackFrame*			fStackFrame;
			::Thread*			fThread;
			BHandler*			fCloseTarget;
};

#endif // EXPRESSION_EVALUATION_WINDOW_H
