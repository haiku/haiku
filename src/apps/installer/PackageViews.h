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

class Package {
public:
	Package();
	~Package();
	void SetName(const char *name) { free(fName); fName=strdup(name);};
	void SetGroup(const char *group) { free(fGroup); fName=strdup(group);};
	void SetDescription(const char *description) { free(fDescription); fName=strdup(description);};
	void SetSize(const int32 size) { fSize = size; };
	void SetIcon(const BBitmap * icon);
	const char * Name() { return fName; };
	const char * Group() { return fGroup; };
	const char * Description() { return fDescription; };
	const int32 Size() { return fSize; };
	const BBitmap * Icon() { return fIcon; };

	static int Compare(const void *firstArg, const void *secondArg);
	static Package *PackageFromEntry(BEntry &dir);
private:
	char *fName;
	char *fGroup;
	char *fDescription;
	int32 fSize;
	BBitmap *fIcon;
};

class Group {
public:
	Group();
	~Group();
	void SetName(const char *name) { free(fName); fName=strdup(name);};
	const char * Name() { return fName; };
private:
	char *fName;
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
	void AddPackage(Package &package);
	void AddGroup(Group &group);
	void Clean();
private:
	BList fViews;
};

#endif	/* __PACKAGEVIEWS_H__ */
