/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BULLET_DATA_H
#define BULLET_DATA_H

#include <Referenceable.h>
#include <String.h>


class BulletData;
typedef BReference<BulletData>	BulletDataRef;


// You cannot modify a BulletData object once it has been created.
class BulletData : public BReferenceable {
public:
								BulletData();
								BulletData(const BString& string,
									float spacing);
								BulletData(const BulletData& other);

			bool				operator==(
									const BulletData& other) const;
			bool				operator!=(
									const BulletData& other) const;

			BulletDataRef		SetString(const BString& string);
	inline	const BString&		String() const
									{ return fString; }

			BulletDataRef 		SetSpacing(float spacing);
	inline	float				Spacing() const
									{ return fSpacing; }

private:
			BulletData&	operator=(const BulletData& other);

private:
			BString				fString;
			float				fSpacing;
};


#endif // BULLET_DATA_H
