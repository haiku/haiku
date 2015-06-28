/*
 * Copyright 2013-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_STOP_CONFIG_VIEW_H
#define IMAGE_STOP_CONFIG_VIEW_H


#include <GroupView.h>

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


class ImageStopConfigView : public BGroupView, private Team::Listener {
public:
								ImageStopConfigView(::Team* team,
									UserInterfaceListener* listener);

								~ImageStopConfigView();

	static	ImageStopConfigView* Create(::Team* team,
									UserInterfaceListener* listener);
									// throws

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

	// Team::Listener
	virtual	void				StopOnImageLoadSettingsChanged(
									const Team::ImageLoadEvent& event);
	virtual	void				StopOnImageLoadNameAdded(
									const Team::ImageLoadNameEvent& event);
	virtual	void				StopOnImageLoadNameRemoved(
									const Team::ImageLoadNameEvent& event);


private:
			void	 			_Init();

			void				_UpdateStopImageState();
									// must be called with team lock held

private:
			::Team*				fTeam;
			UserInterfaceListener* fListener;
			BCheckBox*			fStopOnImageLoad;
			BMenuField*			fStopImageConstraints;
			BListView*			fStopImageNames;
			BTextControl*		fStopImageNameInput;
			BButton*			fAddImageNameButton;
			BButton*			fRemoveImageNameButton;
			BView*				fCustomImageGroup;
			bool				fStopOnLoadEnabled;
			bool				fUseCustomImages;
};


#endif // IMAGE_STOP_CONFIG_VIEW_H
