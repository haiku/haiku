// ----- MemberItem ----------------------------------------------------------------------

class MemberItem : public ListItem
{
	public:
		MemberItem(BView *headerView, const char *text0, const char *text1, const char *text2);
		~MemberItem();

		void ItemSelected();
		void ItemDeselected();
};

class BTextControl;

class GroupPropertiesView : public BView
{
	public:
		GroupPropertiesView(BRect rect, const char *name);
		~GroupPropertiesView();

		void Draw(BRect rect);
		void UpdateInfo();
		char *GetSelectedMember();
		void ItemSelected();
		void ItemDeselected();
		void AddNewMember(char *user);
		void RemoveSelectedMember();

		BButton *GetRemoveButton()	{ return removeBtn; }
		char *getGroup()			{ return group; }
		char *getDesc()				{ return desc; }

	private:
		void AddGroupMembers(ColumnListView* listView);

		BBitmap *icon;
		BTextControl *editName;
		BTextControl *editDesc;
		SmartColumnListView *listView;
		BButton *removeBtn;

		char group[33];
		char desc[64];
		bool newGroup;
};

class GroupPropertiesPanel : public BWindow
{
	public:
		GroupPropertiesPanel(BRect frame, const char *name, BWindow *parent);

		void MessageReceived(BMessage *msg);

		char *getGroup()			{ return group; }
		char *getDesc()				{ return desc; }
		bool isCancelled()			{ return cancelled; }

	private:
		GroupPropertiesView *infoView;
		BWindow *shareWin;
		char group[33];
		char desc[64];
		bool newGroup;
		bool cancelled;
};
