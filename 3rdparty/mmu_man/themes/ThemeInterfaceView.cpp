#include <Alert.h>
#include <Bitmap.h>
#include <Box.h>
#include <DataIO.h>
#include <Entry.h>
#include <File.h>
#include <BeBuild.h>
#ifdef B_ZETA_VERSION
#include <add-ons/pref_app/PrefPanel.h>
#include <locale/Locale.h>
#include <Separator.h>
#else
#define _T(v) v
#define B_PREF_APP_ENABLE_REVERT 'zPAE'
#define B_PREF_APP_SET_DEFAULTS 'zPAD'
#define B_PREF_APP_REVERT 'zPAR'
#define B_PREF_APP_ADDON_REF 'zPAA'
#endif
#include <Resources.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <Roster.h>

#include <Button.h>
#include <ListView.h>
#include <interface/StringView.h>
#include <ScrollView.h>
#include <TextView.h>

#include <Application.h>
#include <MessageFilter.h>

#include <stdio.h>

#include "UITheme.h"
#include "TextInputAlert.h"

extern status_t ScaleBitmap(const BBitmap& inBitmap, BBitmap& outBitmap);

/* gui */
//#include "ThemeSelector.h"
#include "ThemeInterfaceView.h"
#include "ThemeItem.h"
#include "ThemeAddonItem.h"

/* impl */
#include "ThemeManager.h"

const uint32 kThemeChanged		= 'mThC';
const uint32 kApplyThemeBtn		= 'TmAp';
const uint32 kCreateThemeBtn	= 'TmCr';
const uint32 kReallyCreateTheme	= 'TmCR';
const uint32 kSaveThemeBtn		= 'TmSa';
const uint32 kDeleteThemeBtn	= 'TmDe';
const uint32 kHidePreviewBtn	= 'TmHP';
const uint32 kThemeSelected		= 'TmTS';
const uint32 kMakeScreenshot	= 'TmMS';

const uint32 kHideSSPulse		= 'TmH1';
const uint32 kShowSSPulse		= 'TmH2';

static const uint32 skOnlineThemes	= 'TmOL';
static char* skThemeURL			= "http://www.zeta-os.com/cms/download.php?list.4";

#define HIDESS_OFFSET (Bounds().Width()/2 - 130)
// ------------------------------------------------------------------------------
filter_result refs_filter(BMessage *message, BHandler **handler, BMessageFilter *filter)
{
	(void)handler;
	(void)filter;
	switch (message->what) {
	case B_REFS_RECEIVED:
		message->PrintToStream();
		break;
	}
	return B_DISPATCH_MESSAGE;
}

// ------------------------------------------------------------------------------
//extern "C" BView *get_pref_view(const BRect& Bounds)
extern "C" BView *themes_pref(const BRect& Bounds)
{
	return new ThemeInterfaceView(Bounds);
}

