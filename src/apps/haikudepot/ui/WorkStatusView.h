/*
 * Copyright 2017 Julian Harnath <julian.harnath@rwth-aachen.de>
 * Copyright 2021 Andrew Lindesay <apl@lindesay.co.nz>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef WORK_STATUS_VIEW_H
#define WORK_STATUS_VIEW_H


#include <View.h>

#include "PackageInfo.h"


class BarberPole;
class BCardLayout;
class BMessageRunner;
class BStatusBar;
class BStringView;


class WorkStatusView : public BView {
public:
								WorkStatusView(const char* name);
								~WorkStatusView();

			void				SetBusy(const BString& text);
			void				SetBusy();
			void				SetIdle();
			void				SetProgress(float value);
			void				SetText(const BString& text);

private:
			BStatusBar*			fProgressBar;
			BarberPole*			fBarberPole;
			BCardLayout*		fProgressLayout;
			BView*				fProgressView;
			BStringView*		fStatusText;
};


#endif // WORK_STATUS_VIEW_H
