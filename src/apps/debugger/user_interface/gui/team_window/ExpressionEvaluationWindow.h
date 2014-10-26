/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXPRESSION_EVALUATION_WINDOW_H
#define EXPRESSION_EVALUATION_WINDOW_H


#include <Window.h>

#include "types/Types.h"


class BButton;
class BStringView;
class BTextControl;
class SourceLanguage;
class UserInterfaceListener;


class ExpressionEvaluationWindow : public BWindow
{
public:
								ExpressionEvaluationWindow(
									SourceLanguage* language,
									UserInterfaceListener* listener);

								~ExpressionEvaluationWindow();

	static	ExpressionEvaluationWindow* Create(
									SourceLanguage* language,
									UserInterfaceListener* listener);
									// throws


	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();

private:
			void	 			_Init();


private:
			SourceLanguage*		fLanguage;
			BTextControl*		fExpressionInput;
			BStringView*		fExpressionOutput;
			BButton*			fEvaluateButton;
			UserInterfaceListener* fListener;
};

#endif // EXPRESSION_EVALUATION_WINDOW_H
