/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BULLET_H
#define BULLET_H

#include "BulletData.h"


class Bullet {
public:
								Bullet();
								Bullet(const BString& string, float spacing);
								Bullet(const Bullet& other);

			Bullet&				operator=(const Bullet& other);
			bool				operator==(const Bullet& other) const;
			bool				operator!=(const Bullet& other) const;

			bool				SetString(const BString& string);
			const BString&		String() const;

			bool				SetSpacing(float spacing);
			float				Spacing() const;

private:
			BulletDataRef		fBulletData;
};


#endif // BULLET_H
