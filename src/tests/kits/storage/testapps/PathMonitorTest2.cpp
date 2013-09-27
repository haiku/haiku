/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <algorithm>
#include <set>
#include <vector>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Looper.h>
#include <ObjectList.h>
#include <Path.h>
#include <String.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <NotOwningEntryRef.h>
#include <PathMonitor.h>


using BPrivate::BPathMonitor;


static const char* const kTestBasePath = "/tmp/path-monitor-test";
static const bigtime_t kMaxNotificationDelay = 100000;


#define FATAL(...)													\
	do {															\
		throw FatalException(										\
			BString().SetToFormat("%s:%d: ", __FILE__, __LINE__)	\
				<< BString().SetToFormat(__VA_ARGS__));				\
	} while (false)

#define FATAL_IF_ERROR(error, ...)										\
	do {																\
		status_t _fatalError = (error);									\
		if (_fatalError < 0) {											\
			throw FatalException(										\
				BString().SetToFormat("%s:%d: ", __FILE__, __LINE__)	\
					<< BString().SetToFormat(__VA_ARGS__)				\
					<< BString().SetToFormat(							\
						": %s\n", strerror(_fatalError)));				\
		}																\
	} while (false)

#define FATAL_IF_POSIX_ERROR(error, ...)	\
	if ((error) < 0)						\
		FATAL_IF_ERROR(errno, __VA_ARGS__)

#define FAIL(...)	\
	throw TestException(BString().SetToFormat(__VA_ARGS__))


struct TestException {
	TestException(const BString& message)
		:
		fMessage(message)
	{
	}

	const BString& Message() const
	{
		return fMessage;
	}

private:
	BString	fMessage;
};


struct FatalException {
	FatalException(const BString& message)
		:
		fMessage(message)
	{
	}

	const BString& Message() const
	{
		return fMessage;
	}

private:
	BString	fMessage;
};


static BString
test_path(const BString& maybeRelativePath)
{
	if (maybeRelativePath.ByteAt(0) == '/')
		return maybeRelativePath;

	BString path;
	path.SetToFormat("%s/%s", kTestBasePath, maybeRelativePath.String());
	if (path.IsEmpty())
		FATAL_IF_ERROR(B_NO_MEMORY, "Failed to make absolute path");
	return path;
}


static BString
node_ref_to_string(const node_ref& nodeRef)
{
	return BString().SetToFormat("%" B_PRIdDEV ":%" B_PRIdINO, nodeRef.device,
		nodeRef.node);
}


static BString
entry_ref_to_string(const entry_ref& entryRef)
{
	return BString().SetToFormat("%" B_PRIdDEV ":%" B_PRIdINO ":\"%s\"",
		entryRef.device, entryRef.directory, entryRef.name);
}


static BString
indented_string(const char* string, const char* indent,
	const char* firstIndent = NULL)
{
	const char* end = string + strlen(string);
	BString result;
	const char* line = string;
	while (line < end) {
		const char* lineEnd = strchr(line, '\n');
		lineEnd = lineEnd != NULL ? lineEnd + 1 : end;
		result
			<< (line == string && firstIndent != NULL ? firstIndent : indent);
		result.Append(line, lineEnd - line);
		line = lineEnd;
	}

	return result;
}


static BString
message_to_string(const BMessage& message)
{
	BString result;

	char* name;
	type_code typeCode;
	int32 count;
	for (int32 i = 0;
		message.GetInfo(B_ANY_TYPE, i, &name, &typeCode, &count) == B_OK;
		i++) {
		if (i > 0)
			result << '\n';

		result << '"' << name << '"';
		BString type;

		switch (typeCode) {
			case B_UINT8_TYPE:
			case B_INT8_TYPE:
				type << "int8";
				break;

			case B_UINT16_TYPE:
				type = "u";
			case B_INT16_TYPE:
				type << "int16";
				break;

			case B_UINT32_TYPE:
				type = "u";
			case B_INT32_TYPE:
				type << "int32";
				break;

			case B_UINT64_TYPE:
				type = "u";
			case B_INT64_TYPE:
				type << "int64";
				break;

			case B_STRING_TYPE:
				type = "string";
				break;

			default:
			{
				int code = (int)typeCode;
				type.SetToFormat("'%02x%02x%02x%02x'", code >> 24,
					(code >> 16) & 0xff, (code >> 8) & 0xff, code & 0xff);
				break;
			}
		}

		result << " (" << type << "):";

		for (int32 k = 0; k < count; k++) {
			BString value;
			switch (typeCode) {
				case B_UINT8_TYPE:
					value << message.GetUInt8(name, k, 0);
					break;
				case B_INT8_TYPE:
					value << message.GetInt8(name, k, 0);
					break;
				case B_UINT16_TYPE:
					value << message.GetUInt16(name, k, 0);
					break;
				case B_INT16_TYPE:
					value << message.GetInt16(name, k, 0);
					break;
				case B_UINT32_TYPE:
					value << message.GetUInt32(name, k, 0);
					break;
				case B_INT32_TYPE:
					value << message.GetInt32(name, k, 0);
					break;
				case B_UINT64_TYPE:
					value << message.GetUInt64(name, k, 0);
					break;
				case B_INT64_TYPE:
					value << message.GetInt64(name, k, 0);
					break;
				case B_STRING_TYPE:
					value.SetToFormat("\"%s\"", message.GetString(name, k, ""));
					break;
				default:
				{
					const void* data;
					ssize_t size;
					if (message.FindData(name, typeCode, k, &data, &size)
							!= B_OK) {
						value = "???";
						break;
					}

					for (ssize_t l = 0; l < size; l++) {
						uint8 v = ((const uint8*)data)[l];
						value << BString().SetToFormat("%02x", v);
					}
					break;
				}
			}

			if (k == 0 && count == 1) {
				result << ' ' << value;
			} else {
				result << BString().SetToFormat("\n  [%2" B_PRId32 "] ", k)
					<< value;
			}
		}
	}

	return result;
}


