/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2009, Stephan Aßmus, superstippi@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef HEADER_VIEW_H
#define HEADER_VIEW_H


#include <View.h>

#include <ObjectList.h>
#include <Referenceable.h>
#include <Variant.h>


class Header;
class HeaderModel;
class HeaderViewListener;


class HeaderRenderer {
public:
	virtual						~HeaderRenderer();

	virtual	float				HeaderHeight(BView* view, const Header* header)
									= 0;
	virtual	float				PreferredHeaderWidth(BView* view,
									const Header* header) = 0;
	virtual	void				DrawHeader(BView* view, BRect frame,
									BRect updateRect, const Header* header,
									uint32 flags) = 0;
	virtual	void				DrawHeaderBackground(BView* view, BRect frame,
									BRect updateRect, uint32 flags);
};


class DefaultHeaderRenderer : public HeaderRenderer {
public:
								DefaultHeaderRenderer();
	virtual						~DefaultHeaderRenderer();

	virtual	float				HeaderHeight(BView* view, const Header* header);
	virtual	float				PreferredHeaderWidth(BView* view,
									const Header* header);
	virtual	void				DrawHeader(BView* view, BRect frame,
									BRect updateRect, const Header* header,
									uint32 flags);
};


class HeaderListener {
public:
	virtual						~HeaderListener();

	virtual	void				HeaderWidthChanged(Header* header);
	virtual	void				HeaderWidthRestrictionsChanged(Header* header);
	virtual	void				HeaderValueChanged(Header* header);
	virtual	void				HeaderRendererChanged(Header* header);
};


class Header {
public:
								Header(int32 modelIndex = 0);
								Header(float width, float minWidth,
									float maxWidth, float preferredWidth,
									int32 modelIndex = 0);

			float				Width() const;
			float				MinWidth() const;
			float				MaxWidth() const;
			float				PreferredWidth() const;

			void				SetWidth(float width);
			void				SetMinWidth(float width);
			void				SetMaxWidth(float width);
			void				SetPreferredWidth(float width);

			bool				IsResizable() const;
			void				SetResizable(bool resizable);

			bool				GetValue(BVariant& _value) const;
			void				SetValue(const BVariant& value);

			int32				ModelIndex() const;
			void				SetModelIndex(int32 index);

			HeaderRenderer*		GetHeaderRenderer() const;
			void				SetHeaderRenderer(HeaderRenderer* renderer);

			void				AddListener(HeaderListener* listener);
			void				RemoveListener(HeaderListener* listener);

protected:
			void				NotifyWidthChanged();
			void				NotifyWidthRestrictionsChanged();
			void				NotifyValueChanged();
			void				NotifyRendererChanged();

private:
			typedef BObjectList<HeaderListener> ListenerList;

private:
			float				fWidth;
			float				fMinWidth;
			float				fMaxWidth;
			float				fPreferredWidth;
			BVariant			fValue;
			HeaderRenderer*		fRenderer;
			int32				fModelIndex;
			bool				fResizable;
			ListenerList		fListeners;
};


class HeaderModelListener {
public:
	virtual						~HeaderModelListener();

	virtual	void				HeaderAdded(HeaderModel* model, int32 index);
	virtual	void				HeaderRemoved(HeaderModel* model, int32 index);
	virtual	void				HeaderMoved(HeaderModel* model,
									int32 fromIndex, int32 toIndex);
};


class HeaderModel : public BReferenceable {
public:
								HeaderModel();
	virtual						~HeaderModel();

	virtual	int32				CountHeaders() const;
	virtual	Header*				HeaderAt(int32 index) const;
	virtual	int32				IndexOfHeader(Header* header) const;

	virtual	bool				AddHeader(Header* header);
	virtual	Header*				RemoveHeader(int32 index);
	virtual	void				RemoveHeader(Header* header);
	virtual	bool				MoveHeader(int32 fromIndex, int32 toIndex);

	virtual	void				AddListener(HeaderModelListener* listener);
	virtual	void				RemoveListener(HeaderModelListener* listener);

protected:
			void				NotifyHeaderAdded(int32 index);
			void				NotifyHeaderRemoved(int32 index);
			void				NotifyHeaderMoved(int32 fromIndex,
									int32 toIndex);

private:
			typedef BObjectList<Header> HeaderList;
			typedef BObjectList<HeaderModelListener> ListenerList;

private:
			HeaderList			fHeaders;
			ListenerList		fListeners;
};


class HeaderView : public BView, private HeaderModelListener,
	private HeaderListener {
public:
								HeaderView();
	virtual						~HeaderView();

	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

			HeaderModel*		Model() const;
			status_t			SetModel(HeaderModel* model);

			BRect				HeaderFrame(int32 index) const;
			int32				HeaderIndexAt(BPoint point) const;

			void				AddListener(HeaderViewListener* listener);
			void				RemoveListener(HeaderViewListener* listener);

private:
			struct HeaderEntry;

			typedef BObjectList<HeaderEntry> HeaderEntryList;
			typedef BObjectList<HeaderViewListener> ListenerList;

			class State;
			class DefaultState;
			class ResizeState;

			friend class DefaultState;
			friend class ResizeState;

private:
	// HeaderModelListener
	virtual	void				HeaderAdded(HeaderModel* model, int32 index);
	virtual	void				HeaderRemoved(HeaderModel* model, int32 index);
	virtual	void				HeaderMoved(HeaderModel* model,
									int32 fromIndex, int32 toIndex);

	// HeaderListener
	virtual	void				HeaderWidthChanged(Header* header);
	virtual	void				HeaderWidthRestrictionsChanged(Header* header);
	virtual	void				HeaderValueChanged(Header* header);
	virtual	void				HeaderRendererChanged(Header* header);

			void				_HeaderPropertiesChanged(Header* header,
									bool redrawNeeded, bool relayoutNeeded);
			void				_InvalidateHeadersLayout(int32 firstIndex);
			void				_InvalidateHeaders(int32 firstIndex,
									int32 endIndex);
			void				_ValidateHeadersLayout();

			void				_SwitchState(State* newState);

private:
			HeaderModel*		fModel;
			HeaderEntryList		fHeaderEntries;
			float				fPreferredWidth;
			float				fPreferredHeight;
			bool				fLayoutValid;
			ListenerList		fListeners;
			State*				fDefaultState;
			State*				fState;
};


class HeaderViewListener {
public:
	virtual						~HeaderViewListener();
};


#endif	// HEADER_VIEW_H
