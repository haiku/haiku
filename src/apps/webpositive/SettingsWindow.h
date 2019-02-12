/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include <Window.h>


class BButton;
class BCheckBox;
class BMenu;
class BMenuField;
class BMenuItem;
class BSpinner;
class BTextControl;
class FontSelectionView;
class SettingsMessage;
class BFilePanel;


class SettingsWindow : public BWindow {
public:
								SettingsWindow(BRect frame,
									SettingsMessage* settings);
	virtual						~SettingsWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	virtual	void				Show();

private:
			BView*				_CreateGeneralPage(float spacing);
			BView*				_CreateFontsPage(float spacing);
			BView*				_CreateProxyPage(float spacing);
			void				_SetupFontSelectionView(
									FontSelectionView* view,
									BMessage* message);

			bool				_CanApplySettings() const;
			void				_ApplySettings();
			void				_RevertSettings();
			void 				_ChooseDownloadFolder(const BMessage* message);

			void				_HandleDownloadPanelResult(BFilePanel* panel,
									const BMessage* message);
			void				_ValidateControlsEnabledStatus();

			uint32				_StartUpPolicy() const;
			uint32				_NewWindowPolicy() const;
			uint32				_NewTabPolicy() const;

			BFont				_FindDefaultSerifFont() const;

			uint32				_ProxyPort() const;

private:
			SettingsMessage*	fSettings;

			BTextControl*		fStartPageControl;
			BTextControl*		fSearchPageControl;
			BTextControl*		fDownloadFolderControl;

			BMenuField*			fNewWindowBehaviorMenu;
			BMenuItem*			fNewWindowBehaviorOpenHomeItem;
			BMenuItem*			fNewWindowBehaviorOpenSearchItem;
			BMenuItem*			fNewWindowBehaviorOpenBlankItem;

			BMenuField*			fNewTabBehaviorMenu;
			BMenuItem*			fNewTabBehaviorCloneCurrentItem;
			BMenuItem*			fNewTabBehaviorOpenHomeItem;
			BMenuItem*			fNewTabBehaviorOpenSearchItem;
			BMenuItem*			fNewTabBehaviorOpenBlankItem;

			BMenuField*			fStartUpBehaviorMenu;
			BMenuItem*			fStartUpBehaviorResumePriorSession;
			BMenuItem*			fStartUpBehaviorStartNewSession;

			BSpinner*			fDaysInHistory;
			BCheckBox*			fShowTabsIfOnlyOnePage;
			BCheckBox*			fAutoHideInterfaceInFullscreenMode;
			BCheckBox*			fAutoHidePointer;
			BCheckBox*			fShowHomeButton;

			FontSelectionView*	fStandardFontView;
			FontSelectionView*	fSerifFontView;
			FontSelectionView*	fSansSerifFontView;
			FontSelectionView*	fFixedFontView;

			BCheckBox*			fUseProxyCheckBox;
			BTextControl*		fProxyAddressControl;
			BTextControl*		fProxyPortControl;
			BCheckBox*			fUseProxyAuthCheckBox;
			BTextControl*		fProxyUsernameControl;
			BTextControl*		fProxyPasswordControl;

			BButton*			fApplyButton;
			BButton*			fCancelButton;
			BButton*			fRevertButton;
			BButton*			fChooseButton;

			BSpinner*			fStandardSizesSpinner;
			BSpinner*			fFixedSizesSpinner;

			BFilePanel*			fOpenFilePanel;
};


#endif // SETTINGS_WINDOW_H