static BString
watch_flags_to_string(uint32 flags)
{
	BString result;
	if ((flags & B_WATCH_NAME) != 0)
		result << "name ";
	if ((flags & B_WATCH_STAT) != 0)
		result << "stat ";
	if ((flags & B_WATCH_ATTR) != 0)
		result << "attr ";
	if ((flags & B_WATCH_DIRECTORY) != 0)
		result << "dir ";
	if ((flags & B_WATCH_RECURSIVELY) != 0)
		result << "recursive ";
	if ((flags & B_WATCH_FILES_ONLY) != 0)
		result << "files-only ";
	if ((flags & B_WATCH_DIRECTORIES_ONLY) != 0)
		result << "dirs-only ";

	if (!result.IsEmpty())
		result.Truncate(result.Length() - 1);
	return result;
}


struct MonitoringInfo {
	MonitoringInfo()
	{
	}

	MonitoringInfo(int32 opcode, const char* path)
		:
		fOpcode(opcode)
	{
		_Init(opcode, path);
	}

	MonitoringInfo(int32 opcode, const char* fromPath, const char* toPath)
	{
		_Init(opcode, toPath);

		// init fFromEntryRef
		BEntry entry;
		FATAL_IF_ERROR(entry.SetTo(fromPath),
			"Failed to init BEntry for \"%s\"", fromPath);
		FATAL_IF_ERROR(entry.GetRef(&fFromEntryRef),
			"Failed to get entry_ref for \"%s\"", fromPath);
	}

	BString ToString() const
	{
		switch (fOpcode) {
			case B_ENTRY_CREATED:
			case B_ENTRY_REMOVED:
				return BString().SetToFormat("%s %s at %s",
					fOpcode == B_ENTRY_CREATED ? "created" : "removed",
					node_ref_to_string(fNodeRef).String(),
					entry_ref_to_string(fEntryRef).String());

			case B_ENTRY_MOVED:
				return BString().SetToFormat("moved %s from %s to %s",
					node_ref_to_string(fNodeRef).String(),
					entry_ref_to_string(fFromEntryRef).String(),
					entry_ref_to_string(fEntryRef).String());

			case B_STAT_CHANGED:
				return BString().SetToFormat("stat changed for %s",
					node_ref_to_string(fNodeRef).String());

			case B_ATTR_CHANGED:
				return BString().SetToFormat("attr changed for %s",
					node_ref_to_string(fNodeRef).String());

			case B_DEVICE_MOUNTED:
				return BString().SetToFormat("volume mounted");

			case B_DEVICE_UNMOUNTED:
				return BString().SetToFormat("volume unmounted");
		}

		return BString();
	}

	bool Matches(const BMessage& message) const
	{
		if (fOpcode != message.GetInt32("opcode", -1))
			return false;

		switch (fOpcode) {
			case B_ENTRY_CREATED:
			case B_ENTRY_REMOVED:
			{
				NotOwningEntryRef entryRef;
				node_ref nodeRef;

				if (message.FindInt32("device", &nodeRef.device) != B_OK
					|| message.FindInt64("node", &nodeRef.node) != B_OK
					|| message.FindInt64("directory", &entryRef.directory)
						!= B_OK
					|| message.FindString("name", (const char**)&entryRef.name)
						!= B_OK) {
					return false;
				}
				entryRef.device = nodeRef.device;

				return nodeRef == fNodeRef && entryRef == fEntryRef;
			}

			case B_ENTRY_MOVED:
			{
				NotOwningEntryRef fromEntryRef;
				NotOwningEntryRef toEntryRef;
				node_ref nodeRef;

				if (message.FindInt32("node device", &nodeRef.device) != B_OK
					|| message.FindInt64("node", &nodeRef.node) != B_OK
					|| message.FindInt32("device", &fromEntryRef.device)
						!= B_OK
					|| message.FindInt64("from directory",
						&fromEntryRef.directory) != B_OK
					|| message.FindInt64("to directory", &toEntryRef.directory)
						!= B_OK
					|| message.FindString("from name",
						(const char**)&fromEntryRef.name) != B_OK
					|| message.FindString("name",
						(const char**)&toEntryRef.name) != B_OK) {
					return false;
				}
				toEntryRef.device = fromEntryRef.device;

				return nodeRef == fNodeRef && toEntryRef == fEntryRef
					&& fromEntryRef == fFromEntryRef;
			}

			case B_STAT_CHANGED:
			case B_ATTR_CHANGED:
			{
				node_ref nodeRef;

				if (message.FindInt32("device", &nodeRef.device) != B_OK
					|| message.FindInt64("node", &nodeRef.node) != B_OK) {
					return false;
				}

				return nodeRef == fNodeRef;
			}

			case B_DEVICE_MOUNTED:
			case B_DEVICE_UNMOUNTED:
				return true;
		}

		return false;
	}

private:
	void _Init(int32 opcode, const char* path)
	{
		fOpcode = opcode;
		BEntry entry;
		FATAL_IF_ERROR(entry.SetTo(path), "Failed to init BEntry for \"%s\"",
			path);
		FATAL_IF_ERROR(entry.GetRef(&fEntryRef),
			"Failed to get entry_ref for \"%s\"", path);
		FATAL_IF_ERROR(entry.GetNodeRef(&fNodeRef),
			"Failed to get node_ref for \"%s\"", path);
	}

private:
	int32		fOpcode;
	node_ref	fNodeRef;
	entry_ref	fEntryRef;
	entry_ref	fFromEntryRef;
};


