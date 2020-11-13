/*
 * Copyright 2009-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_LAYOUT_BUILDER_H
#define	_LAYOUT_BUILDER_H


#include <new>

#include <CardLayout.h>
#include <CardView.h>
#include <GridLayout.h>
#include <GridView.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <SpaceLayoutItem.h>
#include <SplitView.h>
#include <TextControl.h>
#include <Window.h>


namespace BLayoutBuilder {

template<typename ParentBuilder> class Base;
template<typename ParentBuilder = void*> class Group;
template<typename ParentBuilder = void*> class Grid;
template<typename ParentBuilder = void*> class Split;
template<typename ParentBuilder = void*> class Cards;
template<typename ParentBuilder = void*> class Menu;
template<typename ParentBuilder = void*> class MenuItem;


template<typename ParentBuilder>
class Base {
protected:
	inline						Base();

public:
	inline	void				SetParent(ParentBuilder* parent);
		// conceptually private
	inline	ParentBuilder&		End();

protected:
			ParentBuilder*		fParent;
};


template<typename ParentBuilder>
class Group : public Base<ParentBuilder> {
public:
	typedef Group<ParentBuilder>	ThisBuilder;
	typedef Group<ThisBuilder>		GroupBuilder;
	typedef Grid<ThisBuilder>		GridBuilder;
	typedef Split<ThisBuilder>		SplitBuilder;
	typedef Cards<ThisBuilder>		CardBuilder;

public:
	inline						Group(orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
	inline						Group(BWindow* window,
									orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
	inline						Group(BView* view,
									orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
	inline						Group(BGroupLayout* layout);
	inline						Group(BGroupView* view);

	inline	BGroupLayout*		Layout() const;
	inline	BView*				View() const;
	inline	ThisBuilder&		GetLayout(BGroupLayout** _layout);
	inline	ThisBuilder&		GetView(BView** _view);

	inline	ThisBuilder&		Add(BView* view);
	inline	ThisBuilder&		Add(BView* view, float weight);
	inline	ThisBuilder&		Add(BLayoutItem* item);
	inline	ThisBuilder&		Add(BLayoutItem* item, float weight);

	inline	GroupBuilder		AddGroup(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	GroupBuilder		AddGroup(BGroupView* groupView,
									float weight = 1.0f);
	inline	GroupBuilder		AddGroup(BGroupLayout* groupLayout,
									float weight = 1.0f);

	inline	GridBuilder			AddGrid(float horizontal
										= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	GridBuilder			AddGrid(BGridLayout* gridLayout,
									float weight = 1.0f);
	inline	GridBuilder			AddGrid(BGridView* gridView,
									float weight = 1.0f);

	inline	SplitBuilder		AddSplit(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	SplitBuilder		AddSplit(BSplitView* splitView,
									float weight = 1.0f);

	inline	CardBuilder			AddCards(float weight = 1.0f);
	inline	CardBuilder			AddCards(BCardLayout* cardLayout,
									float weight = 1.0f);
	inline	CardBuilder			AddCards(BCardView* cardView,
									float weight = 1.0f);

	inline	ThisBuilder&		AddGlue(float weight = 1.0f);
	inline	ThisBuilder&		AddStrut(float size);

	inline	ThisBuilder&		SetInsets(float left, float top, float right,
									float bottom);
	inline	ThisBuilder&		SetInsets(float horizontal, float vertical);
	inline	ThisBuilder&		SetInsets(float insets);

	inline	ThisBuilder&		SetExplicitMinSize(BSize size);
	inline	ThisBuilder&		SetExplicitMaxSize(BSize size);
	inline	ThisBuilder&		SetExplicitPreferredSize(BSize size);
	inline	ThisBuilder&		SetExplicitAlignment(BAlignment alignment);

	inline						operator BGroupLayout*();

private:
			BGroupLayout*		fLayout;
};


template<typename ParentBuilder>
class Grid : public Base<ParentBuilder> {
public:
	typedef Grid<ParentBuilder>		ThisBuilder;
	typedef Group<ThisBuilder>		GroupBuilder;
	typedef Grid<ThisBuilder>		GridBuilder;
	typedef Split<ThisBuilder>		SplitBuilder;
	typedef Cards<ThisBuilder>		CardBuilder;

public:
	inline						Grid(float horizontal
										= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
	inline						Grid(BWindow* window,
									float horizontal = B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
	inline						Grid(BView* view,
									float horizontal = B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
	inline						Grid(BGridLayout* layout);
	inline						Grid(BGridView* view);

	inline	BGridLayout*		Layout() const;
	inline	BView*				View() const;
	inline	ThisBuilder&		GetLayout(BGridLayout** _layout);
	inline	ThisBuilder&		GetView(BView** _view);

	inline	ThisBuilder&		Add(BView* view, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	ThisBuilder&		Add(BLayoutItem* item, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	ThisBuilder&		AddMenuField(BMenuField* menuField,
									int32 column, int32 row,
									alignment labelAlignment
										= B_ALIGN_HORIZONTAL_UNSET,
									int32 labelColumnCount = 1,
									int32 fieldColumnCount = 1,
									int32 rowCount = 1);
	inline	ThisBuilder&		AddTextControl(BTextControl* textControl,
									int32 column, int32 row,
									alignment labelAlignment
										= B_ALIGN_HORIZONTAL_UNSET,
									int32 labelColumnCount = 1,
									int32 textColumnCount = 1,
									int32 rowCount = 1);

	inline	GroupBuilder		AddGroup(orientation orientation,
									float spacing, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	GroupBuilder		AddGroup(BGroupView* groupView,	int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);
	inline	GroupBuilder		AddGroup(BGroupLayout* groupLayout,
									int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);

	inline	GridBuilder			AddGrid(float horizontalSpacing,
									float verticalSpacing, int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);
	inline	GridBuilder			AddGrid(BGridLayout* gridLayout,
									int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	GridBuilder			AddGrid(BGridView* gridView,
									int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);

	inline	SplitBuilder		AddSplit(orientation orientation,
									float spacing, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	SplitBuilder		AddSplit(BSplitView* splitView, int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);

	inline	CardBuilder			AddCards(int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	CardBuilder			AddCards(BCardLayout* cardLayout, int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);
	inline	CardBuilder			AddCards(BCardView* cardView, int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);

	inline	ThisBuilder&		AddGlue(int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);

	inline	ThisBuilder&		SetHorizontalSpacing(float spacing);
	inline	ThisBuilder&		SetVerticalSpacing(float spacing);
	inline	ThisBuilder&		SetSpacing(float horizontal, float vertical);

	inline	ThisBuilder&		SetColumnWeight(int32 column, float weight);
	inline	ThisBuilder&		SetRowWeight(int32 row, float weight);

	inline	ThisBuilder&		SetInsets(float left, float top, float right,
									float bottom);
	inline	ThisBuilder&		SetInsets(float horizontal, float vertical);
	inline	ThisBuilder&		SetInsets(float insets);

	inline	ThisBuilder&		SetExplicitMinSize(BSize size);
	inline	ThisBuilder&		SetExplicitMaxSize(BSize size);
	inline	ThisBuilder&		SetExplicitPreferredSize(BSize size);
	inline	ThisBuilder&		SetExplicitAlignment(BAlignment alignment);

	inline						operator BGridLayout*();

private:
			BGridLayout*		fLayout;
};


template<typename ParentBuilder>
class Split : public Base<ParentBuilder> {
public:
	typedef Split<ParentBuilder>	ThisBuilder;
	typedef Group<ThisBuilder>		GroupBuilder;
	typedef Grid<ThisBuilder>		GridBuilder;
	typedef Split<ThisBuilder>		SplitBuilder;
	typedef Cards<ThisBuilder>		CardBuilder;

public:
	inline						Split(orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
	inline						Split(BSplitView* view);

	inline	BSplitView*			View() const;
	inline	ThisBuilder&		GetView(BView** _view);
	inline	ThisBuilder&		GetSplitView(BSplitView** _view);

	inline	ThisBuilder&		Add(BView* view);
	inline	ThisBuilder&		Add(BView* view, float weight);
	inline	ThisBuilder&		Add(BLayoutItem* item);
	inline	ThisBuilder&		Add(BLayoutItem* item, float weight);

	inline	GroupBuilder		AddGroup(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	GroupBuilder		AddGroup(BGroupView* groupView,
									float weight = 1.0f);
	inline	GroupBuilder		AddGroup(BGroupLayout* groupLayout,
									float weight = 1.0f);

	inline	GridBuilder			AddGrid(float horizontal
											= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	GridBuilder			AddGrid(BGridView* gridView,
									float weight = 1.0f);
	inline	GridBuilder			AddGrid(BGridLayout* gridLayout,
									float weight = 1.0f);

	inline	SplitBuilder		AddSplit(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	SplitBuilder		AddSplit(BSplitView* splitView,
									float weight = 1.0f);

	inline	CardBuilder			AddCards(float weight = 1.0f);
	inline	CardBuilder			AddCards(BCardLayout* cardLayout,
									float weight = 1.0f);
	inline	CardBuilder			AddCards(BCardView* cardView,
									float weight = 1.0f);

	inline	ThisBuilder&		SetCollapsible(bool collapsible);
	inline	ThisBuilder&		SetCollapsible(int32 index, bool collapsible);
	inline	ThisBuilder&		SetCollapsible(int32 first, int32 last,
									bool collapsible);

	inline	ThisBuilder&		SetInsets(float left, float top, float right,
									float bottom);
	inline	ThisBuilder&		SetInsets(float horizontal, float vertical);
	inline	ThisBuilder&		SetInsets(float insets);

	inline						operator BSplitView*();

private:
			BSplitView*			fView;
};

template<typename ParentBuilder>
class Cards : public Base<ParentBuilder> {
public:
	typedef Cards<ParentBuilder>	ThisBuilder;
	typedef Group<ThisBuilder>		GroupBuilder;
	typedef Grid<ThisBuilder>		GridBuilder;
	typedef Split<ThisBuilder>		SplitBuilder;
	typedef Cards<ThisBuilder>		CardBuilder;

public:
	inline						Cards();
	inline						Cards(BWindow* window);
	inline						Cards(BView* view);
	inline						Cards(BCardLayout* layout);
	inline						Cards(BCardView* view);

	inline	BCardLayout*		Layout() const;
	inline	BView*				View() const;
	inline	ThisBuilder&		GetLayout(BCardLayout** _layout);
	inline	ThisBuilder&		GetView(BView** _view);

	inline	ThisBuilder&		Add(BView* view);
	inline	ThisBuilder&		Add(BLayoutItem* item);

	inline	GroupBuilder		AddGroup(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING);
	inline	GroupBuilder		AddGroup(BGroupView* groupView);
	inline	GroupBuilder		AddGroup(BGroupLayout* groupLayout);

	inline	GridBuilder			AddGrid(float horizontal
										= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
	inline	GridBuilder			AddGrid(BGridLayout* gridLayout);
	inline	GridBuilder			AddGrid(BGridView* gridView);

	inline	SplitBuilder		AddSplit(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING);
	inline	SplitBuilder		AddSplit(BSplitView* splitView);

	inline	CardBuilder			AddCards();
	inline	CardBuilder			AddCards(BCardLayout* cardLayout);
	inline	CardBuilder			AddCards(BCardView* cardView);

	inline	ThisBuilder&		SetExplicitMinSize(BSize size);
	inline	ThisBuilder&		SetExplicitMaxSize(BSize size);
	inline	ThisBuilder&		SetExplicitPreferredSize(BSize size);
	inline	ThisBuilder&		SetExplicitAlignment(BAlignment alignment);

	inline	ThisBuilder&		SetVisibleItem(int32 index);

	inline						operator BCardLayout*();

private:
			BCardLayout*		fLayout;
};


template<typename ParentBuilder>
class Menu : public Base<ParentBuilder> {
public:
	typedef Menu<ParentBuilder>		ThisBuilder;
	typedef MenuItem<ParentBuilder>	ItemBuilder;
	typedef Menu<ThisBuilder>		MenuBuilder;

public:
	inline						Menu(BMenu* menu);

	inline	ThisBuilder&		GetMenu(BMenu*& _menu);

	inline	ItemBuilder			AddItem(BMenuItem* item);
	inline	ItemBuilder			AddItem(BMenu* menu);
	inline	ItemBuilder			AddItem(const char* label, BMessage* message,
									char shortcut = 0, uint32 modifiers = 0);
	inline	ItemBuilder			AddItem(const char* label, uint32 messageWhat,
									char shortcut = 0, uint32 modifiers = 0);

	inline	MenuBuilder			AddMenu(BMenu* menu);
	inline	MenuBuilder			AddMenu(const char* title,
									menu_layout layout = B_ITEMS_IN_COLUMN);

	inline	ThisBuilder&		AddSeparator();

private:
			BMenu*				fMenu;
};


template<typename ParentBuilder>
class MenuItem : public Menu<ParentBuilder> {
public:
	typedef MenuItem<ParentBuilder>	ThisBuilder;

public:
	inline						MenuItem(ParentBuilder* parentBuilder,
									BMenu* menu, BMenuItem* item);

	inline	ThisBuilder&		GetItem(BMenuItem*& _item);

	inline	ThisBuilder&		SetEnabled(bool enabled);

private:
			BMenuItem*			fMenuItem;
};


// #pragma mark - Base


template<typename ParentBuilder>
Base<ParentBuilder>::Base()
	:
	fParent(NULL)
{
}


template<typename ParentBuilder>
void
Base<ParentBuilder>::SetParent(ParentBuilder* parent)
{
	fParent = parent;
}


template<typename ParentBuilder>
ParentBuilder&
Base<ParentBuilder>::End()
{
	return *fParent;
}


// #pragma mark - Group


template<typename ParentBuilder>
Group<ParentBuilder>::Group(orientation orientation, float spacing)
	:
	fLayout((new BGroupView(orientation, spacing))->GroupLayout())
{
}


template<typename ParentBuilder>
Group<ParentBuilder>::Group(BWindow* window, orientation orientation,
		float spacing)
	:
	fLayout(new BGroupLayout(orientation, spacing))
{
	window->SetLayout(fLayout);
	fLayout->Owner()->AdoptSystemColors();
}


template<typename ParentBuilder>
Group<ParentBuilder>::Group(BView* view, orientation orientation,
		float spacing)
	:
	fLayout(new BGroupLayout(orientation, spacing))
{

	if (view->HasDefaultColors())
		view->AdoptSystemColors();

	view->SetLayout(fLayout);
}


template<typename ParentBuilder>
Group<ParentBuilder>::Group(BGroupLayout* layout)
	:
	fLayout(layout)
{
}


template<typename ParentBuilder>
Group<ParentBuilder>::Group(BGroupView* view)
	:
	fLayout(view->GroupLayout())
{
}


template<typename ParentBuilder>
BGroupLayout*
Group<ParentBuilder>::Layout() const
{
	return fLayout;
}


template<typename ParentBuilder>
BView*
Group<ParentBuilder>::View() const
{
	return fLayout->Owner();
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::GetLayout(BGroupLayout** _layout)
{
	*_layout = fLayout;
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::GetView(BView** _view)
{
	*_view = fLayout->Owner();
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::Add(BView* view)
{
	fLayout->AddView(view);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::Add(BView* view, float weight)
{
	fLayout->AddView(view, weight);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::Add(BLayoutItem* item)
{
	fLayout->AddItem(item);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::Add(BLayoutItem* item, float weight)
{
	fLayout->AddItem(item, weight);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::GroupBuilder
Group<ParentBuilder>::AddGroup(orientation orientation, float spacing,
		float weight)
{
	GroupBuilder builder(new BGroupLayout(orientation, spacing));
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::GroupBuilder
Group<ParentBuilder>::AddGroup(BGroupView* groupView, float weight)
{
	GroupBuilder builder(groupView);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::GroupBuilder
Group<ParentBuilder>::AddGroup(BGroupLayout* groupLayout, float weight)
{
	GroupBuilder builder(groupLayout);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::GridBuilder
Group<ParentBuilder>::AddGrid(float horizontalSpacing,
	float verticalSpacing, float weight)
{
	GridBuilder builder(new BGridLayout(horizontalSpacing, verticalSpacing));
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::GridBuilder
Group<ParentBuilder>::AddGrid(BGridLayout* gridLayout, float weight)
{
	GridBuilder builder(gridLayout);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::GridBuilder
Group<ParentBuilder>::AddGrid(BGridView* gridView, float weight)
{
	GridBuilder builder(gridView);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::SplitBuilder
Group<ParentBuilder>::AddSplit(orientation orientation, float spacing,
		float weight)
{
	SplitBuilder builder(orientation, spacing);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::SplitBuilder
Group<ParentBuilder>::AddSplit(BSplitView* splitView, float weight)
{
	SplitBuilder builder(splitView);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::CardBuilder
Group<ParentBuilder>::AddCards(float weight)
{
	CardBuilder builder;
	builder.SetParent(this);
	fLayout->AddView(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::CardBuilder
Group<ParentBuilder>::AddCards(BCardLayout* cardLayout, float weight)
{
	CardBuilder builder(cardLayout);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::CardBuilder
Group<ParentBuilder>::AddCards(BCardView* cardView, float weight)
{
	CardBuilder builder(cardView);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::AddGlue(float weight)
{
	fLayout->AddItem(BSpaceLayoutItem::CreateGlue(), weight);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::AddStrut(float size)
{
	if (fLayout->Orientation() == B_HORIZONTAL)
		fLayout->AddItem(BSpaceLayoutItem::CreateHorizontalStrut(size));
	else
		fLayout->AddItem(BSpaceLayoutItem::CreateVerticalStrut(size));

	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetInsets(float left, float top, float right,
	float bottom)
{
	fLayout->SetInsets(left, top, right, bottom);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetInsets(float horizontal, float vertical)
{
	fLayout->SetInsets(horizontal, vertical);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetInsets(float insets)
{
	fLayout->SetInsets(insets);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetExplicitMinSize(BSize size)
{
	fLayout->SetExplicitMinSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetExplicitMaxSize(BSize size)
{
	fLayout->SetExplicitMaxSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetExplicitPreferredSize(BSize size)
{
	fLayout->SetExplicitPreferredSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetExplicitAlignment(BAlignment alignment)
{
	fLayout->SetExplicitAlignment(alignment);
	return *this;
}


template<typename ParentBuilder>
Group<ParentBuilder>::operator BGroupLayout*()
{
	return fLayout;
}


// #pragma mark - Grid


template<typename ParentBuilder>
Grid<ParentBuilder>::Grid(float horizontalSpacing, float verticalSpacing)
	:
	fLayout((new BGridView(horizontalSpacing, verticalSpacing))->GridLayout())
{
}


template<typename ParentBuilder>
Grid<ParentBuilder>::Grid(BWindow* window, float horizontalSpacing,
	float verticalSpacing)
	:
	fLayout(new BGridLayout(horizontalSpacing, verticalSpacing))
{
	window->SetLayout(fLayout);
	fLayout->Owner()->AdoptSystemColors();
}


template<typename ParentBuilder>
Grid<ParentBuilder>::Grid(BView* view, float horizontalSpacing,
	float verticalSpacing)
	:
	fLayout(new BGridLayout(horizontalSpacing, verticalSpacing))
{
	if (view->HasDefaultColors())
		view->AdoptSystemColors();

	view->SetLayout(fLayout);
}


template<typename ParentBuilder>
Grid<ParentBuilder>::Grid(BGridLayout* layout)
	:
	fLayout(layout)
{
}


template<typename ParentBuilder>
Grid<ParentBuilder>::Grid(BGridView* view)
	:
	fLayout(view->GridLayout())
{
}


template<typename ParentBuilder>
BGridLayout*
Grid<ParentBuilder>::Layout() const
{
	return fLayout;
}


template<typename ParentBuilder>
BView*
Grid<ParentBuilder>::View() const
{
	return fLayout->Owner();
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::GetLayout(BGridLayout** _layout)
{
	*_layout = fLayout;
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::GetView(BView** _view)
{
	*_view = fLayout->Owner();
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::Add(BView* view, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	fLayout->AddView(view, column, row, columnCount, rowCount);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::Add(BLayoutItem* item, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	fLayout->AddItem(item, column, row, columnCount, rowCount);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::AddMenuField(BMenuField* menuField, int32 column,
	int32 row, alignment labelAlignment, int32 labelColumnCount,
	int32 fieldColumnCount, int32 rowCount)
{
	BLayoutItem* item = menuField->CreateLabelLayoutItem();
	item->SetExplicitAlignment(
		BAlignment(labelAlignment, B_ALIGN_VERTICAL_UNSET));
	fLayout->AddItem(item, column, row, labelColumnCount, rowCount);
	fLayout->AddItem(menuField->CreateMenuBarLayoutItem(),
		column + labelColumnCount, row, fieldColumnCount, rowCount);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::AddTextControl(BTextControl* textControl, int32 column,
	int32 row, alignment labelAlignment, int32 labelColumnCount,
	int32 textColumnCount, int32 rowCount)
{
	BLayoutItem* item = textControl->CreateLabelLayoutItem();
	item->SetExplicitAlignment(
		BAlignment(labelAlignment, B_ALIGN_VERTICAL_UNSET));
	fLayout->AddItem(item, column, row, labelColumnCount, rowCount);
	fLayout->AddItem(textControl->CreateTextViewLayoutItem(),
		column + labelColumnCount, row, textColumnCount, rowCount);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::GroupBuilder
Grid<ParentBuilder>::AddGroup(orientation orientation, float spacing,
		int32 column, int32 row, int32 columnCount, int32 rowCount)
{
	GroupBuilder builder(new BGroupLayout(orientation, spacing));
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::GroupBuilder
Grid<ParentBuilder>::AddGroup(BGroupView* groupView, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	GroupBuilder builder(groupView);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::GroupBuilder
Grid<ParentBuilder>::AddGroup(BGroupLayout* groupLayout, int32 column,
	int32 row, int32 columnCount, int32 rowCount)
{
	GroupBuilder builder(groupLayout);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::GridBuilder
Grid<ParentBuilder>::AddGrid(float horizontalSpacing, float verticalSpacing,
	int32 column, int32 row, int32 columnCount, int32 rowCount)
{
	GridBuilder builder(new BGridLayout(horizontalSpacing, verticalSpacing));
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::GridBuilder
Grid<ParentBuilder>::AddGrid(BGridView* gridView, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	GridBuilder builder(gridView);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::SplitBuilder
Grid<ParentBuilder>::AddSplit(orientation orientation, float spacing,
	int32 column, int32 row, int32 columnCount, int32 rowCount)
{
	SplitBuilder builder(orientation, spacing);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::SplitBuilder
Grid<ParentBuilder>::AddSplit(BSplitView* splitView, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	SplitBuilder builder(splitView);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::CardBuilder
Grid<ParentBuilder>::AddCards(int32 column, int32 row, int32 columnCount,
	int32 rowCount)
{
	CardBuilder builder;
	builder.SetParent(this);
	fLayout->AddView(builder.View(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::CardBuilder
Grid<ParentBuilder>::AddCards(BCardLayout* cardLayout, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	CardBuilder builder(cardLayout);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::CardBuilder
Grid<ParentBuilder>::AddCards(BCardView* cardView, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	CardBuilder builder(cardView);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::AddGlue(int32 column, int32 row, int32 columnCount,
	int32 rowCount)
{
	fLayout->AddItem(BSpaceLayoutItem::CreateGlue(), column, row, columnCount,
		rowCount);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetHorizontalSpacing(float spacing)
{
	fLayout->SetHorizontalSpacing(spacing);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetVerticalSpacing(float spacing)
{
	fLayout->SetVerticalSpacing(spacing);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetSpacing(float horizontal, float vertical)
{
	fLayout->SetSpacing(horizontal, vertical);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetColumnWeight(int32 column, float weight)
{
	fLayout->SetColumnWeight(column, weight);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetRowWeight(int32 row, float weight)
{
	fLayout->SetRowWeight(row, weight);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetInsets(float left, float top, float right,
	float bottom)
{
	fLayout->SetInsets(left, top, right, bottom);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetInsets(float horizontal, float vertical)
{
	fLayout->SetInsets(horizontal, vertical);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetInsets(float insets)
{
	fLayout->SetInsets(insets);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetExplicitMinSize(BSize size)
{
	fLayout->SetExplicitMinSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetExplicitMaxSize(BSize size)
{
	fLayout->SetExplicitMaxSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetExplicitPreferredSize(BSize size)
{
	fLayout->SetExplicitPreferredSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetExplicitAlignment(BAlignment alignment)
{
	fLayout->SetExplicitAlignment(alignment);
	return *this;
}


template<typename ParentBuilder>
Grid<ParentBuilder>::operator BGridLayout*()
{
	return fLayout;
}


// #pragma mark - Split


template<typename ParentBuilder>
Split<ParentBuilder>::Split(orientation orientation, float spacing)
	:
	fView(new BSplitView(orientation, spacing))
{
}


template<typename ParentBuilder>
Split<ParentBuilder>::Split(BSplitView* view)
	:
	fView(view)
{
}


template<typename ParentBuilder>
BSplitView*
Split<ParentBuilder>::View() const
{
	return fView;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::GetView(BView** _view)
{
	*_view = fView;
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::GetSplitView(BSplitView** _view)
{
	*_view = fView;
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::Add(BView* view)
{
	fView->AddChild(view);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::Add(BView* view, float weight)
{
	fView->AddChild(view, weight);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::Add(BLayoutItem* item)
{
	fView->AddChild(item);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::Add(BLayoutItem* item, float weight)
{
	fView->AddChild(item, weight);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::GroupBuilder
Split<ParentBuilder>::AddGroup(orientation orientation, float spacing,
		float weight)
{
	GroupBuilder builder(new BGroupLayout(orientation, spacing));
	builder.SetParent(this);
	fView->AddChild(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::GroupBuilder
Split<ParentBuilder>::AddGroup(BGroupView* groupView, float weight)
{
	GroupBuilder builder(groupView);
	builder.SetParent(this);
	fView->AddChild(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::GroupBuilder
Split<ParentBuilder>::AddGroup(BGroupLayout* groupLayout, float weight)
{
	GroupBuilder builder(groupLayout);
	builder.SetParent(this);
	fView->AddChild(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::GridBuilder
Split<ParentBuilder>::AddGrid(float horizontalSpacing, float verticalSpacing,
	float weight)
{
	GridBuilder builder(new BGridLayout(horizontalSpacing, verticalSpacing));
	builder.SetParent(this);
	fView->AddChild(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::GridBuilder
Split<ParentBuilder>::AddGrid(BGridView* gridView, float weight)
{
	GridBuilder builder(gridView);
	builder.SetParent(this);
	fView->AddChild(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::GridBuilder
Split<ParentBuilder>::AddGrid(BGridLayout* layout, float weight)
{
	GridBuilder builder(layout);
	builder.SetParent(this);
	fView->AddChild(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::SplitBuilder
Split<ParentBuilder>::AddSplit(orientation orientation, float spacing,
		float weight)
{
	SplitBuilder builder(orientation, spacing);
	builder.SetParent(this);
	fView->AddChild(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::CardBuilder
Split<ParentBuilder>::AddCards(float weight)
{
	CardBuilder builder;
	builder.SetParent(this);
	fView->AddChild(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::CardBuilder
Split<ParentBuilder>::AddCards(BCardLayout* cardLayout, float weight)
{
	CardBuilder builder(cardLayout);
	builder.SetParent(this);
	fView->AddChild(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::CardBuilder
Split<ParentBuilder>::AddCards(BCardView* cardView, float weight)
{
	CardBuilder builder(cardView);
	builder.SetParent(this);
	fView->AddChild(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::SetCollapsible(bool collapsible)
{
	fView->SetCollapsible(collapsible);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::SetCollapsible(int32 index, bool collapsible)
{
	fView->SetCollapsible(index, collapsible);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::SetCollapsible(int32 first, int32 last, bool collapsible)
{
	fView->SetCollapsible(first, last, collapsible);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::SetInsets(float left, float top, float right,
	float bottom)
{
	fView->SetInsets(left, top, right, bottom);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::SetInsets(float horizontal, float vertical)
{
	fView->SetInsets(horizontal, vertical);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::SetInsets(float insets)
{
	fView->SetInsets(insets);
	return *this;
}


template<typename ParentBuilder>
Split<ParentBuilder>::operator BSplitView*()
{
	return fView;
}


// #pragma mark - Cards


template<typename ParentBuilder>
Cards<ParentBuilder>::Cards()
	:
	fLayout((new BCardView())->CardLayout())
{
}


template<typename ParentBuilder>
Cards<ParentBuilder>::Cards(BWindow* window)
	:
	fLayout(new BCardLayout())
{
	window->SetLayout(fLayout);

	fLayout->Owner()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


template<typename ParentBuilder>
Cards<ParentBuilder>::Cards(BView* view)
	:
	fLayout(new BCardLayout())
{
	view->SetLayout(fLayout);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


template<typename ParentBuilder>
Cards<ParentBuilder>::Cards(BCardLayout* layout)
	:
	fLayout(layout)
{
}


template<typename ParentBuilder>
Cards<ParentBuilder>::Cards(BCardView* view)
	:
	fLayout(view->CardLayout())
{
}


template<typename ParentBuilder>
BCardLayout*
Cards<ParentBuilder>::Layout() const
{
	return fLayout;
}


template<typename ParentBuilder>
BView*
Cards<ParentBuilder>::View() const
{
	return fLayout->Owner();
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::GetLayout(BCardLayout** _layout)
{
	*_layout = fLayout;
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::GetView(BView** _view)
{
	*_view = fLayout->Owner();
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::Add(BView* view)
{
	fLayout->AddView(view);
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::Add(BLayoutItem* item)
{
	fLayout->AddItem(item);
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::GroupBuilder
Cards<ParentBuilder>::AddGroup(orientation orientation, float spacing)
{
	GroupBuilder builder(new BGroupLayout(orientation, spacing));
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::GroupBuilder
Cards<ParentBuilder>::AddGroup(BGroupView* groupView)
{
	GroupBuilder builder(groupView);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::GroupBuilder
Cards<ParentBuilder>::AddGroup(BGroupLayout* groupLayout)
{
	GroupBuilder builder(groupLayout);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::GridBuilder
Cards<ParentBuilder>::AddGrid(float horizontal, float vertical)
{
	GridBuilder builder(horizontal, vertical);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::GridBuilder
Cards<ParentBuilder>::AddGrid(BGridLayout* gridLayout)
{
	GridBuilder builder(gridLayout);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::GridBuilder
Cards<ParentBuilder>::AddGrid(BGridView* gridView)
{
	GridBuilder builder(gridView);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::SplitBuilder
Cards<ParentBuilder>::AddSplit(orientation orientation, float spacing)
{
	SplitBuilder builder(orientation, spacing);
	builder.SetParent(this);
	fLayout->AddView(builder.View());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::SplitBuilder
Cards<ParentBuilder>::AddSplit(BSplitView* splitView)
{
	SplitBuilder builder(splitView);
	builder.SetParent(this);
	fLayout->AddView(builder.View());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::CardBuilder
Cards<ParentBuilder>::AddCards()
{
	CardBuilder builder;
	builder.SetParent(this);
	fLayout->AddView(builder.View());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::CardBuilder
Cards<ParentBuilder>::AddCards(BCardLayout* cardLayout)
{
	CardBuilder builder(cardLayout);
	builder.SetParent(this);
	fLayout->AddView(builder.View());
	return builder;
}

template<typename ParentBuilder>
typename Cards<ParentBuilder>::CardBuilder
Cards<ParentBuilder>::AddCards(BCardView* cardView)
{
	CardBuilder builder(cardView);
	builder.SetParent(this);
	fLayout->AddView(builder.View());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::SetExplicitMinSize(BSize size)
{
	fLayout->SetExplicitMinSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::SetExplicitMaxSize(BSize size)
{
	fLayout->SetExplicitMaxSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::SetExplicitPreferredSize(BSize size)
{
	fLayout->SetExplicitPreferredSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::SetExplicitAlignment(BAlignment alignment)
{
	fLayout->SetExplicitAlignment(alignment);
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::SetVisibleItem(int32 item)
{
	fLayout->SetVisibleItem(item);
	return *this;
}


template<typename ParentBuilder>
Cards<ParentBuilder>::operator BCardLayout*()
{
	return fLayout;
}


// #pragma mark - Menu


template<typename ParentBuilder>
Menu<ParentBuilder>::Menu(BMenu* menu)
	:
	fMenu(menu)
{
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::ThisBuilder&
Menu<ParentBuilder>::GetMenu(BMenu*& _menu)
{
	_menu = fMenu;
	return *this;
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::ItemBuilder
Menu<ParentBuilder>::AddItem(BMenuItem* item)
{
	fMenu->AddItem(item);
	return MenuItem<ParentBuilder>(this->fParent, fMenu, item);
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::ItemBuilder
Menu<ParentBuilder>::AddItem(BMenu* menu)
{
	if (!fMenu->AddItem(menu))
		throw std::bad_alloc();

	return MenuItem<ParentBuilder>(this->fParent, fMenu,
		fMenu->ItemAt(fMenu->CountItems() - 1));
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::ItemBuilder
Menu<ParentBuilder>::AddItem(const char* label, BMessage* message,
	char shortcut, uint32 modifiers)
{
	BMenuItem* item = new BMenuItem(label, message, shortcut, modifiers);
	if (!fMenu->AddItem(item)) {
		delete item;
		item = NULL;
	}

	return MenuItem<ParentBuilder>(this->fParent, fMenu, item);
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::ItemBuilder
Menu<ParentBuilder>::AddItem(const char* label, uint32 messageWhat,
	char shortcut, uint32 modifiers)
{
	BMessage* message = new BMessage(messageWhat);
	BMenuItem* item;
	try {
		item = new BMenuItem(label, message, shortcut, modifiers);
	} catch (...) {
		delete message;
		throw;
	}

	if (!fMenu->AddItem(item)) {
		delete item;
		item = NULL;
	}

	return MenuItem<ParentBuilder>(this->fParent, fMenu, item);
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::ThisBuilder&
Menu<ParentBuilder>::AddSeparator()
{
	fMenu->AddSeparatorItem();
	return *this;
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::MenuBuilder
Menu<ParentBuilder>::AddMenu(BMenu* menu)
{
	if (!fMenu->AddItem(menu))
		throw std::bad_alloc();

	MenuBuilder builder(menu);
	builder.SetParent(this);
	return builder;
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::MenuBuilder
Menu<ParentBuilder>::AddMenu(const char* title, menu_layout layout)
{
	BMenu* menu = new BMenu(title, layout);
	if (!fMenu->AddItem(menu)) {
		delete menu;
		throw std::bad_alloc();
	}

	MenuBuilder builder(menu);
	builder.SetParent(this);
	return builder;
}


// #pragma mark - MenuItem


template<typename ParentBuilder>
MenuItem<ParentBuilder>::MenuItem(ParentBuilder* parentBuilder, BMenu* menu,
	BMenuItem* item)
	:
	Menu<ParentBuilder>(menu),
	fMenuItem(item)
{
	this->SetParent(parentBuilder);
}


template<typename ParentBuilder>
typename MenuItem<ParentBuilder>::ThisBuilder&
MenuItem<ParentBuilder>::GetItem(BMenuItem*& _item)
{
	_item = fMenuItem;
	return *this;
}


template<typename ParentBuilder>
typename MenuItem<ParentBuilder>::ThisBuilder&
MenuItem<ParentBuilder>::SetEnabled(bool enabled)
{
	fMenuItem->SetEnabled(enabled);
	return *this;
}


}	// namespace BLayoutBuilder


#endif	// _LAYOUT_BUILDER_H
