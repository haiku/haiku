/*
 * Copyright 2006, Haiku Inc.
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
									float spacing = 0.0f);
								BGroupLayoutBuilder(BGroupLayout* layout);
								BGroupLayoutBuilder(BGroupView* view);

			BGroupLayout*		RootLayout() const;
			BGroupLayout*		TopLayout() const;
			BGroupLayoutBuilder& GetTopLayout(BGroupLayout** _layout);
			BGroupLayoutBuilder& GetTopView(BView** _view);

			BGroupLayoutBuilder& Add(BView* view);
			BGroupLayoutBuilder& Add(BView* view, float weight);
			BGroupLayoutBuilder& Add(BLayoutItem* item);
			BGroupLayoutBuilder& Add(BLayoutItem* item, float weight);

			BGroupLayoutBuilder& AddGroup(enum orientation orientation,
									float spacing = 0.0f, float weight = 1.0f);
			BGroupLayoutBuilder& End();

			BGroupLayoutBuilder& AddGlue(float weight = 1.0f);
			BGroupLayoutBuilder& AddStrut(float size);

			BGroupLayoutBuilder& SetInsets(float left, float top, float right,
									float bottom);

								operator BGroupLayout*();
								operator BView*();

private:
			bool				_PushLayout(BGroupLayout* layout);
			void				_PopLayout();

private:
			BGroupLayout*		fRootLayout;
			BList				fLayoutStack;
};

#endif	// _GROUP_LAYOUT_BUILDER_H