// ------------------------------------------------------------------------------
ThemeInterfaceView::ThemeInterfaceView(BRect _bounds)
	: BView(_bounds, "Themes", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
	, fThemeManager(NULL)
	, fScreenshotPaneHidden(false)
	, fHasScreenshot(false)
	, fPopupInvoker(NULL)
	, fBox(NULL)
{
/*
	BMessageFilter *filt = new BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, refs_filter);
	be_app->Lock();
	be_app->AddCommonFilter(filt);
	be_app->Unlock();
*/
}

// ------------------------------------------------------------------------------
ThemeInterfaceView::~ThemeInterfaceView()
{
	delete fPopupInvoker;
	delete fThemeManager;
}

// ------------------------------------------------------------------------------
void
ThemeInterfaceView::AllAttached()
{
	BView::AllAttached();
	
	fPopupInvoker = new BInvoker(new BMessage(kReallyCreateTheme), this);
#ifdef B_BEOS_VERSION_DANO
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
#else
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
#endif
	
	fThemeManager = new ThemeManager;
	fThemeManager->LoadThemes();

	BRect frame = Bounds();
	frame.InsetBy(10.0, 10.0);
	
	// add the theme listview
	BRect list_frame = frame;
	list_frame.right = 130;
	fThemeList = new BListView(list_frame.InsetByCopy(3.0, 3.0), "themelist");
	fThemeListSV = new BScrollView("themelistsv", fThemeList, B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true);
	AddChild(fThemeListSV);
	fThemeList->SetSelectionMessage(new BMessage(kThemeSelected));
	fThemeList->SetInvocationMessage(new BMessage(kApplyThemeBtn));
	fThemeList->SetTarget(this);

	// buttons...
	fNewBtn = new BButton(BRect(), "create", _T("New"), new BMessage(kCreateThemeBtn));
	AddChild(fNewBtn);
	fNewBtn->SetTarget(this);
	fNewBtn->ResizeToPreferred();
	fNewBtn->MoveTo(fThemeListSV->Frame().right + 15.0, frame.bottom - fNewBtn->Bounds().Height());
	BPoint lt = fNewBtn->Frame().LeftTop();
	lt.x = fNewBtn->Frame().right + 10.0;

	lt.x = fNewBtn->Frame().right + 10.0;
	fSaveBtn = new BButton(BRect(), "save", _T("Save"), new BMessage(kSaveThemeBtn));
	AddChild(fSaveBtn);
	fSaveBtn->SetTarget(this);
	fSaveBtn->ResizeToPreferred();
	fSaveBtn->MoveTo(lt);

	lt.x = fSaveBtn->Frame().right + 10.0;
	fDeleteBtn = new BButton(BRect(), "delete", _T("Delete"), new BMessage(kDeleteThemeBtn));
	AddChild(fDeleteBtn);
	fDeleteBtn->SetTarget(this);
	fDeleteBtn->ResizeToPreferred();
	fDeleteBtn->MoveTo(lt);

	// buttons...
	fSetShotBtn = new BButton(BRect(), "makeshot", _T("Add Screenshot"), new BMessage(kMakeScreenshot), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	AddChild(fSetShotBtn);
	fSetShotBtn->SetTarget(this);
	fSetShotBtn->ResizeToPreferred();
	
	fShowSSPaneBtn = new BButton(BRect(), "hidess", _T("Show Options"), new BMessage(kHidePreviewBtn), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	AddChild(fShowSSPaneBtn);
	fShowSSPaneBtn->SetTarget(this);
	fShowSSPaneBtn->ResizeToPreferred();

	fApplyBtn = new BButton(BRect(), "apply", _T("Apply"), new BMessage(kApplyThemeBtn), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	AddChild(fApplyBtn);
	fApplyBtn->ResizeToPreferred();
	fApplyBtn->SetTarget(this);

	float widest = max_c(fSetShotBtn->Bounds().Width(), fShowSSPaneBtn->Bounds().Width());
	widest = max_c(widest, fApplyBtn->Bounds().Width());
	float height = fSetShotBtn->Bounds().Height();
	fSetShotBtn->ResizeTo(widest, height);
	fShowSSPaneBtn->ResizeTo(widest, height);
	fApplyBtn->ResizeTo(widest, height);
	
	fSetShotBtn->MoveTo(frame.right - widest, frame.top + 5.0);
	fShowSSPaneBtn->MoveTo(frame.right - widest, fSetShotBtn->Frame().bottom + 10.0);
	fApplyBtn->MoveTo(frame.right - widest, fNewBtn->Frame().top - fApplyBtn->Bounds().Height() - 10);
	
	// add the preview screen
	BRect preview_frame(fNewBtn->Frame().left, fThemeListSV->Frame().top, frame.right - widest - 10, fNewBtn->Frame().top - 10);

	fBox = new BBox(preview_frame, "preview", B_FOLLOW_ALL);
	AddChild(fBox);
	fBox->SetLabel(_T("Preview"));
	
	preview_frame.InsetBy(10.0, 15.0);
	preview_frame.OffsetTo(10.0, 20.0);
	fScreenshotPane = new BView(preview_frame, "screenshot", B_FOLLOW_ALL, B_WILL_DRAW);
	fBox->AddChild(fScreenshotPane);
	fScreenshotPane->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	
	fScreenshotNone = new BStringView(BRect(), "sshotnone", _T("No Theme selected"), (uint32) 0, B_FOLLOW_ALL);
	fScreenshotNone->SetFontSize(20.0);
	fScreenshotNone->SetAlignment(B_ALIGN_CENTER);
	fBox->AddChild(fScreenshotNone);
	fScreenshotNone->ResizeToPreferred();
	fScreenshotNone->ResizeTo(fBox->Bounds().Width() - 10.0, fScreenshotNone->Bounds().Height());
	fScreenshotNone->MoveTo(fBox->Bounds().left + 5.0,
							((fBox->Frame().Height() - fScreenshotNone->Frame().Height()) / 2.0));
							
	// Theme hyperlink
	/*
	BStringView* hlink = new BStringView(BRect(), "theme_hyperlink", _T("More themes online"), new BMessage(skOnlineThemes), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(hlink);
	hlink->SetClickText(hlink->GetText(), *this);
	hlink->ResizeToPreferred();
	hlink->MoveTo(frame.right - hlink->Bounds().Width(), fNewBtn->Frame().top + 5);
	*/
	
	// the addons list view
	preview_frame = fBox->Frame();
	preview_frame.InsetBy(3.0, 3.0);
	preview_frame.right -= B_V_SCROLL_BAR_WIDTH;
	fAddonList = new BListView(preview_frame, "addonlist");
	fAddonListSV = new BScrollView("addonlistsv", fAddonList, B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true);
	AddChild(fAddonListSV);
	fAddonList->SetSelectionMessage(new BMessage(kThemeSelected));
	fAddonList->SetInvocationMessage(new BMessage(kApplyThemeBtn));
	fAddonList->SetTarget(this);	
	fAddonListSV->Hide();

	PopulateThemeList();
	PopulateAddonList();
}

// ------------------------------------------------------------------------------
void
ThemeInterfaceView::MessageReceived(BMessage *_msg)
{
	ThemeManager *tman;
	int32 value;
	int32 id;
//_msg->PrintToStream();
	switch(_msg->what)
	{
		case B_REFS_RECEIVED:
			_msg->PrintToStream();
			break;

		case kMakeScreenshot:
			AddScreenshot();
			break;

		case kThemeSelected:
			ThemeSelected();
			break;

		case kHidePreviewBtn:
			HideScreenshotPane(!IsScreenshotPaneHidden());
			break;

		case kApplyThemeBtn:
			ApplySelected();
			break;

		case kCreateThemeBtn:
		{
			TextInputAlert *alert = new TextInputAlert("New name", "New Theme Name", "", "Ok", "Cancel");
			alert->Go(fPopupInvoker);
			break;
		}

		case kReallyCreateTheme:
		{
			const char *name;
			if (_msg->FindString("text", &name) < B_OK)
				break;
			CreateNew(name);
			break;
		}

		case kSaveThemeBtn:
			SaveSelected();
			break;

		case kDeleteThemeBtn:
			DeleteSelected();
			break;
#if 0
		case kColorsChanged:
		case kGeneralChanged:
		case kFontsChanged:
		{
			BMessenger msgr (Parent());
			msgr.SendMessage(B_PREF_APP_ENABLE_REVERT);
			BMessage changes;
			if (_msg->FindMessage("changes", &changes) == B_OK)
			{
				update_ui_settings(changes);
			}
			break;
		}
#endif
		case B_PREF_APP_SET_DEFAULTS:
		{
			ApplyDefaults();
			break;
		}
		
		case B_PREF_APP_REVERT:
		{
			Revert();
			break;
		}
		
		case B_PREF_APP_ADDON_REF:
		{
			break;
		}

		case kThemeChanged:
		{
/*			BMessage data;
			BMessage names;
			get_ui_settings(&data, &names);
			fColorSelector->Update(data);
			fFontSelector->Refresh();
			fGeneralSelector->Refresh(data);
*/
			break;
		}

		case CB_APPLY:
			tman = GetThemeManager();
			_msg->PrintToStream();
			if (_msg->FindInt32("be:value", &value) < B_OK)
				value = false;
			if (_msg->FindInt32("addon", &id) < B_OK)
				break;
			//tman->SetAddonFlags(id, (tman->AddonFlags(id) & ~Z_THEME_ADDON_DO_SET_ALL));
			tman->SetAddonFlags(id, (tman->AddonFlags(id) & ~Z_THEME_ADDON_DO_SET_ALL) | (value?Z_THEME_ADDON_DO_SET_ALL:0));
			break;

		case CB_SAVE:
			tman = GetThemeManager();
			_msg->PrintToStream();
			if (_msg->FindInt32("be:value", &value) < B_OK)
				value = false;
			if (_msg->FindInt32("addon", &id) < B_OK)
				break;
			tman->SetAddonFlags(id, (tman->AddonFlags(id) & ~Z_THEME_ADDON_DO_RETRIEVE) | (value?Z_THEME_ADDON_DO_RETRIEVE:0));
			break;

		case BTN_PREFS:
			tman = GetThemeManager();
			if (_msg->FindInt32("addon", &id) < B_OK)
				break;
			tman->RunPreferencesPanel(id);
			break;

		case kHideSSPulse:
			break;
			
		case kShowSSPulse:
			break;
		
		case skOnlineThemes:
			be_roster->Launch( "application/x-vnd.Mozilla-Firefox", 1, &skThemeURL);
			break;
			
		default:
		{
			BView::MessageReceived(_msg);
			break;
		}
	}
}

// ------------------------------------------------------------------------------
ThemeManager* ThemeInterfaceView::GetThemeManager()
{
	return fThemeManager;
}

// ------------------------------------------------------------------------------
void ThemeInterfaceView::HideScreenshotPane(bool hide)
{
	BString lbl;
	if (IsScreenshotPaneHidden() && hide)
		return;
	if (!IsScreenshotPaneHidden() && !hide)
		return;
	fScreenshotPaneHidden = hide;

	if (hide) 
	{
		if (!fScreenshotPane->IsHidden()) 
		{
			fBox->Hide();
			fAddonListSV->Show();
		}
		fShowSSPaneBtn->SetLabel(_T("Show Preview"));
	} 
	else 
	{
		fShowSSPaneBtn->SetLabel(_T("Show Options"));
		if (fScreenshotPane->IsHidden()) 
		{
			fBox->Show();
			fAddonListSV->Hide();
		}
	}
	// TODO
}


// ------------------------------------------------------------------------------
bool ThemeInterfaceView::IsScreenshotPaneHidden()
{
	return fScreenshotPaneHidden;
}

// ------------------------------------------------------------------------------
void ThemeInterfaceView::PopulateThemeList()
{
	status_t err;
	int32 i, count;
	BString name;
	ThemeItem *ti;
	bool isro;
	ThemeManager* tman = GetThemeManager();
	count = tman->CountThemes();
	fThemeList->MakeEmpty();
	for (i = 0; i < count; i++) {
		err = tman->ThemeName(i, name);
		isro = tman->ThemeIsReadOnly(i);
		if (err)
			continue;
		ti = new ThemeItem(i, name.String(), isro);
		fThemeList->AddItem(ti);
	}
}

// ------------------------------------------------------------------------------
void ThemeInterfaceView::PopulateAddonList()
{
	int32 i, count;
	ViewItem *vi;
	ThemeManager* tman = GetThemeManager();
	count = tman->CountAddons();
	fAddonList->MakeEmpty();
	for (i = 0; i < count; i++) {
		vi = new ThemeAddonItem(BRect(0,0,200,52), this, i);
		fAddonList->AddItem(vi);
	}
}

// ------------------------------------------------------------------------------
status_t ThemeInterfaceView::Revert()
{
	status_t err = B_OK;
	ThemeManager* tman = GetThemeManager();
	
	if (tman->CanRevert())
		err = tman->RestoreCurrent();
	if (err)
		return err;
	
	return B_OK;
}

// ------------------------------------------------------------------------------
status_t ThemeInterfaceView::ApplyDefaults()
{
	status_t err = B_OK;
	ThemeManager* tman = GetThemeManager();
	
	SetIsRevertable();

	err = tman->ApplyDefaultTheme();
	return err;
}

// ------------------------------------------------------------------------------
status_t ThemeInterfaceView::ApplySelected()
{
	status_t err;
	ThemeManager* tman = GetThemeManager();
	int32 id;
	ThemeItem *item;
	// find selected theme
	id = fThemeList->CurrentSelection(0);
	if (id < 0)
		return ENOENT;
	item = dynamic_cast<ThemeItem *>(fThemeList->ItemAt(id));
	if (!item)
		return ENOENT;
	id = item->ThemeId();
	if (id < 0)
		return ENOENT;
	SetIsRevertable();
	err = tman->ApplyTheme(id);
	return err;
}

// ------------------------------------------------------------------------------
status_t ThemeInterfaceView::CreateNew(const char *name)
{
	status_t err;
	ThemeManager* tman = GetThemeManager();
	int32 id;
	ThemeItem *ti;
	BString n(name);
	
	id = tman->MakeTheme();
	if (id < 0)
		return B_ERROR;
	err = tman->SetThemeName(id, name);
	if (err)
		return err;
	err = tman->ThemeName(id, n);
	if (err)
		return err;
	err = tman->SaveTheme(id, true);
	if (err)
		return err;
	ti = new ThemeItem(id, n.String(), false);
	fThemeList->AddItem(ti);
	return B_OK;
}

// ------------------------------------------------------------------------------
status_t ThemeInterfaceView::SaveSelected()
{
	status_t err;
	ThemeManager* tman = GetThemeManager();
	int32 id;
	ThemeItem *item;
	BMessage theme;
	// find selected theme
	id = fThemeList->CurrentSelection(0);
	if (id < 0)
		return B_OK;
	item = dynamic_cast<ThemeItem *>(fThemeList->ItemAt(id));
	if (!item)
		return AError(__FUNCTION__, ENOENT);
	if (item->IsReadOnly())
		return AError(__FUNCTION__, B_READ_ONLY_DEVICE);
	id = item->ThemeId();
	if (id < 0)
		return AError(__FUNCTION__, ENOENT);

	err = tman->UpdateTheme(id);
	if (err)
		return AError(__FUNCTION__, err);
	err = tman->SaveTheme(id);
	if (err)
		return AError(__FUNCTION__, err);
	//err = tman->ApplyTheme(theme);
	return err;
}

// ------------------------------------------------------------------------------
status_t ThemeInterfaceView::DeleteSelected()
{
	status_t err;
	ThemeManager* tman = GetThemeManager();
	int32 id;
	ThemeItem *item;
	BMessage theme;
	// find selected theme
	id = fThemeList->CurrentSelection(0);
	if (id < 0)
		return B_OK;
	item = dynamic_cast<ThemeItem *>(fThemeList->ItemAt(id));
	if (!item)
		return AError(__FUNCTION__, ENOENT);
	if (item->IsReadOnly())
		return AError(__FUNCTION__, B_READ_ONLY_DEVICE);
	id = item->ThemeId();
	if (id < 0)
		return AError(__FUNCTION__, ENOENT);
	// then apply
	err = tman->DeleteTheme(id);
	if (err)
		return AError(__FUNCTION__, err);
	fThemeList->RemoveItem(item);
	delete item;
	//err = tman->ApplyTheme(theme);
	return err;
}

// ------------------------------------------------------------------------------
status_t ThemeInterfaceView::AddScreenshot()
{
	status_t err;
	ThemeManager* tman = GetThemeManager();
	int32 id;
	ThemeItem *item;
	BMessage theme;
	// find selected theme
	id = fThemeList->CurrentSelection(0);
	if (id < 0)
		return B_OK;
	item = dynamic_cast<ThemeItem *>(fThemeList->ItemAt(id));
	if (!item)
		return AError(__FUNCTION__, ENOENT);
	id = item->ThemeId();
	if (id < 0)
		return AError(__FUNCTION__, ENOENT);
	// then apply
	err = tman->MakeThemeScreenShot(id);
	if (err)
		return AError(__FUNCTION__, err);
	err = tman->SaveTheme(id);
	if (err)
		return AError(__FUNCTION__, err);
	ThemeSelected(); // force reload of description for selected theme.
	return err;
}

// ------------------------------------------------------------------------------
status_t ThemeInterfaceView::ThemeSelected()
{
	status_t err;
	ThemeManager* tman = GetThemeManager();
	int32 id;
	ThemeItem *item;
	BString desc;
	BBitmap *sshot = NULL;
	// find selected theme
	id = fThemeList->CurrentSelection(0);
	if (id < 0)
	{
		fScreenshotPane->ClearViewBitmap();
		fScreenshotPane->Invalidate(fScreenshotPane->Bounds());
		
		HideScreenshotPane(false);			
		while(true == fScreenshotNone->IsHidden())
			fScreenshotNone->Show();
			
		fScreenshotNone->SetText(_T("No Theme selected"));			
		return ENOENT;
	}
	
	item = dynamic_cast<ThemeItem *>(fThemeList->ItemAt(id));
	if (!item)
		return ENOENT;
	id = item->ThemeId();
	if (id < 0)
		return ENOENT;
	// then apply
	
	err = tman->ThemeScreenShot(id, &sshot);
	if (err)
		sshot = NULL;
	if (sshot == NULL) 
	{
		SetScreenshot(NULL);
		fprintf(stderr, "ThemeManager: no screenshot; error 0x%08lx\n", err);

		HideScreenshotPane(false);
		while(true == fScreenshotNone->IsHidden())
			fScreenshotNone->Show();

		fScreenshotNone->SetText(_T("No Screenshot"));
		return err;
	}

	SetScreenshot(sshot);
	while(false == fScreenshotNone->IsHidden())
			fScreenshotNone->Hide();

	//err = tman->ApplyTheme(theme);
	return err;
}

// ------------------------------------------------------------------------------
void ThemeInterfaceView::SetIsRevertable()
{
	BMessenger msgr(Parent());
	msgr.SendMessage(B_PREF_APP_ENABLE_REVERT);
}

// PRIVATE: in libzeta for now.
extern status_t ScaleBitmap(const BBitmap& inBitmap, BBitmap& outBitmap);

// ------------------------------------------------------------------------------
void ThemeInterfaceView::SetScreenshot(BBitmap *shot)
{
	// no screenshotpanel
	if(NULL == fScreenshotPane)
		return;
		
	fScreenshotPane->ClearViewBitmap();
	if (shot)
	{
		BBitmap scaled(fScreenshotPane->Bounds(), B_RGB32);
		if( B_OK == ScaleBitmap(*shot, scaled) )
		{
			fScreenshotPane->SetViewBitmap(&scaled);
		}
		else
		{
			fScreenshotPane->SetViewBitmap(shot, shot->Bounds(), fScreenshotPane->Bounds());
		}
	}
	
	fScreenshotPane->Invalidate(fScreenshotPane->Bounds());
	fHasScreenshot = (shot != NULL);
	
	if (shot)
		delete shot;
}

// ------------------------------------------------------------------------------
status_t ThemeInterfaceView::AError(const char *func, status_t err)
{
	BAlert *alert;
	BString msg;
	char *str = strerror(err);
	msg << "Error in " << func << "() : " << str;
	alert = new BAlert("error", msg.String(), _T("Ok"));
	alert->Go();
	return err; /* pass thru */
}

