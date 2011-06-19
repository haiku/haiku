/* Numail Kit - general header for using the kit
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <MailPrivate.h>

#include <stdio.h>

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Locker.h>
#include <Messenger.h>
#include <Path.h>
#include <String.h>


#define timeout 5e5


namespace BPrivate {


BPath
default_mail_directory()
{
	BPath path;
	if (find_directory(B_USER_DIRECTORY, &path) == B_OK)
		path.Append("mail");
	else
		path.SetTo("/boot/home/mail/");

	return path;
}


BPath
default_mail_in_directory()
{
	BPath path = default_mail_directory();
	path.Append("in");

	return path;
}


BPath
default_mail_out_directory()
{
	BPath path = default_mail_directory();
	path.Append("out");

	return path;
}


status_t
WriteMessageFile(const BMessage& archive, const BPath& path, const char* name)
{
	status_t ret = B_OK;
	BString leaf = name;
	leaf << ".tmp";

	BEntry settings_entry;
	BFile tmpfile;
	bigtime_t now = system_time();

	create_directory(path.Path(), 0777);
	{
		BDirectory account_dir(path.Path());
		ret = account_dir.InitCheck();
		if (ret != B_OK)
		{
			fprintf(stderr, "Couldn't open '%s': %s\n",
				path.Path(), strerror(ret));
			return ret;
		}

		// get an entry for the tempfile
		// Get it here so that failure doesn't create any problems
		ret = settings_entry.SetTo(&account_dir,leaf.String());
		if (ret != B_OK)
		{
			fprintf(stderr, "Couldn't create an entry for '%s/%s': %s\n",
				path.Path(), leaf.String(), strerror(ret));
			return ret;
		}
	}

	//
	// Save to a temporary file
	//

	// Our goal is to write to a tempfile and then use 'rename' to
	// link that file into place once it contains valid contents.
	// Given the filesystem's guarantee of atomic "rename" oper-
	// ations this will guarantee that any non-temp files in the
	// config directory are valid configuration files.
	//
	// Ideally, we would be able to do the following:
	//   BFile tmpfile(&account_dir, "tmpfile", B_WRITE_ONLY|B_CREATE_FILE);
	//   // ...
	//   tmpfile.Relink(&account_dir,"realfile");
	// But this doesn't work because there is no way in the API
	// to link based on file descriptor.  (There should be, for
	// exactly this reason, and I see no reason that it can not
	// be added to the API, but as it is not present now so we'll
	// have to deal.)  It has to be file-descriptor based because
	// (a) all a BFile knows is its node/FD and (b) the file may
	// be unlinked at any time, at which point renaming the entry
	// to clobber the "realfile" will result in an invalid con-
	// figuration file being created.
	//
	// We can't count on not clobbering the tempfile to gain
	// exclusivity because, if the system crashes between when
	// we create the tempfile an when we rename it, we will have
	// a zombie tempfile that will prevent us from doing any more
	// saves.
	//
	// What we can do is:
	//
	//    Create or open the tempfile
	//    // At this point no one will *clobber* the file, but
	//    // others may open it
	//    Lock the tempfile
	//    // At this point, no one else may open it and we have
	//    // exclusive access to it.  Because of the above, we
	//    // know that our entry is still valid
	//
	//    Truncate the tempfile
	//    Write settings
	//    Sync
	//    Rename to the realfile
	//    // this does not affect the lock, but now open-
	//    // ing the realfile will fail with B_BUSY
	//    Unlock
	//
	// If this code is the only code that changes these files,
	// then we are guaranteed that all realfiles will be valid
	// settings files.  I think that's the best we can do unless
	// we get the Relink() api.  An implementation of the above
	// follows.
	//

	// Create or open
	ret = B_TIMED_OUT;
	while (system_time() - now < timeout) //-ATT-no timeout arg. Setting by #define
	{
		ret = tmpfile.SetTo(&settings_entry, B_WRITE_ONLY | B_CREATE_FILE);
		if (ret != B_BUSY) break;

		// wait 1/100th second
		snooze((bigtime_t)1e4);
	}
	if (ret != B_OK)
	{
		fprintf(stderr, "Couldn't open '%s/%s' within the timeout period (%fs): %s\n",
			path.Path(), leaf.String(), (float)timeout/1e6, strerror(ret));
		return ret==B_BUSY? B_TIMED_OUT:ret;
	}

	// lock
	ret = B_TIMED_OUT;
	while (system_time() - now < timeout)
	{
		ret = tmpfile.Lock(); //-ATT-changed account_file to tmpfile. Is that allowed?
		if (ret != B_BUSY) break;

		// wait 1/100th second
		snooze((bigtime_t)1e4);
	}
	if (ret != B_OK)
	{
		fprintf(stderr, "Couldn't lock '%s/%s' in within the timeout period (%fs): %s\n",
			path.Path(), leaf.String(), (float)timeout/1e6, strerror(ret));
		// Can't remove it here, since it might be someone else's.
		// Leaving a zombie shouldn't cause any problems tho so
		// that's OK.
		return ret==B_BUSY? B_TIMED_OUT:ret;
	}

	// truncate
	tmpfile.SetSize(0);

	// write
	ret = archive.Flatten(&tmpfile);
	if (ret != B_OK)
	{
		fprintf(stderr, "Couldn't flatten settings to '%s/%s': %s\n",
			path.Path(), leaf.String(), strerror(ret));
		return ret;
	}

	// ensure it's actually writen
	ret = tmpfile.Sync();
	if (ret != B_OK)
	{
		fprintf(stderr, "Couldn't sync settings to '%s/%s': %s\n",
			path.Path(), leaf.String(), strerror(ret));
		return ret;
	}

	// clobber old settings
	ret = settings_entry.Rename(name,true);
	if (ret != B_OK)
	{
		fprintf(stderr, "Couldn't clobber old settings '%s/%s': %s\n",
			path.Path(), name, strerror(ret));
		return ret;
	}

	return B_OK;
}


}	// namespace BPrivate