struct MonitoringInfoSet {
	MonitoringInfoSet()
	{
	}

	MonitoringInfoSet& Add(const MonitoringInfo& info, bool expected = true)
	{
		if (expected)
			fInfos.push_back(info);
		return *this;
	}

	MonitoringInfoSet& Add(int32 opcode, const BString& path,
		bool expected = true)
	{
		return Add(MonitoringInfo(opcode, test_path(path)), expected);
	}

	MonitoringInfoSet& Add(int32 opcode, const BString& fromPath,
		const BString& toPath, bool expected = true)
	{
		return Add(MonitoringInfo(opcode, test_path(fromPath),
			test_path(toPath)), expected);
	}

	bool IsEmpty() const
	{
		return fInfos.empty();
	}

	int32 CountInfos() const
	{
		return fInfos.size();
	}

	const MonitoringInfo& InfoAt(int32 index) const
	{
		return fInfos[index];
	}

	void Remove(int32 index)
	{
		fInfos.erase(fInfos.begin() + index);
	}

	BString ToString() const
	{
		BString result;
		for (int32 i = 0; i < CountInfos(); i++) {
			const MonitoringInfo& info = InfoAt(i);
			if (i > 0)
				result << '\n';
			result << info.ToString();
		}
		return result;
	}

private:
	std::vector<MonitoringInfo>	fInfos;
};


struct Test : private BLooper {
	Test(const char* name)
		:
		fName(name),
		fFlags(0),
		fLooperThread(-1),
		fNotifications(10, true),
		fProcessedMonitoringInfos(),
		fIsWatching(false)
	{
	}

	void Init(uint32 flags)
	{
		fFlags = flags;

		// delete and re-create the test directory
		BEntry entry;
		FATAL_IF_ERROR(entry.SetTo(kTestBasePath),
			"Failed to init entry to \"%s\"", kTestBasePath);

		if (entry.Exists())
			_RemoveRecursively(entry);

		_CreateDirectory(kTestBasePath);

		fLooperThread = BLooper::Run();
		if (fLooperThread < 0)
			FATAL_IF_ERROR(fLooperThread, "Failed to init looper");
	}

	void Delete()
	{
		if (fIsWatching)
			BPathMonitor::StopWatching(this);

		if (fLooperThread < 0) {
			delete this;
		} else {
			PostMessage(B_QUIT_REQUESTED);
			wait_for_thread(fLooperThread, NULL);
		}
	}

	void Do()
	{
		bool recursive = (fFlags & B_WATCH_RECURSIVELY) != 0;
		DoInternal(recursive && (fFlags & B_WATCH_DIRECTORIES_ONLY) != 0,
			recursive && (fFlags & B_WATCH_FILES_ONLY) != 0, recursive,
			!recursive && (fFlags & B_WATCH_DIRECTORY) == 0,
			(fFlags & B_WATCH_STAT) != 0);

		// verify that there aren't any spurious notifications
		snooze(kMaxNotificationDelay);

		AutoLocker<BLooper> locker(this);
		if (fNotifications.IsEmpty())
			return;

		BString pendingNotifications
			= "unexpected notification(s) at end of test:";
		for (int32 i = 0; BMessage* message = fNotifications.ItemAt(i); i++) {
			pendingNotifications << '\n'
				<< indented_string(message_to_string(*message), "    ", "  * ");
		}

		FAIL("%s%s", pendingNotifications.String(),
			_ProcessedInfosString().String());
	}

	const BString& Name() const
	{
		return fName;
	}

protected:
	~Test()
	{
	}

	void StartWatching(const char* path)
	{
		BString absolutePath(test_path(path));
		FATAL_IF_ERROR(BPathMonitor::StartWatching(absolutePath, fFlags, this),
			"Failed to start watching \"%s\"", absolutePath.String());
		fIsWatching = true;
	}

	MonitoringInfo CreateDirectory(const char* path)
	{
		BString absolutePath(test_path(path));
		_CreateDirectory(absolutePath);
		return MonitoringInfo(B_ENTRY_CREATED, absolutePath);
	}

	MonitoringInfo CreateFile(const char* path)
	{
		BString absolutePath(test_path(path));
		FATAL_IF_ERROR(
			BFile().SetTo(absolutePath, B_CREATE_FILE | B_READ_WRITE),
			"Failed to create file \"%s\"", absolutePath.String());
		return MonitoringInfo(B_ENTRY_CREATED, absolutePath);
	}

	MonitoringInfo MoveEntry(const char* fromPath, const char* toPath)
	{
		BString absoluteFromPath(test_path(fromPath));
		BString absoluteToPath(test_path(toPath));
		FATAL_IF_POSIX_ERROR(rename(absoluteFromPath, absoluteToPath),
			"Failed to move \"%s\" to \"%s\"", absoluteFromPath.String(),
			absoluteToPath.String());
		return MonitoringInfo(B_ENTRY_MOVED, absoluteFromPath, absoluteToPath);
	}

	MonitoringInfo RemoveEntry(const char* path)
	{
		BString absolutePath(test_path(path));
		MonitoringInfo info(B_ENTRY_REMOVED, absolutePath);
		BEntry entry;
		FATAL_IF_ERROR(entry.SetTo(absolutePath),
			"Failed to init BEntry for \"%s\"", absolutePath.String());
		FATAL_IF_ERROR(entry.Remove(),
			"Failed to remove entry \"%s\"", absolutePath.String());
		return info;
	}

	MonitoringInfo TouchEntry(const char* path)
	{
		BString absolutePath(test_path(path));
		FATAL_IF_POSIX_ERROR(utimes(absolutePath, NULL),
			"Failed to touch \"%s\"", absolutePath.String());
		MonitoringInfo info(B_STAT_CHANGED, absolutePath);
		return info;
	}

