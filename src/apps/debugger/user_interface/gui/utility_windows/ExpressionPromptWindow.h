/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXPRESSION_PROMPT_WINDOW_H
#define EXPRESSION_PROMPT_WINDOW_H


#include <Window.h>


class BButton;
class BTextControl;


class ExpressionPromptWindow : public BWindow
{
public:
								ExpressionPromptWindow(BHandler* addTarget,
									BHandler* closeTarget);

								~ExpressionPromptWindow();

	static	ExpressionPromptWindow* Create(BHandler* addTarget,
									BHandler* closeTarget);
									// throws


	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();
	virtual	bool				QuitRequested();

private:
			void	 			_Init();

private:
			BTextControl*		fExpressionInput;
			BButton*			fCancelButton;
			BButton*			fAddButton;
			BHandler*			fAddTarget;
			BHandler*			fCloseTarget;
};

#endif // EXPRESSION_PROMPT_WINDOW_H
