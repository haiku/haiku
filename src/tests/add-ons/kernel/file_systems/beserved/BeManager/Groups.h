// ----- GroupItem ----------------------------------------------------------------------

class GroupItem : public ListItem
{
	public:
		GroupItem(BView *headerView, ColumnListView *listView, const char *text0, const char *text1);
		~GroupItem();

		bool ItemInvoked();
		void ItemSelected();
		void ItemDeselected();
		void ItemDeleted();
		bool IsDeleteable();

	private:
		ColumnListView *list;
};


// ----- GroupsItem -----------------------------------------------------

class GroupsItem : public TreeItem
{
	public:
		GroupsItem(uint32 level, bool superitem, bool expanded, int32 resourceID, BView *headerView, ColumnListView *listView, char *text);

		void ItemSelected();
		void ListItemSelected();
		void ListItemDeselected();
		void ListItemUpdated(int index, ListItem *item);
		bool HeaderMessageReceived(BMessage *msg);

	private:
		void BuildHeader();

		BButton *btnEdit;
		BButton *btnRemove;
};
