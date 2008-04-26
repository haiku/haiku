// ----- ServerItem ----------------------------------------------------------------------

class ServerItem : public ListItem
{
	public:
		ServerItem(BView *headerView, ColumnListView *listView, const char *text0);
		~ServerItem();

		bool ItemInvoked();
		void ItemSelected();
		void ItemDeselected();
		void ItemDeleted();
		bool IsDeleteable();

	private:
		ColumnListView *list;
};


// ----- ServersItem -----------------------------------------------------

class ServersItem : public TreeItem
{
	public:
		ServersItem(uint32 level, bool superitem, bool expanded, int32 resourceID, BView *headerView, ColumnListView *listView, char *text);

		void ItemSelected();
		void ListItemSelected();
		void ListItemDeselected();
		void ListItemUpdated(int index, ListItem *item);
		bool HeaderMessageReceived(BMessage *msg);

	private:
		void BuildHeader();

		BButton *btnEdit;
};
