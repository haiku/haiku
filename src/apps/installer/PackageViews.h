/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __PACKAGEVIEWS_H__
#define __PACKAGEVIEWS_H__

#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <CheckBox.h>
#include <Entry.h>
#include <List.h>
#include <StringView.h>


class Group {
public:
								Group();
	virtual						~Group();
			void				SetGroupName(const char* group)
									{ strcpy(fGroup, group); }
			const char*			GroupName() const
									{ return fGroup; }
private:
			char				fGroup[64];
};


class Package : public Group {
public:
								Package(const char* folder);
	virtual						~Package();

			void				SetFolder(const char* folder)
									{ strcpy(fFolder, folder); }
			void				SetName(const char* name)
									{ strcpy(fName, name); }
			void				SetDescription(const char* description)
									{ strcpy(fDescription, description); }
			void				SetSize(const int32 size)
									{ fSize = size; }
			void				SetIcon(BBitmap* icon)
									{ delete fIcon; fIcon = icon; }
			void				SetOnByDefault(bool onByDefault)
									{ fOnByDefault = onByDefault; }
			void				SetAlwaysOn(bool alwaysOn)
									{ fAlwaysOn = alwaysOn; }
			const char*			Folder() const
									{ return fFolder; }
			const char*			Name() const
									{ return fName; }
			const char*			Description() const
									{ return fDescription; }
			const int32			Size() const
									{ return fSize; }
			void				GetSizeAsString(char* string,
									size_t stringSize);
			const BBitmap*		Icon() const
									{ return fIcon; }
			bool				OnByDefault() const
									{ return fOnByDefault; }
			bool				AlwaysOn() const
									{ return fAlwaysOn; }

	static	Package*			PackageFromEntry(BEntry &dir);

private:
			char				fFolder[64];
			char				fName[64];
			char				fDescription[64];
			int32				fSize;
			BBitmap*			fIcon;
			bool				fAlwaysOn;
			bool				fOnByDefault;
};


class PackageCheckBox : public BCheckBox {
public:
								PackageCheckBox(BRect rect, Package* item);
	virtual						~PackageCheckBox();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* dragMessage);

			Package*			GetPackage()
									{ return fPackage; };
private:
			Package*			fPackage;
};


class GroupView : public BStringView {
public:
								GroupView(BRect rect, Group* group);
	virtual						~GroupView();

private:
			Group*				fGroup;
};


class PackagesView : public BView {
public:
								PackagesView(BRect rect, const char* name);
								PackagesView(const char* name);
	virtual						~PackagesView();

			void				Clean();
			void				AddPackages(BList& list, BMessage* message);
			void				GetTotalSizeAsString(char* string,
									size_t stringSize);
			void				GetPackagesToInstall(BList* list, int32* size);

	virtual	void				FrameResized(float width, float height);
	virtual	void				Draw(BRect updateRect);
	virtual	void				GetPreferredSize(float* _width, float* _height);
	virtual	BSize				MaxSize();

private:
			BList				fViews;
};

#endif	// __PACKAGEVIEWS_H__