	void ExpectNotification(const MonitoringInfo& info, bool expected = true)
	{
		if (!expected)
			return;

		AutoLocker<BLooper> locker(this);
		if (fNotifications.IsEmpty()) {
			locker.Unlock();
			snooze(kMaxNotificationDelay);
			locker.Lock();
		}

		if (fNotifications.IsEmpty()) {
			FAIL("missing notification, expected:\n  %s",
				info.ToString().String());
		}

		BMessage* message = fNotifications.RemoveItemAt(0);
		ObjectDeleter<BMessage> messageDeleter(message);

		if (!info.Matches(*message)) {
			BString processedInfosString(_ProcessedInfosString());
			FAIL("unexpected notification:\n  expected:\n  %s\n  got:\n%s%s",
				info.ToString().String(),
				indented_string(message_to_string(*message), "    ").String(),
				processedInfosString.String());
		}

		fProcessedMonitoringInfos.Add(info);
	}

	void ExpectNotifications(MonitoringInfoSet infos)
	{
		bool waited = false;
		AutoLocker<BLooper> locker(this);

		while (!infos.IsEmpty()) {
			if (fNotifications.IsEmpty()) {
				locker.Unlock();
				if (!waited) {
					snooze(kMaxNotificationDelay);
					waited = true;
				}
				locker.Lock();
			}

			if (fNotifications.IsEmpty()) {
				FAIL("missing notification(s), expected:\n%s",
					indented_string(infos.ToString(), "  ").String());
			}

			BMessage* message = fNotifications.RemoveItemAt(0);
			ObjectDeleter<BMessage> messageDeleter(message);

			bool foundMatch = false;
			for (int32 i = 0; i < infos.CountInfos(); i++) {
				const MonitoringInfo& info = infos.InfoAt(i);
				if (info.Matches(*message)) {
					infos.Remove(i);
					foundMatch = true;
					break;
				}
			}

			if (foundMatch)
				continue;

			BString processedInfosString(_ProcessedInfosString());
			FAIL("unexpected notification:\n  expected:\n%s\n  got:\n%s%s",
				indented_string(infos.ToString(), "    ").String(),
				indented_string(message_to_string(*message), "    ").String(),
				processedInfosString.String());
		}
	}

	virtual void DoInternal(bool directoriesOnly, bool filesOnly,
		bool recursive, bool pathOnly, bool watchStat) = 0;

private:
	typedef BObjectList<BMessage> MessageList;
	typedef BObjectList<MonitoringInfo> MonitoringInfoList;

private:
	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case B_PATH_MONITOR:
				if (!fNotifications.AddItem(new BMessage(*message)))
					FATAL_IF_ERROR(B_NO_MEMORY, "Failed to store notification");
				break;

			default:
				BLooper::MessageReceived(message);
				break;
		}
	}

private:
	void _CreateDirectory(const char* path)
	{
		FATAL_IF_ERROR(create_directory(path, 0755),
			"Failed to create directory \"%s\"", path);
	}

	void _RemoveRecursively(BEntry& entry)
	{
		// recurse, if the entry is a directory
		if (entry.IsDirectory()) {
			BDirectory directory;
			FATAL_IF_ERROR(directory.SetTo(&entry),
				"Failed to init BDirectory for \"%s\"",
				BPath(&entry).Path());

			BEntry childEntry;
			while (directory.GetNextEntry(&childEntry) == B_OK)
				_RemoveRecursively(childEntry);
		}

		// remove the entry
		FATAL_IF_ERROR(entry.Remove(), "Failed to remove entry \"%s\"",
			BPath(&entry).Path());
	}

	BString _ProcessedInfosString() const
	{
		BString processedInfosString;
		if (!fProcessedMonitoringInfos.IsEmpty()) {
			processedInfosString << "\nprocessed so far:\n"
				<< indented_string(fProcessedMonitoringInfos.ToString(), "  ");
		}
		return processedInfosString;
	}

protected:
	BString				fName;
	uint32				fFlags;
	thread_id			fLooperThread;
	MessageList			fNotifications;
	MonitoringInfoSet	fProcessedMonitoringInfos;
	bool				fIsWatching;
};


struct TestBase : Test {
protected:
	TestBase(const char* name)
		:
		Test(name)
	{
	}

	void StandardSetup()
	{
		CreateDirectory("base");
		CreateDirectory("base/dir1");
		CreateDirectory("base/dir1/dir0");
		CreateFile("base/file0");
		CreateFile("base/dir1/file0.0");
	}
};


