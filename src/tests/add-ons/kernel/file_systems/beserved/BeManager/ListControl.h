#ifndef max
#define max(a,b)					((a)>=(b)?(a):(b))
#endif

class ListItem : public CLVEasyItem
{
	public:
		ListItem(BView *headerView, const char *text0, const char *text1, const char *text2);
		~ListItem();

		BView *GetAssociatedHeader()			{ return assocHeaderView; }
		void SetAssociatedHeader(BView *header)	{ assocHeaderView = header; }

		virtual bool ItemInvoked();
		virtual void ItemSelected();
		virtual void ItemDeselected();
		virtual void ItemDeleted();
		virtual bool IsDeleteable();

	private:
		BView *assocHeaderView;
};


class SmartColumnListView : public ColumnListView
{
	public:
		SmartColumnListView(BRect Frame, CLVContainerView** ContainerView,
			const char* Name = NULL, uint32 ResizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
			list_view_type Type = B_SINGLE_SELECTION_LIST,
			bool hierarchical = false, bool horizontal = true, bool vertical = true,
			bool scroll_view_corner = true, border_style border = B_NO_BORDER,
			const BFont* LabelFont = be_plain_font);

		virtual ~SmartColumnListView();

		// Event handlers (what makes it "smart").
		void MouseDown(BPoint point);
		void KeyUp(const char *bytes, int32 numBytes);

		// Helper functions.
		bool DeleteItem(int index, ListItem *item);

	private:
		CLVContainerView *container;
};


extern const int32 MSG_LIST_DESELECT;
