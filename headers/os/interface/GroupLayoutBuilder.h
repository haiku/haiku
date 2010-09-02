/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_GROUP_LAYOUT_BUILDER_H
#define	_GROUP_LAYOUT_BUILDER_H

#include <GroupLayout.h>
#include <GroupView.h>
#include <List.h>

class BGroupLayoutBuilder {
public:
								BGroupLayoutBuilder(
									enum orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
								BGroupLayoutBuilder(BGroupLayout* layout);
								BGroupLayoutBuilder(BGroupView* view);

			BGroupLayout*		RootLayout() const;
			BGroupLayout*		TopLayout() const;
			BGroupLayoutBuilder& GetTopLayout(BGroupLayout** _layout);
			BView*				TopView() const;
			BGroupLayoutBuilder& GetTopView(BView** _view);

			BGroupLayoutBuilder& Add(BView* view);
			BGroupLayoutBuilder& Add(BView* view, float weight);
			BGroupLayoutBuilder& Add(BLayoutItem* item);
			BGroupLayoutBuilder& Add(BLayoutItem* item, float weight);

			BGroupLayoutBuilder& AddGroup(enum orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
			BGroupLayoutBuilder& End();

			BGroupLayoutBuilder& AddGlue(float weight = 1.0f);
			BGroupLayoutBuilder& AddStrut(float size);

			BGroupLayoutBuilder& SetInsets(float left, float top, float right,
									float bottom);

								operator BGroupLayout*();

private:
			bool				_PushLayout(BGroupLayout* layout);
			void				_PopLayout();

private:
			BGroupLayout*		fRootLayout;
			BList				fLayoutStack;
};

#endif	// _GROUP_LAYOUT_BUILDER_H
