class BCheckBox;

class ServerPropertiesView : public BView
{
	public:
		ServerPropertiesView(BRect rect, const char *name);
		~ServerPropertiesView();

		void Draw(BRect rect);
		void EnableControls();

		bool GetRecordingLogins()				{ return recordLogins; }

	private:
		BBitmap *icon;
		BCheckBox *chkRecordLogins;
		BButton *btnViewLogins;

		char server[B_FILE_NAME_LENGTH];
		bool recordLogins;
};

class ServerPropertiesPanel : public BWindow
{
	public:
		ServerPropertiesPanel(BRect frame, const char *name, BWindow *parent);

		void MessageReceived(BMessage *msg);

		bool isCancelled()			{ return cancelled; }

	private:
		ServerPropertiesView *infoView;
		BWindow *shareWin;
		char server[B_FILE_NAME_LENGTH];
		bool cancelled;
};
