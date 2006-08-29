/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef PROPERTY_H
#define PROPERTY_H

#include <limits.h>

#include <Archivable.h>
#include <String.h>
#include <TypeConstants.h>

class PropertyAnimator;

// Property
class Property : public BArchivable {
 public:
								Property(uint32 identifier);
								Property(const Property& other);
								Property(BMessage* archive);
	virtual						~Property();

	// BArchivable interface
	virtual	status_t			Archive(BMessage* archive,
										bool deep = true) const;

	// Property
	virtual	Property*			Clone() const = 0;

	virtual	type_code			Type() const = 0;

	virtual	bool				SetValue(const char* value) = 0;
	virtual	bool				SetValue(const Property* other) = 0;
	virtual	void				GetValue(BString& string) = 0;

	// animation
	virtual	bool				InterpolateTo(const Property* other,
											  float scale);

	inline	uint32				Identifier() const
									{ return fIdentifier; }

			void				SetEditable(bool editable);
	inline	bool				IsEditable() const
									{ return fEditable; }

 private:
			uint32				fIdentifier;
			bool				fEditable;
};

// IntProperty
class IntProperty : public Property {
 public:
								IntProperty(uint32 identifier,
											int32 value = 0,
											int32 min = LONG_MIN,
											int32 max = LONG_MAX);
								IntProperty(const IntProperty& other);
								IntProperty(BMessage* archive);
	virtual						~IntProperty();

	virtual	status_t			Archive(BMessage* archive,
										bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

	virtual	Property*			Clone() const;

	virtual	type_code			Type() const
									{ return B_INT32_TYPE; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				InterpolateTo(const Property* other,
											  float scale);

	// IntProperty
			bool				SetValue(int32 value);

	inline	int32				Value() const
									{ return fValue; }
	inline	int32				Min() const
									{ return fMin; }
	inline	int32				Max() const
									{ return fMax; }

 private:
			int32				fValue;
			int32				fMin;
			int32				fMax;
};

// FloatProperty
class FloatProperty : public Property {
 public:
								FloatProperty(uint32 identifier,
											  float value = 0.0,
											  float min = -1000000.0,
											  float max = 1000000.0);
								FloatProperty(const FloatProperty& other);
								FloatProperty(BMessage* archive);
	virtual						~FloatProperty();

	virtual	status_t			Archive(BMessage* archive,
										bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

	virtual	Property*			Clone() const;

	virtual	type_code			Type() const
									{ return B_FLOAT_TYPE; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				InterpolateTo(const Property* other,
											  float scale);

	// FloatProperty
			bool				SetValue(float value);

	inline	float				Value() const
									{ return fValue; }
	inline	float				Min() const
									{ return fMin; }
	inline	float				Max() const
									{ return fMax; }

 private:
			float				fValue;
			float				fMin;
			float				fMax;
};

// UInt8Property
class UInt8Property : public Property {
 public:
								UInt8Property(uint32 identifier,
											  uint8 value = 255);
								UInt8Property(const UInt8Property& other);
								UInt8Property(BMessage* archive);
	virtual						~UInt8Property();

	virtual	status_t			Archive(BMessage* archive,
										bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

	virtual	Property*			Clone() const;

	virtual	type_code			Type() const
									{ return B_INT8_TYPE; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				InterpolateTo(const Property* other,
											  float scale);

	// UInt8Property
			bool				SetValue(uint8 value);

	inline	uint8				Value() const
									{ return fValue; }

 private:
			uint8				fValue;
};

// BoolProperty
class BoolProperty : public Property {
 public:
								BoolProperty(uint32 identifier,
											 bool value = false);
								BoolProperty(const BoolProperty& other);
								BoolProperty(BMessage* archive);
	virtual						~BoolProperty();

	virtual	status_t			Archive(BMessage* archive,
										bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

	virtual	Property*			Clone() const;

	virtual	type_code			Type() const
									{ return B_BOOL_TYPE; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				InterpolateTo(const Property* other,
											  float scale);

	// BoolProperty
			bool				SetValue(bool value);

	inline	bool				Value() const
									{ return fValue; }

 private:
			bool				fValue;
};

// StringProperty
class StringProperty : public Property {
 public:
								StringProperty(uint32 identifier,
											   const char* string);
								StringProperty(const StringProperty& other);
								StringProperty(BMessage* archive);
	virtual						~StringProperty();

	virtual	status_t			Archive(BMessage* archive,
										bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

	virtual	Property*			Clone() const;

	virtual	type_code			Type() const
									{ return B_STRING_TYPE; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	// StringProperty
	inline	const char*			Value() const
									{ return fValue.String(); }

 private:
			BString				fValue;
};

#endif // PROPERTY_H
