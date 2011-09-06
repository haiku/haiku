
#include <stdio.h>
#include <stdlib.h>

#include <typeinfo>

#include <Application.h>
#include <Button.h>
#include <CardLayout.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <ListView.h>
#include <MenuField.h>
#include <RadioButton.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>


static const rgb_color kBlack	= {0, 0, 0, 255};
static const rgb_color kRed		= {255, 0, 0, 255};

// message what codes
enum {
	MSG_TEST_SELECTED		= 'tsts',

	// used in tests
	MSG_TOGGLE_1			= 'tgl1',
	MSG_TOGGLE_2			= 'tgl2',

	MSG_FIXED_ASPECT_RATIO	= 'hwas',
	MSG_FIXED_SUM			= 'hwfs',
	MSG_FIXED_PRODUCT		= 'hwfp',
};

// HeightForWidthTestView types
enum {
	FIXED_SUM,
	FIXED_PRODUCT,
	FIXED_ASPECT_RATIO,
};


// TestView
class TestView : public BView {
public:
	TestView(const rgb_color& color = kBlack)
		: BView("test view", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
		  fColor(color)
	{
	}

	void SetColor(const rgb_color& color)
	{
		fColor = color;
		Invalidate();
	}

	virtual void Draw(BRect updateRect)
	{
		SetHighColor(fColor);

		BRect bounds(Bounds());
		StrokeRect(bounds);

		BPoint rightBottom = bounds.RightBottom();
		StrokeLine(B_ORIGIN, rightBottom);
		StrokeLine(BPoint(rightBottom.x, 0), BPoint(0, rightBottom.y));
	}

	virtual BSize MinSize()
	{
		return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(10, 10));
	}

	virtual BSize PreferredSize()
	{
		return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
			BSize(50, 50));
	}

private:
	rgb_color	fColor;
};


// HeightForWidthTestView
class HeightForWidthTestView : public TestView {
public:
	HeightForWidthTestView(uint32 type, float value)
		: fType(NULL)
	{
		SetType(type, value);
	}

	HeightForWidthTestView(const rgb_color& color, uint32 type, float value)
		: TestView(color),
		  fType(NULL)
	{
		SetType(type, value);
	}

	~HeightForWidthTestView()
	{
		delete fType;
	}

	void SetType(uint32 type, float value)
	{
		delete fType;

		switch (type) {
			case FIXED_SUM:
				fType = new FixedSumType((int)value);
				break;
			case FIXED_PRODUCT:
				fType = new FixedProductType((int)value);
				break;
			case FIXED_ASPECT_RATIO:
			default:
				fType = new FixedAspectRatioType(value);
				break;
		}

		InvalidateLayout();
	}

	BSize MinSize() {
		return BLayoutUtils::ComposeSize(ExplicitMinSize(), fType->MinSize());
	}

	BSize MaxSize() {
		return BLayoutUtils::ComposeSize(ExplicitMaxSize(), fType->MaxSize());
	}

	BSize PreferredSize() {
		return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
			fType->PreferredSize());
	}

	bool HasHeightForWidth() {
		return true;
	}

	void GetHeightForWidth(float width, float* minHeight, float* maxHeight,
			float* preferredHeight) {
		float dummy;
		fType->GetHeightForWidth(width,
			minHeight ? minHeight : &dummy,
			maxHeight ? maxHeight : &dummy,
			preferredHeight ? preferredHeight : &dummy);
	}

private:
	class HeightForWidthType {
	public:
		virtual ~HeightForWidthType()
		{
		}

		virtual BSize MinSize() = 0;
		virtual BSize MaxSize() = 0;
		virtual BSize PreferredSize() = 0;
		virtual void GetHeightForWidth(float width, float* minHeight,
			float* maxHeight, float* preferredHeight) = 0;
	};

	class FixedAspectRatioType : public HeightForWidthType {
	public:
		FixedAspectRatioType(float ratio)
			: fAspectRatio(ratio)
		{
		}
		
		virtual BSize MinSize()
		{
			return BSize(-1, -1);
		}

		virtual BSize MaxSize()
		{
			return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
		}

		virtual BSize PreferredSize()
		{
			float preferredWidth = 49;
			float dummy, preferredHeight;
			GetHeightForWidth(preferredWidth, &dummy, &dummy, &preferredHeight);
			return BSize(preferredWidth, preferredHeight);
		}

		virtual void GetHeightForWidth(float width, float* minHeight,
			float* maxHeight, float* preferredHeight)
		{
			float height = floor((width + 1) * fAspectRatio) - 1;
			*minHeight = height;
			*maxHeight = height;
			*preferredHeight = height;
		}

	private:
		float	fAspectRatio;
	};

	class FixedSumType : public HeightForWidthType {
	public:
		FixedSumType(float sum)
			: fSum(sum)
		{
		}

		virtual BSize MinSize()
		{
			return BSize(0, 0);
		}

		virtual BSize MaxSize()
		{
			return BSize(fSum - 2, fSum - 2);
		}

		virtual BSize PreferredSize()
		{
			float preferredWidth = floor(fSum / 2) - 1;
			float dummy, preferredHeight;
			GetHeightForWidth(preferredWidth, &dummy, &dummy, &preferredHeight);
			return BSize(preferredWidth, preferredHeight);
		}

		virtual void GetHeightForWidth(float width, float* minHeight,
			float* maxHeight, float* preferredHeight)
		{
			float height = fSum - (width + 1) - 1;
			*minHeight = height;
			*maxHeight = height;
			*preferredHeight = height;
		}

	private:
		float	fSum;
	};

	class FixedProductType : public HeightForWidthType {
	public:
		FixedProductType(float product)
			: fProduct(product)
		{
		}

		virtual BSize MinSize()
		{
			return BSize(0, 0);
		}

		virtual BSize MaxSize()
		{
			return BSize(fProduct - 1, fProduct - 1);
		}

		virtual BSize PreferredSize()
		{
			float preferredWidth = floor(sqrt(fProduct));
			float dummy, preferredHeight;
			GetHeightForWidth(preferredWidth, &dummy, &dummy, &preferredHeight);
			return BSize(preferredWidth, preferredHeight);
		}

		virtual void GetHeightForWidth(float width, float* minHeight,
			float* maxHeight, float* preferredHeight)
		{
			float height = floor(fProduct / (width + 1)) - 1;
			*minHeight = height;
			*maxHeight = height;
			*preferredHeight = height;
		}

	private:
		float	fProduct;
	};

