/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXPRESSION_EVALUATION_WINDOW_H
#define EXPRESSION_EVALUATION_WINDOW_H


#include <Window.h>

#include "Team.h"
#include "types/Types.h"

class BMenu;
class BButton;
class BStringView;
class BTextControl;
class Team;
class SourceLanguage;
class UserInterfaceListener;


class ExpressionEvaluationWindow : public BWindow, private Team::Listener
{
public:
								ExpressionEvaluationWindow(
									::Team* team,
									SourceLanguage* language,
									UserInterfaceListener* listener,
									BHandler* target);

								~ExpressionEvaluationWindow();

	static	ExpressionEvaluationWindow* Create(
									::Team* team,
									SourceLanguage* language,
									UserInterfaceListener* listener,
									BHandler* target);
									// throws


	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();
	virtual	bool				QuitRequested();

private:
			void	 			_Init();
			BMenu*				_BuildTypesMenu();

	// Team::Listener
	virtual	void				ExpressionEvaluated(
									const Team::ExpressionEvaluationEvent&
										event);

private:
			::Team*				fTeam;
			SourceLanguage*		fLanguage;
			BTextControl*		fExpressionInput;
			BStringView*		fExpressionOutput;
			BButton*			fEvaluateButton;
			UserInterfaceListener* fListener;
			BHandler*			fCloseTarget;
			type_code			fCurrentEvaluationType;
};

#endif // EXPRESSION_EVALUATION_WINDOW_H
