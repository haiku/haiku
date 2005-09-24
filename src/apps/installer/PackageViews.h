/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __PACKAGEVIEWS_H__
#define __PACKAGEVIEWS_H__

#include <CheckBox.h>
#include <List.h>
#include <StringView.h>
#include <stdlib.h>
#include <string.h>

class Group {
public:
	Group();
	virtual ~Group();
	void SetGroupName(const char *group) { free(fGroup); fGroup=strdup(group);};
	const char * GroupName() const { return fGroup; };
private:
	char *fGroup;
};


class Package : public Group {
public:
	Package();
	virtual ~Package();
	void SetName(const char *name) { free(fName); fName=strdup(name);};
	void SetDescription(const char *description) { free(fDescription); fDescription=strdup(description);};
	void SetSize(const int32 size) { fSize = size; };
	void SetIcon(const BBitmap * icon);
	void SetOnByDefault(bool onByDefault) { fOnByDefault = onByDefault; };
	void SetAlwaysOn(bool alwaysOn) { fAlwaysOn = alwaysOn; };
	const char * Name() const { return fName; };
	const char * Description() const { return fDescription; };
	const int32 Size() const { return fSize; };
	const BBitmap * Icon() const { return fIcon; };
	bool OnByDefault() const { return fOnByDefault; };
	bool AlwaysOn() const { return fAlwaysOn; };

	static Package *PackageFromEntry(BEntry &dir);
private:
	char *fName;
	char *fDescription;
	int32 fSize;
	BBitmap *fIcon;
	bool fAlwaysOn, fOnByDefault;
};


class PackageCheckBox : public BCheckBox {
	public:
		PackageCheckBox(BRect rect, Package &item);
		~PackageCheckBox();
		void Draw(BRect update);
	private:
		Package &fPackage;
};

class GroupView : public BStringView {
	public:
		GroupView(BRect rect, Group &group);
		~GroupView();
	private:
		Group &fGroup;

};


class PackagesView : public BView {
public:
	PackagesView(BRect rect, const char* name);
	~PackagesView();
	void Clean();
	void AddPackages(BList &list);
private:
	BList fViews;
};

#endif	/* __PACKAGEVIEWS_H__ */
