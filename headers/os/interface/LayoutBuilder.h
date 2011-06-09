/*
 * Copyright 2009-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_LAYOUT_BUILDER_H
#define	_LAYOUT_BUILDER_H


#include <new>

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

public:
	inline						Group(enum orientation orientation
										= B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
	inline						Group(BWindow* window,
									enum orientation orientation = B_HORIZONTAL,
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

	inline	GroupBuilder		AddGroup(enum orientation orientation,
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

	inline	SplitBuilder		AddSplit(enum orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	SplitBuilder		AddSplit(BSplitView* splitView,
									float weight = 1.0f);

	inline	ThisBuilder&		AddGlue(float weight = 1.0f);
	inline	ThisBuilder&		AddStrut(float size);

	inline	ThisBuilder&		SetInsets(float left, float top, float right,
									float bottom);
	inline	ThisBuilder&		SetInsets(float horizontal, float vertical);
	inline	ThisBuilder&		SetInsets(float insets);

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

public:
	inline						Grid(float horizontal
										= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
	inline						Grid(BWindow* window,
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

	inline	GroupBuilder		AddGroup(enum orientation orientation,
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

	inline	SplitBuilder		AddSplit(enum orientation orientation,
									float spacing, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	SplitBuilder		AddSplit(BSplitView* splitView, int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);

	inline	ThisBuilder&		SetColumnWeight(int32 column, float weight);
	inline	ThisBuilder&		SetRowWeight(int32 row, float weight);

	inline	ThisBuilder&		SetInsets(float left, float top, float right,
									float bottom);
	inline	ThisBuilder&		SetInsets(float horizontal, float vertical);
	inline	ThisBuilder&		SetInsets(float insets);

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

public:
	inline						Split(enum orientation orientation
										= B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
	inline						Split(BSplitView* view);

	inline	BSplitView*			View() const;
	inline	ThisBuilder&		GetView(BView** _view);
	inline	ThisBuilder&		GetSplitView(BSplitView** _view);

	inline	ThisBuilder&		Add(BView* view);
	inline	ThisBuilder&		Add(BView* view, float weight);
	inline	ThisBuilder&		Add(BLayoutItem* item);
	inline	ThisBuilder&		Add(BLayoutItem* item, float weight);

	inline	GroupBuilder		AddGroup(enum orientation orientation,
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

	inline	SplitBuilder		AddSplit(enum orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	SplitBuilder		AddSplit(BSplitView* splitView,
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
Group<ParentBuilder>::Group(enum orientation orientation, float spacing)
	:
	fLayout((new BGroupView(orientation, spacing))->GroupLayout())
{
}


template<typename ParentBuilder>
Group<ParentBuilder>::Group(BWindow* window, enum orientation orientation,
	float spacing)
	:
	fLayout(new BGroupLayout(orientation, spacing))
{
	window->SetLayout(fLayout);

	fLayout->Owner()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		// TODO: we get a white background if we don't do this
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
Group<ParentBuilder>::AddGroup(enum orientation orientation, float spacing,
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
Group<ParentBuilder>::AddSplit(enum orientation orientation, float spacing,
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

	fLayout->Owner()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		// TODO: we get a white background if we don't do this
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
Grid<ParentBuilder>::AddGroup(enum orientation orientation, float spacing,
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
Grid<ParentBuilder>::AddSplit(enum orientation orientation, float spacing,
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
Grid<ParentBuilder>::operator BGridLayout*()
{
	return fLayout;
}


// #pragma mark - Split


template<typename ParentBuilder>
Split<ParentBuilder>::Split(enum orientation orientation, float spacing)
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
Split<ParentBuilder>::AddGroup(enum orientation orientation, float spacing,
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
Split<ParentBuilder>::AddSplit(enum orientation orientation, float spacing,
	float weight)
{
	SplitBuilder builder(orientation, spacing);
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
	if (!fMenu->AddItem(item))
		delete item;

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

	if (!fMenu->AddItem(item))
		delete item;

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
	SetParent(parentBuilder);
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
