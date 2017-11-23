/*
 * Copyright 2017 Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef WORK_STATUS_VIEW_H
#define WORK_STATUS_VIEW_H


#include <View.h>

#include <set>

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

			void				PackageStatusChanged(
									const PackageInfoRef& package);

private:
			void				_SetTextPendingDownloads();
			void				_SetTextDownloading(const BString& title);

private:
			BStatusBar*			fProgressBar;
			BarberPole*			fBarberPole;
			BCardLayout*		fProgressLayout;
			BView*				fProgressView;
			BStringView*		fStatusText;

			BString				fDownloadingPackage;
			std::set<BString>	fPendingPackages;
};


#endif // WORK_STATUS_VIEW_H