private:
	HeightForWidthType*	fType;
};


// Test
struct Test : BHandler {
	BString	name;
	BView*	rootView;
	BString	description;

	Test(const char* name, const char* description)
		: BHandler(name),
		  name(name),
		  rootView(new BView(name, 0)),
		  description(description)
	{
	}

	virtual ~Test()
	{
	}

	virtual void RegisterListeners()
	{
	}
};


// GroupLayoutTest1
struct GroupLayoutTest1 : public Test {
	GroupLayoutTest1()
		:
		Test("Group", "Simple BGroupLayout.")
	{
		 BLayoutBuilder::Group<>(rootView, B_HORIZONTAL)
			// controls
			.AddGroup(B_VERTICAL)
				.Add(toggleRowButton = new BButton("Toggle Row",
						new BMessage(MSG_TOGGLE_1)))
				.Add(toggleViewButton = new BButton("Toggle View",
						new BMessage(MSG_TOGGLE_2)))
				.AddGlue()
				.End()

			// test views
			.AddGroup(B_VERTICAL)
				// row 1
				.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1)
					.Add(new TestView(), 1)
					.Add(toggledView = new TestView(), 2)
					.Add(new TestView(), 3)
					.End()

				// row 2
				.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 2)
					.GetView(&toggledRow)
					.Add(new TestView())
					.Add(new TestView())
					.Add(new TestView())
					.End()

				// row 3
				.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 3)
					.Add(new TestView(), 3)
					.Add(new TestView(), 2)
					.Add(new TestView(), 1);
	}

	virtual void RegisterListeners()
	{
		toggleRowButton->SetTarget(this);
		toggleViewButton->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_TOGGLE_1:
			{
				if (toggledRow->IsHidden(toggledRow))
					toggledRow->Show();
				else
					toggledRow->Hide();
				break;
			}

			case MSG_TOGGLE_2:
			{
				if (toggledView->IsHidden(toggledView))
					toggledView->Show();
				else
					toggledView->Hide();
				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	BButton*	toggleRowButton;
	BButton*	toggleViewButton;
	BView*		toggledRow;
	TestView*	toggledView;
};


// GroupAlignedLayoutTest1
struct GroupAlignedLayoutTest1 : public Test {
	GroupAlignedLayoutTest1()
		: Test("Group aligned", "Simple BGroupLayout, rows 1 and 3 aligned.")
	{
		BGroupView* rootView  = new BGroupView(B_HORIZONTAL, 10);
		this->rootView = rootView;

		// controls

		BGroupView* controls = new BGroupView(B_VERTICAL, 10);
		rootView->AddChild(controls);
		
		toggleRowButton = new BButton("Toggle Row", new BMessage(MSG_TOGGLE_1));
		controls->AddChild(toggleRowButton);
		
		toggleViewButton = new BButton("Toggle View",
			new BMessage(MSG_TOGGLE_2));
		controls->AddChild(toggleViewButton);


		controls->AddChild(BSpaceLayoutItem::CreateGlue());
		
		// test views
		
		BGroupView* testViews = new BGroupView(B_VERTICAL, 10);
		rootView->AddChild(testViews);

		// row 1
		BGroupView* row = new BGroupView(B_HORIZONTAL, 10);
		BGroupView* row1 = row;
		testViews->GroupLayout()->AddView(row, 1);

		row->GroupLayout()->AddView(new TestView(), 1);
		toggledView = new TestView();
		row->GroupLayout()->AddView(toggledView, 2);
		row->GroupLayout()->AddView(new TestView(), 3);

		// row 2
		row = new BGroupView(B_HORIZONTAL, 10);
		toggledRow = row;
		testViews->GroupLayout()->AddView(row, 2);

		row->GroupLayout()->AddView(new TestView());
		row->GroupLayout()->AddView(new TestView());
		row->GroupLayout()->AddView(new TestView());
		
		// row 3
		row = new BGroupView(B_HORIZONTAL, 10);
		BGroupView* row3 = row;
		testViews->GroupLayout()->AddView(row, 3);

		row->GroupLayout()->AddView(new TestView(), 3);
		row->GroupLayout()->AddView(new TestView(), 2);
		row->GroupLayout()->AddView(new TestView(), 1);

		// align rows 1 and 3
		row1->GroupLayout()->AlignLayoutWith(row3->GroupLayout(), B_HORIZONTAL);
	}

	virtual void RegisterListeners()
	{
		toggleRowButton->SetTarget(this);
		toggleViewButton->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_TOGGLE_1:
			{
				if (toggledRow->IsHidden(toggledRow))
					toggledRow->Show();
				else
					toggledRow->Hide();
				break;
			}

			case MSG_TOGGLE_2:
			{
				if (toggledView->IsHidden(toggledView))
					toggledView->Show();
				else
					toggledView->Hide();
				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	BButton*	toggleRowButton;
	BButton*	toggleViewButton;
	BGroupView*	toggledRow;
	TestView*	toggledView;
};


// GridLayoutTest1
struct GridLayoutTest1 : public Test {
	GridLayoutTest1()
		: Test("Grid", "Simple BGridLayout.")
	{
		BLayoutBuilder::Group<>(rootView, B_HORIZONTAL)
			// controls
			.AddGroup(B_VERTICAL)
				.Add(toggleView1Button = new BButton("Toggle View 1",
						new BMessage(MSG_TOGGLE_1)))
				.Add(toggleView2Button = new BButton("Toggle View 2",
						new BMessage(MSG_TOGGLE_2)))
				.AddGlue()
				.End()

			// test views
			.AddGrid()
				// row 1
				.Add(toggledView1 = new TestView(), 0, 0, 3, 1)
				.Add(new TestView(), 3, 0)
				.Add(new TestView(), 4, 0)
	
				// row 2
				.Add(new TestView(), 0, 1)
				.Add(new TestView(), 1, 1)
				.Add(new TestView(), 2, 1)
				.Add(new TestView(), 3, 1)
				.Add(new TestView(), 4, 1)
	
				// row 3
				.Add(new TestView(), 0, 2)
				.Add(toggledView2 = new TestView(), 1, 2, 2, 2)
				.Add(new TestView(), 3, 2)
				.Add(new TestView(), 4, 2)
	
				// row 4
				.Add(new TestView(), 0, 3)
				.Add(new TestView(), 3, 3)
				.Add(new TestView(), 4, 3)
	
				// weights
				.SetColumnWeight(0, 1)
				.SetColumnWeight(1, 2)
				.SetColumnWeight(2, 3)
				.SetColumnWeight(3, 4)
				.SetColumnWeight(4, 5)
	
				.SetRowWeight(0, 1)
				.SetRowWeight(1, 2)
				.SetRowWeight(2, 3)
				.SetRowWeight(3, 4);
	}

	virtual void RegisterListeners()
	{
		toggleView1Button->SetTarget(this);
		toggleView2Button->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_TOGGLE_1:
			{
				if (toggledView1->IsHidden(toggledView1))
					toggledView1->Show();
				else
					toggledView1->Hide();
				break;
			}

			case MSG_TOGGLE_2:
			{
				if (toggledView2->IsHidden(toggledView2))
					toggledView2->Show();
				else
					toggledView2->Hide();
				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	BButton*	toggleView1Button;
	BButton*	toggleView2Button;
	TestView*	toggledView1;
	TestView*	toggledView2;
};


// SplitterGroupLayoutTest1
struct SplitterGroupLayoutTest1 : public Test {
	SplitterGroupLayoutTest1()
		: Test("Group, splitters 1", "BGroupLayout with BSplitters.")
	{
		BLayoutBuilder::Group<>(rootView, B_HORIZONTAL)
			// controls
			.AddGroup(B_VERTICAL)
				.Add(toggleRowButton = new BButton("Toggle Row",
						new BMessage(MSG_TOGGLE_1)))
				.Add(toggleViewButton = new BButton("Toggle View",
						new BMessage(MSG_TOGGLE_2)))
				.AddGlue()
				.End()

			// test views
			.AddSplit(B_VERTICAL)
				// row 1
				.AddSplit(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1)
					.Add(new TestView(), 1)
					.Add(toggledView = new TestView(), 2)
					.Add(new TestView(), 3)
					.End()
				// make the row uncollapsible
				.SetCollapsible(false)

				// row 2
				.AddSplit(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 2)
					.GetView(&toggledRow)
					.Add(new TestView())
					.Add(new TestView())
					.Add(new TestView())
					.End()

				// row 3
				.AddSplit(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 3)
					.Add(new TestView(), 3)
					.Add(new TestView(), 2)
					.Add(new TestView(), 1)
					.End()
				// make the row uncollapsible
				.SetCollapsible(false);
	}

	virtual void RegisterListeners()
	{
		toggleRowButton->SetTarget(this);
		toggleViewButton->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_TOGGLE_1:
			{
				if (toggledRow->IsHidden(toggledRow))
					toggledRow->Show();
				else
					toggledRow->Hide();
				break;
			}

			case MSG_TOGGLE_2:
			{
				if (toggledView->IsHidden(toggledView))
					toggledView->Show();
				else
					toggledView->Hide();
				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	BButton*	toggleRowButton;
	BButton*	toggleViewButton;
	BView*		toggledRow;
	TestView*	toggledView;
};


// SplitterGroupLayoutTest2
struct SplitterGroupLayoutTest2 : public Test {
	SplitterGroupLayoutTest2()
		: Test("Group, splitters 2",
			"BGroupLayout with BSplitters. Restricted maximum widths.")
	{
		TestView* testView1 = new TestView();
		TestView* testView2 = new TestView();
		TestView* testView3 = new TestView();

		BLayoutBuilder::Group<>(rootView, B_HORIZONTAL)
			// test views
			.AddGroup(B_VERTICAL)
				// split view
				.AddSplit(B_HORIZONTAL)
					.Add(testView1, 0)
					.Add(testView2, 1)
					.Add(testView3, 2);

		// set maximal width on the test views
		testView1->SetExplicitMaxSize(BSize(100, B_SIZE_UNSET));
		testView2->SetExplicitMaxSize(BSize(100, B_SIZE_UNSET));
		testView3->SetExplicitMaxSize(BSize(100, B_SIZE_UNSET));
	}
};


// SplitterGridLayoutTest1
struct SplitterGridLayoutTest1 : public Test {
	SplitterGridLayoutTest1()
		: Test("Grid, h splitters", "BGridLayout with horizontal BSplitters.")
	{
		BGridLayout* layouts[3];

		BLayoutBuilder::Group<>(rootView, B_HORIZONTAL)
			// controls
			.AddGroup(B_VERTICAL)
				.Add(toggleView1Button = new BButton("Toggle View 1",
						new BMessage(MSG_TOGGLE_1)))
				.Add(toggleView2Button = new BButton("Toggle View 2",
						new BMessage(MSG_TOGGLE_2)))
				.AddGlue()
				.End()

			// test views
			.AddSplit(B_HORIZONTAL)
				// splitter element 1
				.AddGrid(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 6)
					.GetLayout(&layouts[0])
					// row 1
					.Add(toggledView1 = new TestView(), 0, 0, 3, 1)
					// row 2
					.Add(new TestView(), 0, 1)
					.Add(new TestView(), 1, 1)
					.Add(new TestView(), 2, 1)
					// row 3
					.Add(new TestView(), 0, 2)
					.Add(toggledView2 = new TestView(), 1, 2, 2, 2)
					// row 4
					.Add(new TestView(), 0, 3)

					// column weights
					.SetColumnWeight(0, 1)
					.SetColumnWeight(1, 2)
					.SetColumnWeight(2, 3)
					.End()
				
				// splitter element 2
				.AddGrid(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 4)
					.GetLayout(&layouts[1])
					// row 1
					.Add(new TestView(), 0, 0)
					// row 2
					.Add(new TestView(), 0, 1)
					// row 3
					.Add(new TestView(), 0, 2)
					// row 4
					.Add(new TestView(), 0, 3)
					.End()

				// splitter element 3
				.AddGrid(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 5)
					.GetLayout(&layouts[2])
					// row 1
					.Add(new TestView(), 0, 0)
					// row 2
					.Add(new TestView(), 0, 1)
					// row 3
					.Add(new TestView(), 0, 2)
					// row 4
					.Add(new TestView(), 0, 3);

		// set row weights
		for (int i = 0; i < 3; i++) {
			layouts[i]->SetRowWeight(0, 1);
			layouts[i]->SetRowWeight(1, 2);
			layouts[i]->SetRowWeight(2, 3);
			layouts[i]->SetRowWeight(3, 4);
		}

		// set explicit min/max heights for toggled views
		toggledView1->SetExplicitMinSize(BSize(B_SIZE_UNSET, 100));
		toggledView2->SetExplicitMaxSize(BSize(B_SIZE_UNSET, 200));

		// align the layouts
		layouts[0]->AlignLayoutWith(layouts[1], B_VERTICAL);
		layouts[0]->AlignLayoutWith(layouts[2], B_VERTICAL);
	}

	virtual void RegisterListeners()
	{
		toggleView1Button->SetTarget(this);
		toggleView2Button->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_TOGGLE_1:
			{
				if (toggledView1->IsHidden(toggledView1))
					toggledView1->Show();
				else
					toggledView1->Hide();
				break;
			}

			case MSG_TOGGLE_2:
			{
				if (toggledView2->IsHidden(toggledView2))
					toggledView2->Show();
				else
					toggledView2->Hide();
				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	BButton*	toggleView1Button;
	BButton*	toggleView2Button;
	TestView*	toggledView1;
	TestView*	toggledView2;
};


// SplitterGridLayoutTest2
struct SplitterGridLayoutTest2 : public Test {
	SplitterGridLayoutTest2()
		: Test("Grid, v splitters", "BGridLayout with vertical BSplitters.")
	{
		BGridLayout* layouts[3];

		BLayoutBuilder::Group<>(rootView, B_HORIZONTAL)
			// controls
			.AddGroup(B_VERTICAL)
				.Add(toggleView1Button = new BButton("Toggle View 1",
						new BMessage(MSG_TOGGLE_1)))
				.Add(toggleView2Button = new BButton("Toggle View 2",
						new BMessage(MSG_TOGGLE_2)))
				.AddGlue()
			.End()

			// test views
			.AddSplit(B_VERTICAL)
				// splitter element 1
				.AddGrid(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 1)
					.GetLayout(&layouts[0])
					// row 1
					.Add(toggledView1 = new TestView(), 0, 0, 3, 1)
					.Add(new TestView(), 3, 0)
					.Add(new TestView(), 4, 0)
					.End()
				
				// splitter element 2
				.AddGrid(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 2)
					.GetLayout(&layouts[1])
					// row 2
					.Add(new TestView(), 0, 0)
					.Add(new TestView(), 1, 0)
					.Add(new TestView(), 2, 0)
					.Add(new TestView(), 3, 0)
					.Add(new TestView(), 4, 0)
					.End()

				// splitter element 3
				.AddGrid(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 7)
					.GetLayout(&layouts[2])
					// row 3
					.Add(new TestView(), 0, 0)
					.Add(toggledView2 = new TestView(), 1, 0, 2, 2)
					.Add(new TestView(), 3, 0)
					.Add(new TestView(), 4, 0)
					// row 4
					.Add(new TestView(), 0, 1)
					.Add(new TestView(), 3, 1)
					.Add(new TestView(), 4, 1)

					// row weights
					.SetRowWeight(0, 3)
					.SetRowWeight(1, 4);

		// set column weights
		for (int i = 0; i < 3; i++) {
			layouts[i]->SetColumnWeight(0, 1);
			layouts[i]->SetColumnWeight(1, 2);
			layouts[i]->SetColumnWeight(2, 3);
			layouts[i]->SetColumnWeight(3, 4);
			layouts[i]->SetColumnWeight(4, 5);
		}

		// align the layouts
		layouts[0]->AlignLayoutWith(layouts[1], B_HORIZONTAL);
		layouts[0]->AlignLayoutWith(layouts[2], B_HORIZONTAL);
	}

	virtual void RegisterListeners()
	{
		toggleView1Button->SetTarget(this);
		toggleView2Button->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_TOGGLE_1:
			{
				if (toggledView1->IsHidden(toggledView1))
					toggledView1->Show();
				else
					toggledView1->Hide();
				break;
			}

			case MSG_TOGGLE_2:
			{
				if (toggledView2->IsHidden(toggledView2))
					toggledView2->Show();
				else
					toggledView2->Hide();
				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	BButton*	toggleView1Button;
	BButton*	toggleView2Button;
	TestView*	toggledView1;
	TestView*	toggledView2;
};


// GroupLayoutHeightForWidthTestHorizontal1
struct GroupLayoutHeightForWidthTestHorizontal1 : public Test {
	GroupLayoutHeightForWidthTestHorizontal1()
		: Test("Group, height for width, h",
			"Horizontal BGroupLayout with height for width view.")
	{
		BLayoutBuilder::Group<>(rootView, B_HORIZONTAL)
			// controls
			.AddGroup(B_VERTICAL, 10, 0)
				.Add(aspectRatioButton = new BRadioButton("fixed aspect ratio",
						new BMessage(MSG_FIXED_ASPECT_RATIO)))
				.Add(sumButton = new BRadioButton("fixed sum",
						new BMessage(MSG_FIXED_SUM)))
				.Add(productButton = new BRadioButton("fixed product",
						new BMessage(MSG_FIXED_PRODUCT)))
				.AddGlue()
			.End()

			// test views
			.AddGroup(B_VERTICAL, 10)
				// row 1
				.AddGroup(B_HORIZONTAL, 10)
					.Add(new TestView())
					.Add(new TestView())
					.Add(new TestView())
				.End()

				// row 2
				.AddGroup(B_HORIZONTAL, 10)
					.Add(new TestView())
					.Add(heightForWidthView = new HeightForWidthTestView(kRed,
							FIXED_ASPECT_RATIO, 0.5f))
					.Add(new TestView())
				.End()

				// row 3
				.AddGroup(B_HORIZONTAL, 10)
					.Add(new TestView())
					.Add(new TestView())
					.Add(new TestView())
				.End()
			.End()
		;

		aspectRatioButton->SetValue(1);
	}

	virtual void RegisterListeners()
	{
		aspectRatioButton->SetTarget(this);
		sumButton->SetTarget(this);
		productButton->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_FIXED_ASPECT_RATIO:
			{
				heightForWidthView->SetType(FIXED_ASPECT_RATIO, 0.5f);
				break;
			}

			case MSG_FIXED_SUM:
			{
				heightForWidthView->SetType(FIXED_SUM, 200);
				break;
			}

			case MSG_FIXED_PRODUCT:
			{
				heightForWidthView->SetType(FIXED_PRODUCT, 40000);
				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	BRadioButton*			aspectRatioButton;
	BRadioButton*			sumButton;
	BRadioButton*			productButton;
	HeightForWidthTestView*	heightForWidthView;
};


// GroupLayoutHeightForWidthTestVertical1
struct GroupLayoutHeightForWidthTestVertical1 : public Test {
	GroupLayoutHeightForWidthTestVertical1()
		: Test("Group, height for width, v",
			"Vertical BGroupLayout with height for width view.")
	{
		BLayoutBuilder::Group<>(rootView, B_HORIZONTAL)
			// controls
			.AddGroup(B_VERTICAL, 10, 0)
				.Add(aspectRatioButton = new BRadioButton("fixed aspect ratio",
						new BMessage(MSG_FIXED_ASPECT_RATIO)))
				.Add(sumButton = new BRadioButton("fixed sum",
						new BMessage(MSG_FIXED_SUM)))
				.Add(productButton = new BRadioButton("fixed product",
						new BMessage(MSG_FIXED_PRODUCT)))
				.AddGlue()
			.End()

			// test views
			.AddGroup(B_HORIZONTAL, 10)
				// column 1
				.AddGroup(B_VERTICAL, 10)
					.Add(new TestView())
					.Add(new TestView())
					.Add(new TestView())
				.End()

				// column 2
				.AddGroup(B_VERTICAL, 10)
					.Add(new TestView())
					.Add(heightForWidthView = new HeightForWidthTestView(kRed,
							FIXED_ASPECT_RATIO, 0.5f))
					.Add(new TestView())
				.End()

				// column 3
				.AddGroup(B_VERTICAL, 10)
					.Add(new TestView())
					.Add(new TestView())
					.Add(new TestView())
				.End()
			.End()
		;

		aspectRatioButton->SetValue(1);
	}

	virtual void RegisterListeners()
	{
		aspectRatioButton->SetTarget(this);
		sumButton->SetTarget(this);
		productButton->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_FIXED_ASPECT_RATIO:
			{
				heightForWidthView->SetType(FIXED_ASPECT_RATIO, 0.5f);
				break;
			}

			case MSG_FIXED_SUM:
			{
				heightForWidthView->SetType(FIXED_SUM, 200);
				break;
			}

			case MSG_FIXED_PRODUCT:
			{
				heightForWidthView->SetType(FIXED_PRODUCT, 40000);
				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	BRadioButton*			aspectRatioButton;
	BRadioButton*			sumButton;
	BRadioButton*			productButton;
	HeightForWidthTestView*	heightForWidthView;
};


// GridLayoutHeightForWidthTest1
struct GridLayoutHeightForWidthTest1 : public Test {
	GridLayoutHeightForWidthTest1()
		: Test("Grid, height for width",
			"BGridLayout with height for width view.")
	{
		BLayoutBuilder::Group<>(rootView, B_HORIZONTAL)
			// controls
			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 0)
				.Add(aspectRatioButton = new BRadioButton("fixed aspect ratio",
						new BMessage(MSG_FIXED_ASPECT_RATIO)))
				.Add(sumButton = new BRadioButton("fixed sum",
						new BMessage(MSG_FIXED_SUM)))
				.Add(productButton = new BRadioButton("fixed product",
						new BMessage(MSG_FIXED_PRODUCT)))
				.AddGlue()
			.End()

			// test views
			.AddGrid()
				// row 1
				.Add(new TestView(), 0, 0, 3, 1)
				.Add(new TestView(), 3, 0)
				.Add(new TestView(), 4, 0)
	
				// row 2
				.Add(new TestView(), 0, 1)
				.Add(new TestView(), 1, 1)
				.Add(new TestView(), 2, 1)
				.Add(new TestView(), 3, 1)
				.Add(new TestView(), 4, 1)
	
				// row 3
				.Add(new TestView(), 0, 2)
				.Add(heightForWidthView = new HeightForWidthTestView(kRed,
						FIXED_ASPECT_RATIO, 0.5f), 1, 2, 2, 2)
				.Add(new TestView(), 3, 2)
				.Add(new TestView(), 4, 2)
	
				// row 4
				.Add(new TestView(), 0, 3)
				.Add(new TestView(), 3, 3)
				.Add(new TestView(), 4, 3)
	
				// weights
				.SetColumnWeight(0, 1)
				.SetColumnWeight(1, 2)
				.SetColumnWeight(2, 3)
				.SetColumnWeight(3, 4)
				.SetColumnWeight(4, 5)
	
				.SetRowWeight(0, 1)
				.SetRowWeight(1, 2)
				.SetRowWeight(2, 3)
				.SetRowWeight(3, 4);

		aspectRatioButton->SetValue(1);
	}

	virtual void RegisterListeners()
	{
		aspectRatioButton->SetTarget(this);
		sumButton->SetTarget(this);
		productButton->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_FIXED_ASPECT_RATIO:
			{
				heightForWidthView->SetType(FIXED_ASPECT_RATIO, 0.5f);
				break;
			}

			case MSG_FIXED_SUM:
			{
				heightForWidthView->SetType(FIXED_SUM, 200);
				break;
			}

			case MSG_FIXED_PRODUCT:
			{
				heightForWidthView->SetType(FIXED_PRODUCT, 40000);
				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	BRadioButton*			aspectRatioButton;
	BRadioButton*			sumButton;
	BRadioButton*			productButton;
	HeightForWidthTestView*	heightForWidthView;
};


// SplitterGroupLayoutHeightForWidthTest1
struct SplitterGroupLayoutHeightForWidthTest1 : public Test {
	SplitterGroupLayoutHeightForWidthTest1()
		: Test("Group, splitters, height for width",
			"Horizontal BGroupLayout with height for width view and "
				"BSplitters.")
	{
		BLayoutBuilder::Group<>(rootView, B_HORIZONTAL)
			// controls
			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 0)
				.Add(aspectRatioButton = new BRadioButton("fixed aspect ratio",
						new BMessage(MSG_FIXED_ASPECT_RATIO)))
				.Add(sumButton = new BRadioButton("fixed sum",
						new BMessage(MSG_FIXED_SUM)))
				.Add(productButton = new BRadioButton("fixed product",
						new BMessage(MSG_FIXED_PRODUCT)))
				.AddGlue()
			.End()

			// test views
			.AddSplit(B_VERTICAL, B_USE_DEFAULT_SPACING, 1)
				// row 1
				.AddSplit(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1)
					.Add(new TestView(), 1)
					.Add(new TestView(), 2)
					.Add(new TestView(), 3)
					.End()
				// make the row uncollapsible
				.SetCollapsible(false)

				// row 2
				.AddSplit(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 2)
					.Add(new TestView())
					.Add(heightForWidthView = new HeightForWidthTestView(kRed,
							FIXED_ASPECT_RATIO, 0.5f))
					.Add(new TestView())
					.End()

				// row 3
				.AddSplit(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 3)
					.Add(new TestView(), 3)
					.Add(new TestView(), 2)
					.Add(new TestView(), 1)
					.End()
				// make the row uncollapsible
				.SetCollapsible(false);

		aspectRatioButton->SetValue(1);
	}

	virtual void RegisterListeners()
	{
		aspectRatioButton->SetTarget(this);
		sumButton->SetTarget(this);
		productButton->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_FIXED_ASPECT_RATIO:
			{
				heightForWidthView->SetType(FIXED_ASPECT_RATIO, 0.5f);
				break;
			}

			case MSG_FIXED_SUM:
			{
				heightForWidthView->SetType(FIXED_SUM, 200);
				break;
			}

			case MSG_FIXED_PRODUCT:
			{
				heightForWidthView->SetType(FIXED_PRODUCT, 40000);
				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	BRadioButton*			aspectRatioButton;
	BRadioButton*			sumButton;
	BRadioButton*			productButton;
	HeightForWidthTestView*	heightForWidthView;
};


// SplitterGridLayoutHeightForWidthTest1
struct SplitterGridLayoutHeightForWidthTest1 : public Test {
	SplitterGridLayoutHeightForWidthTest1()
		: Test("Grid, splitters, height for width",
			"BGridLayout with height for width view and horizontal BSplitters.")
	{
		BGridLayout* layouts[3];

		BLayoutBuilder::Group<>(rootView, B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			// controls
			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 0)
				.Add(aspectRatioButton = new BRadioButton("fixed aspect ratio",
						new BMessage(MSG_FIXED_ASPECT_RATIO)))
				.Add(sumButton = new BRadioButton("fixed sum",
						new BMessage(MSG_FIXED_SUM)))
				.Add(productButton = new BRadioButton("fixed product",
						new BMessage(MSG_FIXED_PRODUCT)))
				.AddGlue()
			.End()

			// test views
			.AddSplit(B_HORIZONTAL)
				// splitter element 1
				.AddGrid(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 6)
					.GetLayout(&layouts[0])
					// row 1
					.Add(new TestView(), 0, 0, 3, 1)
					// row 2
					.Add(new TestView(), 0, 1)
					.Add(new TestView(), 1, 1)
					.Add(new TestView(), 2, 1)
					// row 3
					.Add(new TestView(), 0, 2)
					.Add(heightForWidthView = new HeightForWidthTestView(kRed,
							FIXED_ASPECT_RATIO, 0.5f), 1, 2, 2, 2)
					// row 4
					.Add(new TestView(), 0, 3)

					// column weights
					.SetColumnWeight(0, 1)
					.SetColumnWeight(1, 2)
					.SetColumnWeight(2, 3)
					.End()
				
				// splitter element 2
				.AddGrid(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 4)
					.GetLayout(&layouts[1])
					// row 1
					.Add(new TestView(), 0, 0)
					// row 2
					.Add(new TestView(), 0, 1)
					// row 3
					.Add(new TestView(), 0, 2)
					// row 4
					.Add(new TestView(), 0, 3)
					.End()

				// splitter element 3
				.AddGrid(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 5)
					.GetLayout(&layouts[2])
					// row 1
					.Add(new TestView(), 0, 0)
					// row 2
					.Add(new TestView(), 0, 1)
					// row 3
					.Add(new TestView(), 0, 2)
					// row 4
					.Add(new TestView(), 0, 3);

		// set row weights
		for (int i = 0; i < 3; i++) {
			layouts[i]->SetRowWeight(0, 1);
			layouts[i]->SetRowWeight(1, 2);
			layouts[i]->SetRowWeight(2, 3);
			layouts[i]->SetRowWeight(3, 4);
		}

		// align the layouts
		layouts[0]->AlignLayoutWith(layouts[1], B_VERTICAL);
		layouts[0]->AlignLayoutWith(layouts[2], B_VERTICAL);

		aspectRatioButton->SetValue(1);
	}

	virtual void RegisterListeners()
	{
		aspectRatioButton->SetTarget(this);
		sumButton->SetTarget(this);
		productButton->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_FIXED_ASPECT_RATIO:
			{
				heightForWidthView->SetType(FIXED_ASPECT_RATIO, 0.5f);
				break;
			}

			case MSG_FIXED_SUM:
			{
				heightForWidthView->SetType(FIXED_SUM, 200);
				break;
			}

			case MSG_FIXED_PRODUCT:
			{
				heightForWidthView->SetType(FIXED_PRODUCT, 40000);
				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	BRadioButton*			aspectRatioButton;
	BRadioButton*			sumButton;
	BRadioButton*			productButton;
	HeightForWidthTestView*	heightForWidthView;
};


// LabelTest1
struct LabelTest1 : public Test {
	LabelTest1()
		: Test("BTextControl, BMenuField, grid",
			"Aligning BTextControl/BMenuField labels using a 2 column "
			"BGridLayout.")
	{
		textControl1 = new BTextControl("Label", NULL, NULL);
		textControl2 = new BTextControl("Long Label", NULL, NULL);
		textControl3 = new BTextControl("Very Long Label", NULL, NULL);

		menuField1 = new BMenuField("Label", new BMenu("Options"));
		menuField2 = new BMenuField("Long Label",
			new BMenu("More Options"));
		menuField3 = new BMenuField("Very Long Label",
			new BMenu("Obscure Options"));

		BLayoutBuilder::Group<>(rootView, B_VERTICAL)
			// controls
			.AddGroup(B_HORIZONTAL)
				.Add(changeLabelsButton = new BButton("Random Labels",
						new BMessage(MSG_TOGGLE_1)))
				.AddGlue()
				.End()

			.AddGlue()

			// test views
			.AddGrid()
				// padding
				.Add(BSpaceLayoutItem::CreateGlue(), 0, 0)
				.Add(BSpaceLayoutItem::CreateGlue(), 1, 0)

				// row 1
				.Add(textControl1->CreateLabelLayoutItem(), 0, 1)
				.Add(textControl1->CreateTextViewLayoutItem(), 1, 1)
	
				// row 2
				.Add(textControl2->CreateLabelLayoutItem(), 0, 2)
				.Add(textControl2->CreateTextViewLayoutItem(), 1, 2)
	
				// row 3
				.Add(textControl3->CreateLabelLayoutItem(), 0, 3)
				.Add(textControl3->CreateTextViewLayoutItem(), 1, 3)

				// row 4
				.Add(menuField1->CreateLabelLayoutItem(), 0, 4)
				.Add(menuField1->CreateMenuBarLayoutItem(), 1, 4)
	
				// row 5
				.Add(menuField2->CreateLabelLayoutItem(), 0, 5)
				.Add(menuField2->CreateMenuBarLayoutItem(), 1, 5)
	
				// row 6
				.Add(menuField3->CreateLabelLayoutItem(), 0, 6)
				.Add(menuField3->CreateMenuBarLayoutItem(), 1, 6)

				// padding
				.Add(BSpaceLayoutItem::CreateGlue(), 0, 7)
				.Add(BSpaceLayoutItem::CreateGlue(), 1, 7)
				.End()
			.AddGlue();
	}

	virtual void RegisterListeners()
	{
		changeLabelsButton->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_TOGGLE_1:
			{
				BTextControl* textControls[] = {
					textControl1, textControl2, textControl3
				};

				BMenuField* menuFields[] = {
					menuField1, menuField2, menuField3
				};
				
				for (int i = 0; i < 3; i++) {
					BTextControl* textControl = textControls[i];
					BMenuField* menuField = menuFields[i];

					textControl->SetLabel(_RandomLabel().String());
					menuField->SetLabel(_RandomLabel().String());
				}
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

	BString _RandomLabel() const
	{
		const char* digits = "0123456789";

		int length = rand() % 20;
		BString label("Random ");
		for (int k = 0; k < length; k++)
			label.Append(digits[k % 10], 1);

		return label;
	}

private:
	BButton*		changeLabelsButton;
	BTextControl*	textControl1;
	BTextControl*	textControl2;
	BTextControl*	textControl3;
	BMenuField*		menuField1;
	BMenuField*		menuField2;
	BMenuField*		menuField3;
};


// TestWindow
class TestWindow : public BWindow {
public:
	TestWindow()
		: BWindow(BRect(100, 100, 700, 500), "LayoutTest1",
			B_TITLED_WINDOW,
			B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
				| B_AUTO_UPDATE_SIZE_LIMITS)
	{
		fTestList = new BListView(BRect(0, 0, 10, 10), "test list",
			B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
		BView* scrollView = new BScrollView("test scroll view", fTestList);
		scrollView->SetExplicitMaxSize(BSize(190, B_SIZE_UNLIMITED));

		fTestCardLayout = new BCardLayout();
		BView* cardView = new BView("card view", 0, fTestCardLayout);

		BLayoutBuilder::Group<>(this, B_HORIZONTAL)
			.SetInsets(B_USE_WINDOW_INSETS)
			.Add(scrollView)
			.Add(cardView);

		// add the tests
		_AddTest(new GroupLayoutTest1());
		_AddTest(new GroupAlignedLayoutTest1());
		_AddTest(new GridLayoutTest1());
		_AddTest(new SplitterGroupLayoutTest1());
		_AddTest(new SplitterGroupLayoutTest2());
		_AddTest(new SplitterGridLayoutTest1());
		_AddTest(new SplitterGridLayoutTest2());
		_AddTest(new GroupLayoutHeightForWidthTestHorizontal1());
		_AddTest(new GroupLayoutHeightForWidthTestVertical1());
		_AddTest(new GridLayoutHeightForWidthTest1());
		_AddTest(new SplitterGroupLayoutHeightForWidthTest1());
		_AddTest(new SplitterGridLayoutHeightForWidthTest1());
		_AddTest(new LabelTest1());

		fTestList->SetSelectionMessage(new BMessage(MSG_TEST_SELECTED));
		_DumpViewHierarchy(GetLayout()->Owner());
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_TEST_SELECTED:
				fTestCardLayout->SetVisibleItem(fTestList->CurrentSelection());
				break;
			default:
				BWindow::MessageReceived(message);
				break;
		}
	}

private:
	void _AddTest(Test* test) {
		fTestList->AddItem(new BStringItem(test->name.String()));

		BGroupView* containerView = new BGroupView(B_VERTICAL, 0);

		BStringView* descriptionView = new BStringView("test description",
			test->description.String());

		descriptionView->SetExplicitMinSize(BSize(0, B_SIZE_UNSET));
		containerView->AddChild(descriptionView);

		// spacing/glue
		containerView->GroupLayout()->AddItem(
			BSpaceLayoutItem::CreateVerticalStrut(10));
		containerView->GroupLayout()->AddItem(
			BSpaceLayoutItem::CreateGlue(), 0);
		
		// the test view: wrap it, so we can have unlimited size
		BGroupView* wrapperView = new BGroupView(B_HORIZONTAL, 0);
		containerView->AddChild(wrapperView);
		wrapperView->AddChild(test->rootView);

		// glue
		containerView->GroupLayout()->AddItem(
			BSpaceLayoutItem::CreateGlue(), 0);
		
		wrapperView->SetExplicitMaxSize(
			BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		containerView->SetExplicitMaxSize(
			BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

		fTestCardLayout->AddView(containerView);

		AddHandler(test);
		test->RegisterListeners();
	}

	void _DumpViewHierarchy(BView* view, int32 indent = 0)
	{
		for (int32 i = 0; i < indent; i++)
			printf("  ");
		printf("view: %p", view);
		for (int32 i = 0; i < 15 - indent; i++)
			printf("  ");
		printf("(%s)\n", typeid(*view).name());

		int32 count = view->CountChildren();
		for (int32 i = 0; i < count; i++)
			_DumpViewHierarchy(view->ChildAt(i), indent + 1);
	}
	
private:
	BListView*		fTestList;
	BCardLayout*	fTestCardLayout;
};


int
main()
{
	BApplication app("application/x-vnd.haiku.layout-test1");

	BWindow* window = new TestWindow;
	window->Show();

	app.Run();

	return 0;
}
