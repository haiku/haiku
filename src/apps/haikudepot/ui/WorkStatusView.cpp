/*
 * Copyright 2017 Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "WorkStatusView.h"

#include <CardLayout.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <StatusBar.h>
#include <StringView.h>

#include <stdio.h>

#include "BarberPole.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WorkStatusView"


WorkStatusView::WorkStatusView(const char* name)
	:
	BView(name, 0),
	fProgressBar(new BStatusBar("progress bar")),
	fBarberPole(new BarberPole("barber pole")),
	fProgressLayout(new BCardLayout()),
	fProgressView(new BView("progress view", 0)),
	fStatusText(new BStringView("status text", NULL))
{
	fProgressView->SetLayout(fProgressLayout);
	fProgressLayout->AddView(fBarberPole);
	fProgressLayout->AddView(fProgressBar);

	fProgressBar->SetMaxValue(1.0f);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(new BSeparatorView())
		.AddGroup(B_HORIZONTAL)
			.SetInsets(5, 5, 5, 5)
			.Add(fProgressLayout, 0.2f)
			.Add(fStatusText)
			.AddGlue()
			.End()
	;
}


WorkStatusView::~WorkStatusView()
{
}


void
WorkStatusView::SetBusy(const BString& text)
{
	SetText(text);
	SetBusy();
}


void
WorkStatusView::SetBusy()
{
	fBarberPole->Start();
	if (fProgressLayout->VisibleIndex() != 0)
		fProgressLayout->SetVisibleItem((int32)0);
}


void
WorkStatusView::SetIdle()
{
	fBarberPole->Stop();
	fProgressLayout->SetVisibleItem((int32)0);
	SetText(NULL);
}


void
WorkStatusView::SetProgress(float value)
{
	fProgressBar->SetTo(value);
	if (fProgressLayout->VisibleIndex() != 1)
		fProgressLayout->SetVisibleItem(1);
}


void
WorkStatusView::SetText(const BString& text)
{
	fStatusText->SetText(text);
}


void
WorkStatusView::PackageStatusChanged(const PackageInfoRef& package)
{
	switch (package->State()) {
		case DOWNLOADING:
			fPendingPackages.erase(package->Name());
			if (package->Name() != fDownloadingPackage) {
				fDownloadingPackage = package->Name();
				_SetTextDownloading(package->Title());
			}
			SetProgress(package->DownloadProgress());
			break;

		case PENDING:
			fPendingPackages.insert(package->Name());
			if (package->Name() == fDownloadingPackage)
				fDownloadingPackage = "";
			if (fDownloadingPackage.IsEmpty()) {
				_SetTextPendingDownloads();
				SetBusy();
			}
			break;

		case NONE:
		case ACTIVATED:
		case INSTALLED:
		case UNINSTALLED:
			if (package->Name() == fDownloadingPackage)
				fDownloadingPackage = "";
			fPendingPackages.erase(package->Name());
			if (fPendingPackages.empty())
				SetIdle();
			else {
				_SetTextPendingDownloads();
				SetBusy();
			}
			break;
	}
}


void
WorkStatusView::_SetTextPendingDownloads()
{
	BString text;
	const size_t pendingCount = fPendingPackages.size();
	text << pendingCount;
	if (pendingCount > 1)
		text << B_TRANSLATE(" packages to download");
	else
		text << B_TRANSLATE(" package to download");
	SetText(text);
}


void
WorkStatusView::_SetTextDownloading(const BString& title)
{
	BString text(B_TRANSLATE("Downloading package "));
	text << title;
	if (!fPendingPackages.empty()) {
		text << " (";
		text << fPendingPackages.size();
		text << B_TRANSLATE(" more to download)");
	}
	SetText(text);
}