#define CREATE_TEST_WITH_CUSTOM_SETUP(name, code)						\
	struct Test##name : TestBase {										\
		Test##name() : TestBase(#name) {}								\
		virtual void DoInternal(bool directoriesOnly, bool filesOnly,	\
			bool recursive, bool pathOnly, bool watchStat)				\
		{																\
			code														\
		}																\
	};																	\
	tests.push_back(new Test##name);

#define CREATE_TEST(name, code)			\
	CREATE_TEST_WITH_CUSTOM_SETUP(name, \
		StandardSetup();				\
		StartWatching("base");			\
		code							\
	)


static void
create_tests(std::vector<Test*>& tests)
{
	// test coverage:
	// - file/directory outside
	// - file/directory at top level
	// - file/directory at sub level
	// - move file/directory into/within/out of
	// - move non-empty directory into/within/out of
	// - create/move ancestor folder
	// - remove/move ancestor folder
	// - touch path, file/directory at top and sub level
	// - base file instead of directory
	//
	// not covered (yet):
	// - mount/unmount below/in our path
	// - test symlink in watched path
	// - attribute watching (should be similar to stat watching)

	CREATE_TEST(FileOutside,
		CreateFile("file1");
		MoveEntry("file1", "file2");
		RemoveEntry("file2");
	)

	CREATE_TEST(DirectoryOutside,
		CreateDirectory("dir1");
		MoveEntry("dir1", "dir2");
		RemoveEntry("dir2");
	)

	CREATE_TEST(FileTopLevel,
		ExpectNotification(CreateFile("base/file1"),
			!directoriesOnly && !pathOnly);
		ExpectNotification(MoveEntry("base/file1", "base/file2"),
			!directoriesOnly && !pathOnly);
		ExpectNotification(RemoveEntry("base/file2"),
			!directoriesOnly && !pathOnly);
	)

	CREATE_TEST(DirectoryTopLevel,
		ExpectNotification(CreateDirectory("base/dir2"),
			!filesOnly && !pathOnly);
		ExpectNotification(MoveEntry("base/dir2", "base/dir3"),
			!filesOnly && !pathOnly);
		ExpectNotification(RemoveEntry("base/dir3"),
			!filesOnly && !pathOnly);
	)

	CREATE_TEST(FileSubLevel,
		ExpectNotification(CreateFile("base/dir1/file1"),
			recursive && !directoriesOnly);
		ExpectNotification(MoveEntry("base/dir1/file1", "base/dir1/file2"),
			recursive && !directoriesOnly);
		ExpectNotification(RemoveEntry("base/dir1/file2"),
			recursive && !directoriesOnly);
	)

	CREATE_TEST(DirectorySubLevel,
		ExpectNotification(CreateDirectory("base/dir1/dir2"),
			recursive && !filesOnly);
		ExpectNotification(MoveEntry("base/dir1/dir2", "base/dir1/dir3"),
			recursive && !filesOnly);
		ExpectNotification(RemoveEntry("base/dir1/dir3"),
			recursive && !filesOnly);
	)

	CREATE_TEST(FileMoveIntoTopLevel,
		CreateFile("file1");
		ExpectNotification(MoveEntry("file1", "base/file2"),
			!directoriesOnly && !pathOnly);
		ExpectNotification(RemoveEntry("base/file2"),
			!directoriesOnly && !pathOnly);
	)

	CREATE_TEST(DirectoryMoveIntoTopLevel,
		CreateDirectory("dir2");
		ExpectNotification(MoveEntry("dir2", "base/dir3"),
			!filesOnly && !pathOnly);
		ExpectNotification(RemoveEntry("base/dir3"),
			!filesOnly && !pathOnly);
	)

	CREATE_TEST(FileMoveIntoSubLevel,
		CreateFile("file1");
		ExpectNotification(MoveEntry("file1", "base/dir1/file2"),
			recursive && !directoriesOnly);
		ExpectNotification(RemoveEntry("base/dir1/file2"),
			recursive && !directoriesOnly);
	)

	CREATE_TEST(DirectoryMoveIntoSubLevel,
		CreateDirectory("dir2");
		ExpectNotification(MoveEntry("dir2", "base/dir1/dir3"),
			recursive && !filesOnly);
		ExpectNotification(RemoveEntry("base/dir1/dir3"),
			recursive && !filesOnly);
	)

	CREATE_TEST(FileMoveOutOfTopLevel,
		ExpectNotification(CreateFile("base/file1"),
			!directoriesOnly && !pathOnly);
		ExpectNotification(MoveEntry("base/file1", "file2"),
			!directoriesOnly && !pathOnly);
		RemoveEntry("file2");
	)

	CREATE_TEST(DirectoryMoveOutOfTopLevel,
		ExpectNotification(CreateDirectory("base/dir2"),
			!filesOnly && !pathOnly);
		ExpectNotification(MoveEntry("base/dir2", "dir3"),
			!filesOnly && !pathOnly);
		RemoveEntry("dir3");
	)

	CREATE_TEST(FileMoveOutOfSubLevel,
		ExpectNotification(CreateFile("base/dir1/file1"),
			recursive && !directoriesOnly);
		ExpectNotification(MoveEntry("base/dir1/file1", "file2"),
			recursive && !directoriesOnly);
		RemoveEntry("file2");
	)

	CREATE_TEST(DirectoryMoveOutOfSubLevel,
		ExpectNotification(CreateDirectory("base/dir1/dir2"),
			recursive && !filesOnly);
		ExpectNotification(MoveEntry("base/dir1/dir2", "dir3"),
			recursive && !filesOnly);
		RemoveEntry("dir3");
	)

	CREATE_TEST(FileMoveToTopLevel,
		ExpectNotification(CreateFile("base/dir1/file1"),
			!directoriesOnly && recursive);
		ExpectNotification(MoveEntry("base/dir1/file1", "base/file2"),
			!directoriesOnly && !pathOnly);
		ExpectNotification(RemoveEntry("base/file2"),
			!directoriesOnly && !pathOnly);
	)

	CREATE_TEST(DirectoryMoveToTopLevel,
		ExpectNotification(CreateDirectory("base/dir1/dir2"),
			!filesOnly && recursive);
		ExpectNotification(MoveEntry("base/dir1/dir2", "base/dir3"),
			!filesOnly && !pathOnly);
		ExpectNotification(RemoveEntry("base/dir3"),
			!filesOnly && !pathOnly);
	)

	CREATE_TEST(FileMoveToSubLevel,
		ExpectNotification(CreateFile("base/file1"),
			!directoriesOnly && !pathOnly);
		ExpectNotification(MoveEntry("base/file1", "base/dir1/file2"),
			!directoriesOnly && !pathOnly);
		ExpectNotification(RemoveEntry("base/dir1/file2"),
			!directoriesOnly && recursive);
	)

	CREATE_TEST(DirectoryMoveToSubLevel,
		ExpectNotification(CreateDirectory("base/dir2"),
			!filesOnly && !pathOnly);
		ExpectNotification(MoveEntry("base/dir2", "base/dir1/dir3"),
			!filesOnly && !pathOnly);
		ExpectNotification(RemoveEntry("base/dir1/dir3"),
			!filesOnly && recursive);
	)

	CREATE_TEST(NonEmptyDirectoryMoveIntoTopLevel,
		CreateDirectory("dir2");
		CreateDirectory("dir2/dir3");
		CreateDirectory("dir2/dir4");
		CreateFile("dir2/file1");
		CreateFile("dir2/dir3/file2");
		ExpectNotification(MoveEntry("dir2", "base/dir5"),
			!filesOnly && !pathOnly);
		if (recursive && filesOnly) {
			ExpectNotifications(MonitoringInfoSet()
				.Add(B_ENTRY_CREATED, "base/dir5/file1")
				.Add(B_ENTRY_CREATED, "base/dir5/dir3/file2"));
		}
	)

	CREATE_TEST(NonEmptyDirectoryMoveIntoSubLevel,
		CreateDirectory("dir2");
		CreateDirectory("dir2/dir3");
		CreateDirectory("dir2/dir4");
		CreateFile("dir2/file1");
		CreateFile("dir2/dir3/file2");
		ExpectNotification(MoveEntry("dir2", "base/dir1/dir5"),
			!filesOnly && recursive);
		if (recursive && filesOnly) {
			ExpectNotifications(MonitoringInfoSet()
				.Add(B_ENTRY_CREATED, "base/dir1/dir5/file1")
				.Add(B_ENTRY_CREATED, "base/dir1/dir5/dir3/file2"));
		}
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(NonEmptyDirectoryMoveOutOfTopLevel,
		StandardSetup();
		CreateDirectory("base/dir2");
		CreateDirectory("base/dir2/dir3");
		CreateDirectory("base/dir2/dir4");
		CreateFile("base/dir2/file1");
		CreateFile("base/dir2/dir3/file2");
		StartWatching("base");
		MonitoringInfoSet filesRemoved;
		if (recursive && filesOnly) {
			filesRemoved
				.Add(B_ENTRY_REMOVED, "base/dir2/file1")
				.Add(B_ENTRY_REMOVED, "base/dir2/dir3/file2");
		}
		ExpectNotification(MoveEntry("base/dir2", "dir5"),
			!filesOnly && !pathOnly);
		ExpectNotifications(filesRemoved);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(NonEmptyDirectoryMoveOutOfSubLevel,
		StandardSetup();
		CreateDirectory("base/dir1/dir2");
		CreateDirectory("base/dir1/dir2/dir3");
		CreateDirectory("base/dir1/dir2/dir4");
		CreateFile("base/dir1/dir2/file1");
		CreateFile("base/dir1/dir2/dir3/file2");
		StartWatching("base");
		MonitoringInfoSet filesRemoved;
		if (recursive && filesOnly) {
			filesRemoved
				.Add(B_ENTRY_REMOVED, "base/dir1/dir2/file1")
				.Add(B_ENTRY_REMOVED, "base/dir1/dir2/dir3/file2");
		}
		ExpectNotification(MoveEntry("base/dir1/dir2", "dir5"),
			!filesOnly && recursive);
		ExpectNotifications(filesRemoved);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(NonEmptyDirectoryMoveToTopLevel,
		StandardSetup();
		CreateDirectory("base/dir1/dir2");
		CreateDirectory("base/dir1/dir2/dir3");
		CreateDirectory("base/dir1/dir2/dir4");
		CreateFile("base/dir1/dir2/file1");
		CreateFile("base/dir1/dir2/dir3/file2");
		StartWatching("base");
		MonitoringInfoSet filesMoved;
		if (recursive && filesOnly) {
			filesMoved
				.Add(B_ENTRY_REMOVED, "base/dir1/dir2/file1")
				.Add(B_ENTRY_REMOVED, "base/dir1/dir2/dir3/file2");
		}
		ExpectNotification(MoveEntry("base/dir1/dir2", "base/dir5"),
			!filesOnly && !pathOnly);
		if (recursive && filesOnly) {
			filesMoved
				.Add(B_ENTRY_CREATED, "base/dir5/file1")
				.Add(B_ENTRY_CREATED, "base/dir5/dir3/file2");
		}
		ExpectNotifications(filesMoved);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(NonEmptyDirectoryMoveToSubLevel,
		StandardSetup();
		CreateDirectory("base/dir2");
		CreateDirectory("base/dir2/dir3");
		CreateDirectory("base/dir2/dir4");
		CreateFile("base/dir2/file1");
		CreateFile("base/dir2/dir3/file2");
		StartWatching("base");
		MonitoringInfoSet filesMoved;
		if (recursive && filesOnly) {
			filesMoved
				.Add(B_ENTRY_REMOVED, "base/dir2/file1")
				.Add(B_ENTRY_REMOVED, "base/dir2/dir3/file2");
		}
		ExpectNotification(MoveEntry("base/dir2", "base/dir1/dir5"),
			!filesOnly && !pathOnly);
		if (recursive && filesOnly) {
			filesMoved
				.Add(B_ENTRY_CREATED, "base/dir1/dir5/file1")
				.Add(B_ENTRY_CREATED, "base/dir1/dir5/dir3/file2");
		}
		ExpectNotifications(filesMoved);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(CreateAncestor,
		StartWatching("ancestor/base");
		CreateDirectory("ancestor");
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveCreateAncestor,
		CreateDirectory("ancestorSibling");
		StartWatching("ancestor/base");
		MoveEntry("ancestorSibling", "ancestor");
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveCreateAncestorWithBase,
		CreateDirectory("ancestorSibling");
		CreateDirectory("ancestorSibling/base");
		StartWatching("ancestor/base");
		MoveEntry("ancestorSibling", "ancestor");
		MonitoringInfoSet entriesCreated;
		if (!filesOnly)
			entriesCreated.Add(B_ENTRY_CREATED, "ancestor/base");
		ExpectNotifications(entriesCreated);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveCreateAncestorWithBaseAndFile,
		CreateDirectory("ancestorSibling");
		CreateDirectory("ancestorSibling/base");
		CreateFile("ancestorSibling/base/file1");
		StartWatching("ancestor/base");
		MoveEntry("ancestorSibling", "ancestor");
		MonitoringInfoSet entriesCreated;
		if (!filesOnly)
			entriesCreated.Add(B_ENTRY_CREATED, "ancestor/base");
		else if (!pathOnly)
			entriesCreated.Add(B_ENTRY_CREATED, "ancestor/base/file1");
		ExpectNotifications(entriesCreated);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveCreateAncestorWithBaseAndDirectory,
		CreateDirectory("ancestorSibling");
		CreateDirectory("ancestorSibling/base");
		CreateDirectory("ancestorSibling/base/dir1");
		CreateFile("ancestorSibling/base/dir1/file1");
		StartWatching("ancestor/base");
		MoveEntry("ancestorSibling", "ancestor");
		MonitoringInfoSet entriesCreated;
		if (!filesOnly) {
			entriesCreated.Add(B_ENTRY_CREATED, "ancestor/base");
		} else if (recursive)
			entriesCreated.Add(B_ENTRY_CREATED, "ancestor/base/dir1/file1");
		ExpectNotifications(entriesCreated);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(CreateBase,
		CreateDirectory("ancestor");
		StartWatching("ancestor/base");
		ExpectNotification(CreateDirectory("ancestor/base"),
			!filesOnly);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveCreateBase,
		CreateDirectory("ancestor");
		CreateDirectory("ancestor/baseSibling");
		StartWatching("ancestor/base");
		ExpectNotification(MoveEntry("ancestor/baseSibling", "ancestor/base"),
			!filesOnly);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveCreateBaseWithFile,
		CreateDirectory("ancestor");
		CreateDirectory("ancestor/baseSibling");
		CreateFile("ancestor/baseSibling/file1");
		StartWatching("ancestor/base");
		ExpectNotification(MoveEntry("ancestor/baseSibling", "ancestor/base"),
			!filesOnly);
		MonitoringInfoSet entriesCreated;
		if (filesOnly && !pathOnly)
			entriesCreated.Add(B_ENTRY_CREATED, "ancestor/base/file1");
		ExpectNotifications(entriesCreated);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveCreateBaseWithDirectory,
		CreateDirectory("ancestor");
		CreateDirectory("ancestor/baseSibling");
		CreateDirectory("ancestor/baseSibling/dir1");
		CreateFile("ancestor/baseSibling/dir1/file1");
		StartWatching("ancestor/base");
		ExpectNotification(MoveEntry("ancestor/baseSibling", "ancestor/base"),
			!filesOnly);
		MonitoringInfoSet entriesCreated;
		if (filesOnly && recursive)
			entriesCreated.Add(B_ENTRY_CREATED, "ancestor/base/dir1/file1");
		ExpectNotifications(entriesCreated);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveRemoveAncestorWithBaseAndFile,
		CreateDirectory("ancestor");
		CreateDirectory("ancestor/base");
		CreateFile("ancestor/base/file1");
		StartWatching("ancestor/base");
		MonitoringInfoSet entriesRemoved;
		if (!filesOnly)
			entriesRemoved.Add(B_ENTRY_REMOVED, "ancestor/base");
		else if (!pathOnly)
			entriesRemoved.Add(B_ENTRY_REMOVED, "ancestor/base/file1");
		MoveEntry("ancestor", "ancestorSibling");
		ExpectNotifications(entriesRemoved);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveRemoveAncestorWithBaseAndDirectory,
		CreateDirectory("ancestor");
		CreateDirectory("ancestor/base");
		CreateDirectory("ancestor/base/dir1");
		CreateFile("ancestor/base/dir1/file1");
		StartWatching("ancestor/base");
		MonitoringInfoSet entriesRemoved;
		if (!filesOnly)
			entriesRemoved.Add(B_ENTRY_REMOVED, "ancestor/base");
		else if (recursive)
			entriesRemoved.Add(B_ENTRY_REMOVED, "ancestor/base/dir1/file1");
		MoveEntry("ancestor", "ancestorSibling");
		ExpectNotifications(entriesRemoved);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveRemoveBaseWithFile,
		CreateDirectory("ancestor");
		CreateDirectory("ancestor/base");
		CreateFile("ancestor/base/file1");
		StartWatching("ancestor/base");
		MonitoringInfoSet entriesRemoved;
		if (filesOnly && !pathOnly)
			entriesRemoved.Add(B_ENTRY_REMOVED, "ancestor/base/file1");
		ExpectNotification(MoveEntry("ancestor/base", "ancestor/baseSibling"),
			!filesOnly);
		ExpectNotifications(entriesRemoved);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveRemoveBaseWithDirectory,
		CreateDirectory("ancestor");
		CreateDirectory("ancestor/base");
		CreateDirectory("ancestor/base/dir1");
		CreateFile("ancestor/base/dir1/file1");
		StartWatching("ancestor/base");
		MonitoringInfoSet entriesRemoved;
		if (filesOnly && recursive)
			entriesRemoved.Add(B_ENTRY_REMOVED, "ancestor/base/dir1/file1");
		ExpectNotification(MoveEntry("ancestor/base", "ancestor/baseSibling"),
			!filesOnly);
		ExpectNotifications(entriesRemoved);
	)

	CREATE_TEST(TouchBase,
		ExpectNotification(TouchEntry("base"), watchStat && !filesOnly);
	)

	CREATE_TEST(TouchFileTopLevel,
		ExpectNotification(TouchEntry("base/file0"),
			watchStat && recursive && !directoriesOnly);
	)

	CREATE_TEST(TouchFileSubLevel,
		ExpectNotification(TouchEntry("base/dir1/file0.0"),
			watchStat && recursive && !directoriesOnly);
	)

	CREATE_TEST(TouchDirectoryTopLevel,
		ExpectNotification(TouchEntry("base/dir1"),
			watchStat && recursive && !filesOnly);
	)

	CREATE_TEST(TouchDirectorySubLevel,
		ExpectNotification(TouchEntry("base/dir1/dir0"),
			watchStat && recursive && !filesOnly);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(CreateFileBase,
		StartWatching("file");
		ExpectNotification(CreateFile("file"),
			!directoriesOnly);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveCreateFileBase,
		CreateFile("fileSibling");
		StartWatching("file");
		ExpectNotification(MoveEntry("fileSibling", "file"),
			!directoriesOnly);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(RemoveFileBase,
		CreateFile("file");
		StartWatching("file");
		ExpectNotification(RemoveEntry("file"),
			!directoriesOnly);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(MoveRemoveFileBase,
		CreateFile("file");
		StartWatching("file");
		ExpectNotification(MoveEntry("file", "fileSibling"),
			!directoriesOnly);
	)

	CREATE_TEST_WITH_CUSTOM_SETUP(TouchFileBase,
		CreateFile("file");
		StartWatching("file");
		ExpectNotification(TouchEntry("file"),
			watchStat && !directoriesOnly);
	)
}


static void
run_tests(std::set<BString> testNames, uint32 watchFlags,
	size_t& totalTests, size_t& succeededTests)
{
	std::vector<Test*> tests;
	create_tests(tests);

	// filter the tests, if test names have been specified
	size_t testCount = tests.size();
	if (!testNames.empty()) {
		for (size_t i = 0; i < testCount;) {
			Test* test = tests[i];
			std::set<BString>::iterator it = testNames.find(test->Name());
			if (it != testNames.end()) {
				testNames.erase(it);
				i++;
			} else {
				tests.erase(tests.begin() + i);
				test->Delete();
				testCount--;
			}
		}

		if (!testNames.empty()) {
			printf("no such test(s):\n");
			for (std::set<BString>::iterator it = testNames.begin();
				it != testNames.end(); ++it) {
				printf("  %s\n", it->String());
				exit(1);
			}
		}
	}

	printf("\nrunning tests with flags: %s\n",
		watch_flags_to_string(watchFlags).String());

	int32 longestTestName = 0;

	for (size_t i = 0; i < testCount; i++) {
		Test* test = tests[i];
		longestTestName = std::max(longestTestName, test->Name().Length());
	}

	for (size_t i = 0; i < testCount; i++) {
		Test* test = tests[i];
		bool terminate = false;

		try {
			totalTests++;
			test->Init(watchFlags);
			printf("  %s: %*s", test->Name().String(),
				int(longestTestName - test->Name().Length()), "");
			fflush(stdout);
			test->Do();
			printf("SUCCEEDED\n");
			succeededTests++;
		} catch (FatalException& exception) {
			printf("FAILED FATALLY\n");
			printf("%s\n",
				indented_string(exception.Message(), "    ").String());
			terminate = true;
		} catch (TestException& exception) {
			printf("FAILED\n");
			printf("%s\n",
				indented_string(exception.Message(), "    ").String());
		}

		test->Delete();

		if (terminate)
			exit(1);
	}
}


int
main(int argc, const char* const* argv)
{
	// any args are test names
	std::set<BString> testNames;
	for (int i = 1; i < argc; i++)
		testNames.insert(argv[i]);

	// flags that can be combined arbitrarily
	const uint32 kFlags[] = {
		B_WATCH_NAME,
		B_WATCH_STAT,
		// not that interesting, since similar to B_WATCH_STAT: B_WATCH_ATTR
		B_WATCH_DIRECTORY,
		B_WATCH_RECURSIVELY,
	};
	const size_t kFlagCount = sizeof(kFlags) / sizeof(kFlags[0]);

	size_t totalTests = 0;
	size_t succeededTests = 0;

	for (size_t i = 0; i < 1 << kFlagCount; i++) {
		// construct flags mask
		uint32 flags = 0;
		for (size_t k = 0; k < kFlagCount; k++) {
			if ((i & (1 << k)) != 0)
				flags |= kFlags[k];
		}

		// run tests -- in recursive mode do that additionally for the mutually
		// B_WATCH_FILES_ONLY and B_WATCH_DIRECTORIES_ONLY flags.
		run_tests(testNames, flags, totalTests, succeededTests);
		if ((flags & B_WATCH_RECURSIVELY) != 0) {
			run_tests(testNames, flags | B_WATCH_FILES_ONLY, totalTests,
				succeededTests);
			run_tests(testNames, flags | B_WATCH_DIRECTORIES_ONLY, totalTests,
				succeededTests);
		}
	}

	printf("\n");
	if (succeededTests == totalTests) {
		printf("ALL TESTS SUCCEEDED\n");
	} else {
		printf("%zu of %zu TESTS FAILED\n", totalTests - succeededTests,
			totalTests);
	}

	return 0;
}
