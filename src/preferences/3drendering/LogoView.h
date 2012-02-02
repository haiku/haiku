/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef LOGO_VIEW_H
#define LOGO_VIEW_H


#include <View.h>

class LogoView : public BView {
public:
    LogoView();
    virtual ~LogoView();
    
    virtual void Draw(BRect update);
    virtual	void GetPreferredSize(float* _width, float* _height);

private:
	BBitmap* fLogo;
};

#endif /* LOGO_VIEW_H */
