/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef BREAK_CONDITION_CONFIG_WINDOW_H
#define BREAK_CONDITION_CONFIG_WINDOW_H


#include <Window.h>

#include "types/Types.h"


class BButton;
class BCheckBox;
class ImageDebugInfo;
class Team;
class UserInterfaceListener;


class BreakConditionConfigWindow : public BWindow {
public:
								BreakConditionConfigWindow(::Team* team,
									UserInterfaceListener* listener,
									BHandler* target);

								~BreakConditionConfigWindow();

	static	BreakConditionConfigWindow* Create(::Team* team,
									UserInterfaceListener* listener,
									BHandler* target);
									// throws

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();

private:
			void	 			_Init();
			void				_UpdateThrownBreakpoints(bool enable);
			status_t			_FindExceptionFunction(ImageDebugInfo* info,
									target_addr_t& _foundAddress) const;


private:
			::Team*				fTeam;
			UserInterfaceListener* fListener;
			BCheckBox*			fExceptionThrown;
			BCheckBox*			fExceptionCaught;
			BButton*			fCloseButton;
			BHandler*			fTarget;
};


#endif // BREAK_CONDITION_CONFIG_WINDOW_H
