#ifndef T_FIND_PANEL_H
#define T_FIND_PANEL_H


#include <View.h>


class BButton;
class BLooper;
class BEntry;
class BFile;
class BLocker;
class BMenu;
class BMenuField;
class BMenuItem;
class BMessage;
class BMessageRunner;
class BPopUpMenu;
class BQuery;

namespace BPrivate {

class BColumn;
class BQueryContainerWindow;
class BQueryPoseView;
class ShortMimeInfo;


const uint32 kTVolumeItem = 'Fvol';

const uint32 kAddColumn = 'Facl';
const uint32 kRemoveColumn = 'Frcl';
const uint32 kMoveColumn = 'Fmcl';

const uint32 kUpdatePoseView = 'Fupv';


class TFindPanel : public BView {
public:
								TFindPanel(BQueryContainerWindow*, BQueryPoseView*, BLooper*,
									entry_ref* ref = NULL);
								~TFindPanel();

protected:
	virtual	void				MessageReceived(BMessage*);
	virtual	void				AttachedToWindow();

private:
			// Private Member functions for populating Menus (Ported from FindPanel.cpp)
			BMenuItem* 			CurrentMimeType(const char** type = NULL) const;
			status_t			SetCurrentMimeType(BMenuItem* item);
			status_t 			SetCurrentMimeType(const char* label);
			void				AddMimeTypesToMenu(BPopUpMenu*);
	static 	bool 				AddOneMimeTypeToMenu(const ShortMimeInfo*, void* castToMenu);
			void				PushMimeType(BQuery*) const;
			void				AddVolumes(BMenu*);
			void 				ShowVolumeMenuLabel();
			BPopUpMenu* 		MimeTypeMenu() const { return fMimeTypeMenu; }

	static	void				ResizeMenuField(BMenuField*);

			void				HandleResizingColumnsContainer();
			void				HandleMovingAColumn();
			status_t			AddAttributeColumn(BColumn*);

			float				FindMaximumHeightOfColumns();

			status_t			GetPredicateString(BString*) const;

			status_t			GetDefaultFileEntryAndReference();
			
			void				SaveQueryAsAttributesToFile();
			void				WritePredicateToFile(const BString*);
			void				WriteVolumesToFile();
			void				WriteDirectoriesToFile();
			void				WriteColumnsToFile();
			void				RestoreColumnsFromFile();

			void				SendPoseViewUpdateMessage();


			void				DebouncingCheck();

private:
			BPopUpMenu*			fMimeTypeMenu;
			BMenuField*			fMimeTypeField;
			BPopUpMenu*			fVolMenu;
			BButton*			fPauseButton;

			BView*				fColumnsContainer;

			BQueryContainerWindow*	fContainerWindow;
			BQueryPoseView*			fPoseView;
			BLooper*				fLooper;

			BEntry*				fEntry;
			BFile*				fFile;

			typedef BView __inherited;
};


}

#endif
