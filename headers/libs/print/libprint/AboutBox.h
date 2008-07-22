/*
 * AboutBox.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __ABOUTBOX_H
#define __ABOUTBOX_H

#include <Application.h>

class AboutBox : public BApplication {
public:
	AboutBox(const char *signature, const char *driver_name, const char *version, const char *copyright);
};

#endif	/* __ABOUTBOX_H */
