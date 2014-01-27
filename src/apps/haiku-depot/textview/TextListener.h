/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_LISTENER_H
#define TEXT_LISTENER_H


#include <Referenceable.h>


class TextChangeEvent {
protected:
								TextChangeEvent();
								TextChangeEvent(int32 firstChangedParagraph,
									int32 changedParagraphCount);
	virtual						~TextChangeEvent();

public:
			int32				FirstChangedParagraph() const
									{ return fFirstChangedParagraph; }
			int32				ChangedParagraphCount() const
									{ return fChangedParagraphCount; }

private:
			int32				fFirstChangedParagraph;
			int32				fChangedParagraphCount;
};


class TextChangingEvent : public TextChangeEvent {
public:
								TextChangingEvent(int32 firstChangedParagraph,
									int32 changedParagraphCount);
	virtual						~TextChangingEvent();

			void				Cancel();
			bool				IsCanceled() const
									{ return fIsCanceled; }

private:
								TextChangingEvent();

private:
			bool				fIsCanceled;
};


class TextChangedEvent : public TextChangeEvent {
public:
								TextChangedEvent(int32 firstChangedParagraph,
									int32 changedParagraphCount);
	virtual						~TextChangedEvent();

private:
								TextChangedEvent();
};


class TextListener : public BReferenceable {
public:
								TextListener();
	virtual						~TextListener();

	virtual	void				TextChanging(TextChangingEvent& event);

	virtual	void				TextChanged(const TextChangedEvent& event);
};


typedef BReference<TextListener> TextListenerRef;


#endif // TEXT_LISTENER_H
