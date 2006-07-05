/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef PROPERTY_EDITOR_FACTORY
#define PROPERTY_EDITOR_FACTORY

#include <SupportDefs.h>

class Property;
class PropertyEditorView;

PropertyEditorView*	EditorFor(Property* property);

#endif // PROPERTY_EDITOR_FACTORY
