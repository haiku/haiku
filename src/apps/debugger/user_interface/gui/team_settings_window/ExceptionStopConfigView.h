/*
 * Copyright 2013-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXCEPTION_STOP_CONFIG_VIEW_H
#define EXCEPTION_STOP_CONFIG_VIEW_H


#include <GroupView.h>

#include "types/Types.h"


class BBox;
class BButton;
class BCheckBox;
class ImageDebugInfo;
class Team;
class UserInterfaceListener;


class ExceptionStopConfigView : public BGroupView {
public:
								ExceptionStopConfigView(::Team* team,
									UserInterfaceListener* listener);

								~ExceptionStopConfigView();

	static	ExceptionStopConfigView* Create(::Team* team,
									UserInterfaceListener* listener);
									// throws

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			void	 			_Init();
			void				_UpdateThrownBreakpoints(bool enable);
			status_t			_FindExceptionFunction(ImageDebugInfo* info,
									target_addr_t& _foundAddress) const;

			void				_UpdateExceptionState();

private:
			::Team*				fTeam;
			UserInterfaceListener* fListener;
			BCheckBox*			fExceptionThrown;
			BCheckBox*			fExceptionCaught;
};


#endif // EXCEPTION_STOP_CONFIG_VIEW_H
