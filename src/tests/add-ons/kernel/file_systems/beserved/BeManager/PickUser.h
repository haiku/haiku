// ----- PickUserItem ----------------------------------------------------------------------

class PickUserItem : public ListItem
{
	public:
		PickUserItem(BView *headerView, const char *text0, const char *text1);
		~PickUserItem();

		void ItemSelected();
		void ItemDeselected();
};


class PickUserView : public BView
{
	public:
		PickUserView(BRect rect, char *group);
		~PickUserView();

		void Draw(BRect rect);
		char *GetSelectedUser();
		void ItemSelected();
		void ItemDeselected();

	private:
		void AddUserList(ColumnListView* listView);

		SmartColumnListView *listView;
		BButton *okBtn;
		char excludedGroup[33];
};

class PickUserPanel : public BWindow
{
	public:
		PickUserPanel(BRect frame, char *group);

		void MessageReceived(BMessage *msg);
		char *GetUser();

	private:
		PickUserView *infoView;
		char user[33];
};
