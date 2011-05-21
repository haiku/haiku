/*
 * Public domain source code.
 *
 * Author:
 *		Joseph "looncraz" Groover <looncraz@satx.rr.com>
 */
#ifndef DECOR_INFO_H
#define DECOR_INFO_H


#include <Entry.h>
#include <Locker.h>
#include <ObjectList.h>
#include <String.h>


class BWindow;


namespace BPrivate {


// NOTE: DecorInfo itself is not thread-safe
class DecorInfo {
public:
								DecorInfo();
								DecorInfo(const BString& path);
								DecorInfo(const entry_ref& ref);
								~DecorInfo();

			status_t			SetTo(const entry_ref& ref);
			status_t			SetTo(BString path);
			status_t			InitCheck()	const;
			void				Unset();

			bool				IsDefault()	const;

			BString				Path()	const;
					// Returns "Default" for the default decorator

			const entry_ref*	Ref() const;
				// Returns NULL if virtual (default) or InitCheck() != B_OK
				// The ref returned may NOT be the same as the one given to
				// SetTo or the constructor - we may have traversed a Symlink!

			BString				Name() const;
			BString				ShortcutName() const;

			BString				Authors() const;
			BString				ShortDescription() const;
			BString				LongDescription() const;
			BString				LicenseURL() const;
			BString				LicenseName() const;
			BString				SupportURL() const;

			float				Version() const;
			time_t				ModificationTime() const;

			bool				CheckForChanges(bool &deleted);

private:
			void				_Init(bool is_update = false);

private:
			entry_ref			fRef;

			BString				fPath;
			BString				fName;
			BString				fAuthors;
			BString				fShortDescription;
			BString				fLongDescription;
			BString				fLicenseURL;
			BString				fLicenseName;
			BString				fSupportURL;

			float				fVersion;

			time_t				fModificationTime;

			status_t			fInitStatus;
};


class DecorInfoUtility {
public:
								DecorInfoUtility(bool scanNow = true);
									// NOTE: When scanNow is passed false,
									// scanning will be performed lazily, such
									// as in CountDecorators() and other
									// methods.

								~DecorInfoUtility();

			status_t			ScanDecorators();
									// Can also be used to rescan for changes.
									// Warning: potentially destructive as we
									// will remove all DecorInfo objects which
									// no longer have a file system cousin.
									// TODO: Would a call-back mechanism be
									// worthwhile here?

			int32				CountDecorators();

			DecorInfo*			DecoratorAt(int32);

			DecorInfo*			FindDecorator(const BString& string);
									// Checks for ref.name, path, fName, and
									// "Default," an empty-string returns the
									// current decorator NULL on match failure

			DecorInfo*			CurrentDecorator();
			DecorInfo*			DefaultDecorator();

			bool				IsCurrentDecorator(DecorInfo* decor);

			status_t			SetDecorator(DecorInfo* decor);
			status_t			SetDecorator(int32);

			status_t			Preview(DecorInfo* decor, BWindow* window);

private:
			DecorInfo*			_FindDecor(const BString& path);

private:
			BObjectList<DecorInfo> fList;
			BLocker				fLock;
			bool				fHasScanned;
};


}	// namespace BPrivate


using BPrivate::DecorInfo;
using BPrivate::DecorInfoUtility;


#endif	// DECOR_INFO_H
