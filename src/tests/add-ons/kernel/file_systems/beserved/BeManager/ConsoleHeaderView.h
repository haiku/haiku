class ConsoleHeaderView : public BView
{
	public:
		ConsoleHeaderView(BRect rect);
		~ConsoleHeaderView();

		void Draw(BRect rect);

		BButton *openBtn;
		BButton *hostBtn;

	private:
		BBitmap *icon;
};
