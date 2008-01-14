#include <View.h>

namespace Z {
namespace ThemeManager {
	class ThemeManager;
} // ns ThemeManager
} // ns Z
using Z::ThemeManager::ThemeManager;

class BBitmap;
class BSeparator;
class BBox;
class BListView;
class BButton;
class BScrollView;
class BTextView;
class BMessage;
class BStringView;
class MyInvoker;

class ThemeInterfaceView : public BView
{
	public:
							ThemeInterfaceView(BRect _bounds);
		virtual 			~ThemeInterfaceView();
		
		virtual void 		AllAttached();
		virtual void 		MessageReceived(BMessage* _msg);
		ThemeManager*		GetThemeManager();
		
		void				HideScreenshotPane(bool hide);
		bool				IsScreenshotPaneHidden();

		void				PopulateThemeList();
		void				PopulateAddonList();

		status_t			Revert();
		status_t			ApplyDefaults();
		status_t			ApplySelected();
		status_t			CreateNew(const char *name);
		status_t			SaveSelected();
		status_t			DeleteSelected();
		status_t			AddScreenshot();
		
		status_t			ThemeSelected();
		
		void				SetIsRevertable();
		void				SetScreenshot(BBitmap *shot);
		status_t			AError(const char *func, status_t err);

	private:
		static int32		_ThemeListPopulatorTh(void *arg);
		void				_ThemeListPopulator();
		
		ThemeManager*		fThemeManager;
		bool				fScreenshotPaneHidden;
		bool				fHasScreenshot;
		
		MyInvoker*			fPopupInvoker;
		BScrollView*		fThemeListSV;
		BListView*			fThemeList;
		BButton*			fApplyBtn;
		BButton*			fNewBtn;
		BButton*			fSaveBtn;
		BButton*			fDeleteBtn;
		BButton*			fSetShotBtn;
		BButton*			fShowSSPaneBtn;
		BView*				fScreenshotPane;
		BStringView*		fScreenshotNone;
		BBox*				fBox;
		BScrollView*		fAddonListSV;
		BListView*			fAddonList;
};

extern "C" BView *themes_pref(const BRect& Bounds);

#define SSPANE_WIDTH 320
#define SSPANE_HEIGHT 240


