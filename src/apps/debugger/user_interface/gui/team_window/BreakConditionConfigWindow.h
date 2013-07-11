/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef BREAK_CONDITION_CONFIG_WINDOW_H
#define BREAK_CONDITION_CONFIG_WINDOW_H


#include <Window.h>

#include "Team.h"

#include "types/Types.h"


class BBox;
class BButton;
class BCheckBox;
class BListView;
class BMenuField;
class BTextControl;
class ImageDebugInfo;
class UserInterfaceListener;


class BreakConditionConfigWindow : public BWindow, private Team::Listener {
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

	// Team::Listener
	virtual	void				StopOnImageLoadSettingsChanged(
									const Team::ImageLoadEvent& event);
	virtual	void				StopOnImageLoadNameAdded(
									const Team::ImageLoadNameEvent& event);
	virtual	void				StopOnImageLoadNameRemoved(
									const Team::ImageLoadNameEvent& event);


private:
			void	 			_Init();
			void				_UpdateThrownBreakpoints(bool enable);
			status_t			_FindExceptionFunction(ImageDebugInfo* info,
									target_addr_t& _foundAddress) const;

			void				_UpdateExceptionState();
			void				_UpdateStopImageState();
			void				_UpdateStopImageButtons(bool previousStop,
									bool previousCustomImages);
									// must be called with team lock held



private:
			::Team*				fTeam;
			UserInterfaceListener* fListener;
			BBox*				fExceptionSettingsBox;
			BBox*				fImageSettingsBox;
			BCheckBox*			fExceptionThrown;
			BCheckBox*			fExceptionCaught;
			BCheckBox*			fStopOnImageLoad;
			BMenuField*			fStopImageConstraints;
			BListView*			fStopImageNames;
			BTextControl*		fStopImageNameInput;
			BButton*			fAddImageNameButton;
			BButton*			fRemoveImageNameButton;
			BView*				fCustomImageGroup;
			bool				fStopOnLoadEnabled;
			bool				fUseCustomImages;
			BButton*			fCloseButton;
			BHandler*			fTarget;
};


#endif // BREAK_CONDITION_CONFIG_WINDOW_H
