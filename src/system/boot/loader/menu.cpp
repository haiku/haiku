/*
 * Copyright 2003-2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Copyright 2013-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "menu.h"

#include <errno.h>
#include <strings.h>

#include <algorithm>

#include <OS.h>

#include <AutoDeleter.h>
#include <boot/menu.h>
#include <boot/PathBlacklist.h>
#include <boot/stage2.h>
#include <boot/vfs.h>
#include <boot/platform.h>
#include <boot/platform/generic/text_console.h>
#include <boot/stdio.h>
#include <safemode.h>
#include <util/ring_buffer.h>
#include <util/SinglyLinkedList.h>

#include "kernel_debug_config.h"

#include "load_driver_settings.h"
#include "loader.h"
#include "package_support.h"
#include "pager.h"
#include "RootFileSystem.h"


//#define TRACE_MENU
#ifdef TRACE_MENU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// only set while in user_menu()
static Menu* sMainMenu = NULL;
static Menu* sBlacklistRootMenu = NULL;
static BootVolume* sBootVolume = NULL;
static PathBlacklist* sPathBlacklist;


MenuItem::MenuItem(const char *label, Menu *subMenu)
	:
	fLabel(strdup(label)),
	fTarget(NULL),
	fIsMarked(false),
	fIsSelected(false),
	fIsEnabled(true),
	fType(MENU_ITEM_STANDARD),
	fMenu(NULL),
	fSubMenu(NULL),
	fData(NULL),
	fHelpText(NULL),
	fShortcut(0)
{
	SetSubmenu(subMenu);
}


MenuItem::~MenuItem()
{
	delete fSubMenu;
	free(const_cast<char *>(fLabel));
}


void
MenuItem::SetTarget(menu_item_hook target)
{
	fTarget = target;
}


/**	Marks or unmarks a menu item. A marked menu item usually gets a visual
 *	clue like a checkmark that distinguishes it from others.
 *	For menus of type CHOICE_MENU, there can only be one marked item - the
 *	chosen one.
 */

void
MenuItem::SetMarked(bool marked)
{
	if (marked && fMenu != NULL && fMenu->Type() == CHOICE_MENU) {
		// always set choice text of parent if we were marked
		fMenu->SetChoiceText(Label());
	}

	if (fIsMarked == marked)
		return;

	if (marked && fMenu != NULL && fMenu->Type() == CHOICE_MENU) {
		// unmark previous item
		MenuItem *markedItem = fMenu->FindMarked();
		if (markedItem != NULL)
			markedItem->SetMarked(false);
	}

	fIsMarked = marked;

	if (fMenu != NULL)
		fMenu->Draw(this);
}


void
MenuItem::Select(bool selected)
{
	if (fIsSelected == selected)
		return;

	if (selected && fMenu != NULL) {
		// unselect previous item
		MenuItem *selectedItem = fMenu->FindSelected();
		if (selectedItem != NULL)
			selectedItem->Select(false);
	}

	fIsSelected = selected;

	if (fMenu != NULL)
		fMenu->Draw(this);
}


void
MenuItem::SetType(menu_item_type type)
{
	fType = type;
}


void
MenuItem::SetEnabled(bool enabled)
{
	if (fIsEnabled == enabled)
		return;

	fIsEnabled = enabled;

	if (fMenu != NULL)
		fMenu->Draw(this);
}


void
MenuItem::SetData(const void *data)
{
	fData = data;
}


/*!	This sets a help text that is shown when the item is
	selected.
	Note, unlike the label, the string is not copied, it's
	just referenced and has to stay valid as long as the
	item's menu is being used.
*/
void
MenuItem::SetHelpText(const char* text)
{
	fHelpText = text;
}


void
MenuItem::SetShortcut(char key)
{
	fShortcut = key;
}


void
MenuItem::SetLabel(const char* label)
{
	if (char* newLabel = strdup(label)) {
		free(const_cast<char*>(fLabel));
		fLabel = newLabel;
	}
}


void
MenuItem::SetSubmenu(Menu* subMenu)
{
	fSubMenu = subMenu;

	if (fSubMenu != NULL)
		fSubMenu->fSuperItem = this;
}


void
MenuItem::SetMenu(Menu* menu)
{
	fMenu = menu;
}


//	#pragma mark -


Menu::Menu(menu_type type, const char* title)
	:
	fTitle(title),
	fChoiceText(NULL),
	fCount(0),
	fIsHidden(true),
	fType(type),
	fSuperItem(NULL),
	fShortcuts(NULL)
{
}


Menu::~Menu()
{
	// take all remaining items with us

	MenuItem *item;
	while ((item = fItems.Head()) != NULL) {
		fItems.Remove(item);
		delete item;
	}
}


void
Menu::Entered()
{
}


void
Menu::Exited()
{
}


MenuItem*
Menu::ItemAt(int32 index)
{
	if (index < 0 || index >= fCount)
		return NULL;

	MenuItemIterator iterator = ItemIterator();
	MenuItem *item;

	while ((item = iterator.Next()) != NULL) {
		if (index-- == 0)
			return item;
	}

	return NULL;
}


int32
Menu::IndexOf(MenuItem* searchedItem)
{
	int32 index = 0;

	MenuItemIterator iterator = ItemIterator();
	while (MenuItem* item = iterator.Next()) {
		if (item == searchedItem)
			return index;

		index++;
	}

	return -1;
}


int32
Menu::CountItems() const
{
	return fCount;
}


MenuItem*
Menu::FindItem(const char* label)
{
	MenuItemIterator iterator = ItemIterator();
	while (MenuItem* item = iterator.Next()) {
		if (item->Label() != NULL && !strcmp(item->Label(), label))
			return item;
	}

	return NULL;
}


MenuItem*
Menu::FindMarked()
{
	MenuItemIterator iterator = ItemIterator();
	while (MenuItem* item = iterator.Next()) {
		if (item->IsMarked())
			return item;
	}

	return NULL;
}


MenuItem*
Menu::FindSelected(int32* _index)
{
	int32 index = 0;

	MenuItemIterator iterator = ItemIterator();
	while (MenuItem* item = iterator.Next()) {
		if (item->IsSelected()) {
			if (_index != NULL)
				*_index = index;
			return item;
		}

		index++;
	}

	return NULL;
}


void
Menu::AddItem(MenuItem* item)
{
	item->fMenu = this;
	fItems.Add(item);
	fCount++;
}


status_t
Menu::AddSeparatorItem()
{
	MenuItem* item = new(std::nothrow) MenuItem();
	if (item == NULL)
		return B_NO_MEMORY;

	item->SetType(MENU_ITEM_SEPARATOR);

	AddItem(item);
	return B_OK;
}


MenuItem*
Menu::RemoveItemAt(int32 index)
{
	if (index < 0 || index >= fCount)
		return NULL;

	MenuItemIterator iterator = ItemIterator();
	while (MenuItem* item = iterator.Next()) {
		if (index-- == 0) {
			RemoveItem(item);
			return item;
		}
	}

	return NULL;
}


void
Menu::RemoveItem(MenuItem* item)
{
	item->fMenu = NULL;
	fItems.Remove(item);
	fCount--;
}


void
Menu::AddShortcut(char key, shortcut_hook function)
{
	Menu::shortcut* shortcut = new(std::nothrow) Menu::shortcut;
	if (shortcut == NULL)
		return;

	shortcut->key = key;
	shortcut->function = function;

	shortcut->next = fShortcuts;
	fShortcuts = shortcut;
}


shortcut_hook
Menu::FindShortcut(char key) const
{
	if (key == 0)
		return NULL;

	const Menu::shortcut* shortcut = fShortcuts;
	while (shortcut != NULL) {
		if (shortcut->key == key)
			return shortcut->function;

		shortcut = shortcut->next;
	}

	Menu *superMenu = Supermenu();

	if (superMenu != NULL)
		return superMenu->FindShortcut(key);

	return NULL;
}


MenuItem*
Menu::FindItemByShortcut(char key)
{
	if (key == 0)
		return NULL;

	MenuItemList::Iterator iterator = ItemIterator();
	while (MenuItem* item = iterator.Next()) {
		if (item->Shortcut() == key)
			return item;
	}

	Menu *superMenu = Supermenu();

	if (superMenu != NULL)
		return superMenu->FindItemByShortcut(key);

	return NULL;
}


void
Menu::SortItems(bool (*less)(const MenuItem*, const MenuItem*))
{
	fItems.Sort(less);
}


void
Menu::Run()
{
	platform_run_menu(this);
}


void
Menu::Draw(MenuItem* item)
{
	if (!IsHidden())
		platform_update_menu_item(this, item);
}


//	#pragma mark -


static const char*
size_to_string(off_t size, char* buffer, size_t bufferSize)
{
	static const char* const kPrefixes[] = { "K", "M", "G", "T", "P", NULL };
	int32 nextIndex = 0;
	int32 remainder = 0;
	while (size >= 1024 && kPrefixes[nextIndex] != NULL) {
		remainder = size % 1024;
		size /= 1024;
		nextIndex++;

		if (size < 1024) {
			// Compute the decimal remainder and make sure we have at most
			// 3 decimal places (or 4 for 1000 <= size <= 1023).
			int32 factor;
			if (size >= 100)
				factor = 100;
			else if (size >= 10)
				factor = 10;
			else
				factor = 1;

			remainder = (remainder * 1000 + 5 * factor) / 1024;

			if (remainder >= 1000) {
				size++;
				remainder = 0;
			} else
				remainder /= 10 * factor;
		} else
			size += (remainder + 512) / 1024;
	}

	if (remainder == 0) {
		snprintf(buffer, bufferSize, "%" B_PRIdOFF, size);
	} else {
		snprintf(buffer, bufferSize, "%" B_PRIdOFF ".%" B_PRId32, size,
			remainder);
	}

	size_t length = strlen(buffer);
	snprintf(buffer + length, bufferSize - length, " %sB",
		nextIndex == 0 ? "" : kPrefixes[nextIndex - 1]);

	return buffer;
}


// #pragma mark - blacklist menu


class BlacklistMenuItem : public MenuItem {
public:
	BlacklistMenuItem(char* label, Node* node, Menu* subMenu)
		:
		MenuItem(label, subMenu),
		fNode(node),
		fSubMenu(subMenu)
	{
		fNode->Acquire();
		SetType(MENU_ITEM_MARKABLE);
	}

	~BlacklistMenuItem()
	{
		fNode->Release();

		// make sure the submenu is destroyed
		SetSubmenu(fSubMenu);
	}

	bool IsDirectoryItem() const
	{
		return fNode->Type() == S_IFDIR;
	}

	bool GetPath(BlacklistedPath& _path) const
	{
		Menu* menu = Supermenu();
		if (menu != NULL && menu != sBlacklistRootMenu
			&& menu->Superitem() != NULL) {
			return static_cast<BlacklistMenuItem*>(menu->Superitem())
					->GetPath(_path)
			   && _path.Append(Label());
		}

		return _path.SetTo(Label());
	}

	void UpdateBlacklisted()
	{
		BlacklistedPath path;
		if (GetPath(path))
			_SetMarked(sPathBlacklist->Contains(path.Path()), false);
	}

	virtual void SetMarked(bool marked)
	{
		_SetMarked(marked, true);
	}

	static bool Less(const MenuItem* a, const MenuItem* b)
	{
		const BlacklistMenuItem* item1
			= static_cast<const BlacklistMenuItem*>(a);
		const BlacklistMenuItem* item2
			= static_cast<const BlacklistMenuItem*>(b);

		// directories come first
		if (item1->IsDirectoryItem() != item2->IsDirectoryItem())
			return item1->IsDirectoryItem();

		// compare the labels
		return strcasecmp(item1->Label(), item2->Label()) < 0;
	}

private:
	void _SetMarked(bool marked, bool updateBlacklist)
	{
		if (marked == IsMarked())
			return;

		// For directories toggle the availability of the submenu.
		if (IsDirectoryItem())
			SetSubmenu(marked ? NULL : fSubMenu);

		if (updateBlacklist) {
			BlacklistedPath path;
			if (GetPath(path)) {
				if (marked)
					sPathBlacklist->Add(path.Path());
				else
					sPathBlacklist->Remove(path.Path());
			}
		}

		MenuItem::SetMarked(marked);
	}

private:
	Node*	fNode;
	Menu*	fSubMenu;
};


class BlacklistMenu : public Menu {
public:
	BlacklistMenu()
		:
		Menu(STANDARD_MENU, kDefaultMenuTitle),
		fDirectory(NULL)
	{
	}

	~BlacklistMenu()
	{
		SetDirectory(NULL);
	}

	virtual void Entered()
	{
		_DeleteItems();

		if (fDirectory != NULL) {
			void* cookie;
			if (fDirectory->Open(&cookie, O_RDONLY) == B_OK) {
				Node* node;
				while (fDirectory->GetNextNode(cookie, &node) == B_OK) {
					BlacklistMenuItem* item = _CreateItem(node);
					node->Release();
					if (item == NULL)
						break;

					AddItem(item);

					item->UpdateBlacklisted();
				}
				fDirectory->Close(cookie);
			}

			SortItems(&BlacklistMenuItem::Less);
		}

		if (CountItems() > 0)
			AddSeparatorItem();
		AddItem(new(nothrow) MenuItem("Return to parent directory"));
	}

	virtual void Exited()
	{
		_DeleteItems();
	}

protected:
	void SetDirectory(Directory* directory)
	{
		if (fDirectory != NULL)
			fDirectory->Release();

		fDirectory = directory;

		if (fDirectory != NULL)
			fDirectory->Acquire();
	}

private:
	static BlacklistMenuItem* _CreateItem(Node* node)
	{
		// Get the node name and duplicate it, so we can use it as a label.
		char name[B_FILE_NAME_LENGTH];
		if (node->GetName(name, sizeof(name)) != B_OK)
			return NULL;

		// append '/' to directory labels
		bool isDirectory = node->Type() == S_IFDIR;
		if (isDirectory)
			strlcat(name, "/", sizeof(name));

		// If this is a directory, create the submenu.
		BlacklistMenu* subMenu = NULL;
		if (isDirectory) {
			subMenu = new(std::nothrow) BlacklistMenu;
			if (subMenu != NULL)
				subMenu->SetDirectory(static_cast<Directory*>(node));

		}
		ObjectDeleter<BlacklistMenu> subMenuDeleter(subMenu);

		// create the menu item
		BlacklistMenuItem* item = new(std::nothrow) BlacklistMenuItem(name,
			node, subMenu);
		if (item == NULL)
			return NULL;

		subMenuDeleter.Detach();
		return item;
	}

	void _DeleteItems()
	{
		int32 count = CountItems();
		for (int32 i = 0; i < count; i++)
			delete RemoveItemAt(0);
	}

private:
	Directory*	fDirectory;

protected:
	static const char* const kDefaultMenuTitle;
};


const char* const BlacklistMenu::kDefaultMenuTitle
	= "Mark the entries to blacklist";


class BlacklistRootMenu : public BlacklistMenu {
public:
	BlacklistRootMenu()
		:
		BlacklistMenu()
	{
	}

	virtual void Entered()
	{
		// Get the system directory, but only if this is a packaged Haiku.
		// Otherwise blacklisting isn't supported.
		if (sBootVolume != NULL && sBootVolume->IsValid()
			&& sBootVolume->IsPackaged()) {
			SetDirectory(sBootVolume->SystemDirectory());
			SetTitle(kDefaultMenuTitle);
		} else {
			SetDirectory(NULL);
			SetTitle(sBootVolume != NULL && sBootVolume->IsValid()
				? "The selected boot volume doesn't support blacklisting!"
				: "No boot volume selected!");
		}

		BlacklistMenu::Entered();

		// rename last item
		if (MenuItem* item = ItemAt(CountItems() - 1))
			item->SetLabel("Return to safe mode menu");
	}

	virtual void Exited()
	{
		BlacklistMenu::Exited();
		SetDirectory(NULL);
	}
};


// #pragma mark - boot volume menu


class BootVolumeMenuItem : public MenuItem {
public:
	BootVolumeMenuItem(const char* volumeName)
		:
		MenuItem(volumeName),
		fStateChoiceText(NULL)
	{
	}

	~BootVolumeMenuItem()
	{
		UpdateStateName(NULL);
	}

	void UpdateStateName(PackageVolumeState* volumeState)
	{
		free(fStateChoiceText);
		fStateChoiceText = NULL;

		if (volumeState != NULL && volumeState->Name() != NULL) {
			char nameBuffer[128];
			snprintf(nameBuffer, sizeof(nameBuffer), "%s (%s)", Label(),
				volumeState->DisplayName());
			fStateChoiceText = strdup(nameBuffer);
		}

		Supermenu()->SetChoiceText(
			fStateChoiceText != NULL ? fStateChoiceText : Label());
	}

private:
	char*	fStateChoiceText;
};


class PackageVolumeStateMenuItem : public MenuItem {
public:
	PackageVolumeStateMenuItem(const char* label, PackageVolumeInfo* volumeInfo,
		PackageVolumeState*	volumeState)
		:
		MenuItem(label),
		fVolumeInfo(volumeInfo),
		fVolumeState(volumeState)
	{
		fVolumeInfo->AcquireReference();
	}

	~PackageVolumeStateMenuItem()
	{
		fVolumeInfo->ReleaseReference();
	}

	PackageVolumeInfo* VolumeInfo() const
	{
		return fVolumeInfo;
	}

	PackageVolumeState* VolumeState() const
	{
		return fVolumeState;
	}

private:
	PackageVolumeInfo*	fVolumeInfo;
	PackageVolumeState*	fVolumeState;
};


// #pragma mark -


class StringBuffer {
public:
	StringBuffer()
		:
		fBuffer(NULL),
		fLength(0),
		fCapacity(0)
	{
	}

	~StringBuffer()
	{
		free(fBuffer);
	}

	const char* String() const
	{
		return fBuffer != NULL ? fBuffer : "";
	}

	size_t Length() const
	{
		return fLength;
	}

	bool Append(const char* toAppend)
	{
		return Append(toAppend, strlen(toAppend));
	}

	bool Append(const char* toAppend, size_t length)
	{
		size_t oldLength = fLength;
		if (!_Resize(fLength + length))
			return false;

		memcpy(fBuffer + oldLength, toAppend, length);
		return true;
	}

private:
	bool _Resize(size_t newLength)
	{
		if (newLength >= fCapacity) {
			size_t newCapacity = std::max(fCapacity, size_t(32));
			while (newLength >= newCapacity)
				newCapacity *= 2;

			char* buffer = (char*)realloc(fBuffer, newCapacity);
			if (buffer == NULL)
				return false;

			fBuffer = buffer;
			fCapacity = newCapacity;
		}

		fBuffer[newLength] = '\0';
		fLength = newLength;
		return true;
	}

private:
	char*	fBuffer;
	size_t	fLength;
	size_t	fCapacity;
};


// #pragma mark -


static StringBuffer sSafeModeOptionsBuffer;


static MenuItem*
get_continue_booting_menu_item()
{
	// It's the last item in the main menu.
	if (sMainMenu == NULL || sMainMenu->CountItems() == 0)
		return NULL;
	return sMainMenu->ItemAt(sMainMenu->CountItems() - 1);
}


static void
update_continue_booting_menu_item(status_t status)
{
	MenuItem* bootItem = get_continue_booting_menu_item();
	if (bootItem == NULL) {
		// huh?
		return;
	}

	if (status == B_OK) {
		bootItem->SetLabel("Continue booting");
		bootItem->SetEnabled(true);
		bootItem->Select(true);
	} else {
		char label[128];
		snprintf(label, sizeof(label), "Cannot continue booting (%s)",
			strerror(status));
		bootItem->SetLabel(label);
		bootItem->SetEnabled(false);
	}
}


static bool
user_menu_boot_volume(Menu* menu, MenuItem* item)
{
	if (sBootVolume->IsValid() && sBootVolume->RootDirectory() == item->Data())
		return true;

	sPathBlacklist->MakeEmpty();

	status_t status = sBootVolume->SetTo((Directory*)item->Data());
	update_continue_booting_menu_item(status);

	gBootVolume.SetBool(BOOT_VOLUME_USER_SELECTED, true);
	return true;
}


static bool
user_menu_boot_volume_state(Menu* menu, MenuItem* _item)
{
	PackageVolumeStateMenuItem* item = static_cast<PackageVolumeStateMenuItem*>(
		_item);
	if (sBootVolume->IsValid() && sBootVolume->GetPackageVolumeState() != NULL
			&& sBootVolume->GetPackageVolumeState() == item->VolumeState()) {
		return true;
	}

	BootVolumeMenuItem* volumeItem = static_cast<BootVolumeMenuItem*>(
		item->Supermenu()->Superitem());
	volumeItem->SetMarked(true);
	volumeItem->Select(true);
	volumeItem->UpdateStateName(item->VolumeState());

	sPathBlacklist->MakeEmpty();

	status_t status = sBootVolume->SetTo((Directory*)item->Data(),
		item->VolumeInfo(), item->VolumeState());
	update_continue_booting_menu_item(status);

	gBootVolume.SetBool(BOOT_VOLUME_USER_SELECTED, true);
	return true;
}


static bool
debug_menu_display_current_log(Menu* menu, MenuItem* item)
{
	// get the buffer
	size_t bufferSize;
	const char* buffer = platform_debug_get_log_buffer(&bufferSize);
	if (buffer == NULL || bufferSize == 0)
		return true;

	struct TextSource : PagerTextSource {
		TextSource(const char* buffer, size_t size)
			:
			fBuffer(buffer),
			fSize(strnlen(buffer, size))
		{
		}

		virtual size_t BytesAvailable() const
		{
			return fSize;
		}

		virtual size_t Read(size_t offset, void* buffer, size_t size) const
		{
			if (offset >= fSize)
				return 0;

			if (size > fSize - offset)
				size = fSize - offset;

			memcpy(buffer, fBuffer + offset, size);
			return size;
		}

	private:
		const char*	fBuffer;
		size_t		fSize;
	};

	pager(TextSource(buffer, bufferSize));

	return true;
}


static bool
debug_menu_display_previous_syslog(Menu* menu, MenuItem* item)
{
	ring_buffer* buffer = (ring_buffer*)gKernelArgs.debug_output.Pointer();
	if (buffer == NULL)
		return true;

	struct TextSource : PagerTextSource {
		TextSource(ring_buffer* buffer)
			:
			fBuffer(buffer)
		{
		}

		virtual size_t BytesAvailable() const
		{
			return ring_buffer_readable(fBuffer);
		}

		virtual size_t Read(size_t offset, void* buffer, size_t size) const
		{
			return ring_buffer_peek(fBuffer, offset, buffer, size);
		}

	private:
		ring_buffer*	fBuffer;
	};

	pager(TextSource(buffer));

	return true;
}


static status_t
save_previous_syslog_to_volume(Directory* directory)
{
	// find an unused name
	char name[16];
	bool found = false;
	for (int i = 0; i < 99; i++) {
		snprintf(name, sizeof(name), "SYSLOG%02d.TXT", i);
		Node* node = directory->Lookup(name, false);
		if (node == NULL) {
			found = true;
			break;
		}

		node->Release();
	}

	if (!found) {
		printf("Failed to find an unused name for the syslog file!\n");
		return B_ERROR;
	}

	printf("Writing syslog to file \"%s\" ...\n", name);

	int fd = open_from(directory, name, O_RDWR | O_CREAT | O_EXCL, 0644);
	if (fd < 0) {
		printf("Failed to create syslog file!\n");
		return fd;
	}

	ring_buffer* syslogBuffer
		= (ring_buffer*)gKernelArgs.debug_output.Pointer();
	iovec vecs[2];
	int32 vecCount = ring_buffer_get_vecs(syslogBuffer, vecs);
	if (vecCount > 0) {
		size_t toWrite = ring_buffer_readable(syslogBuffer);

		ssize_t written = writev(fd, vecs, vecCount);
		if (written < 0 || (size_t)written != toWrite) {
			printf("Failed to write to the syslog file \"%s\"!\n", name);
			close(fd);
			return errno;
		}
	}

	close(fd);

	printf("Successfully wrote syslog file.\n");

	return B_OK;
}


static bool
debug_menu_add_advanced_option(Menu* menu, MenuItem* item)
{
	char buffer[256];

	size_t size = platform_get_user_input_text(menu, item, buffer,
		sizeof(buffer) - 1);

	if (size > 0) {
		buffer[size] = '\n';
		if (!sSafeModeOptionsBuffer.Append(buffer)) {
			dprintf("debug_menu_add_advanced_option(): failed to append option "
				"to buffer\n");
		}
	}

	return true;
}


static bool
debug_menu_toggle_debug_syslog(Menu* menu, MenuItem* item)
{
	gKernelArgs.keep_debug_output_buffer = item->IsMarked();
	return true;
}


static bool
debug_menu_toggle_previous_debug_syslog(Menu* menu, MenuItem* item)
{
	gKernelArgs.previous_debug_size = item->IsMarked();
	return true;
}


static bool
debug_menu_save_previous_syslog(Menu* menu, MenuItem* item)
{
	Directory* volume = (Directory*)item->Data();

	console_clear_screen();

	save_previous_syslog_to_volume(volume);

	printf("\nPress any key to continue\n");
	console_wait_for_key();

	return true;
}


static void
add_boot_volume_item(Menu* menu, Directory* volume, const char* name)
{
	BReference<PackageVolumeInfo> volumeInfo;
	PackageVolumeState* selectedState = NULL;
	if (volume == sBootVolume->RootDirectory()) {
		volumeInfo.SetTo(sBootVolume->GetPackageVolumeInfo());
		selectedState = sBootVolume->GetPackageVolumeState();
	} else {
		volumeInfo.SetTo(new(std::nothrow) PackageVolumeInfo);
		if (volumeInfo->SetTo(volume, "system/packages") == B_OK)
			selectedState = volumeInfo->States().Head();
		else
			volumeInfo.Unset();
	}

	BootVolumeMenuItem* item = new(nothrow) BootVolumeMenuItem(name);
	menu->AddItem(item);

	Menu* subMenu = NULL;

	if (volumeInfo != NULL && volumeInfo->LoadOldStates() == B_OK) {
		subMenu = new(std::nothrow) Menu(CHOICE_MENU, "Select Haiku version");

		for (PackageVolumeStateList::ConstIterator it
				= volumeInfo->States().GetIterator();
			PackageVolumeState* state = it.Next();) {
			PackageVolumeStateMenuItem* stateItem
				= new(nothrow) PackageVolumeStateMenuItem(state->DisplayName(),
					volumeInfo, state);
			subMenu->AddItem(stateItem);
			stateItem->SetTarget(user_menu_boot_volume_state);
			stateItem->SetData(volume);

			if (state == selectedState) {
				stateItem->SetMarked(true);
				stateItem->Select(true);
				item->UpdateStateName(stateItem->VolumeState());
			}
		}
	}

	if (subMenu != NULL && subMenu->CountItems() > 1) {
		item->SetHelpText(
			"Enter to choose a different state to boot");
		item->SetSubmenu(subMenu);
	} else {
		delete subMenu;
		item->SetTarget(user_menu_boot_volume);
		item->SetData(volume);
	}

	if (volume == sBootVolume->RootDirectory()) {
		item->SetMarked(true);
		item->Select(true);
	}
}


static Menu*
add_boot_volume_menu()
{
	Menu* menu = new(std::nothrow) Menu(CHOICE_MENU, "Select Boot Volume");
	MenuItem* item;
	void* cookie;
	int32 count = 0;

	if (gRoot->Open(&cookie, O_RDONLY) == B_OK) {
		Directory* volume;
		while (gRoot->GetNextNode(cookie, (Node**)&volume) == B_OK) {
			// only list bootable volumes
			if (volume != sBootVolume->RootDirectory() && !is_bootable(volume))
				continue;

			char name[B_FILE_NAME_LENGTH];
			if (volume->GetName(name, sizeof(name)) == B_OK) {
				add_boot_volume_item(menu, volume, name);

				count++;
			}
		}
		gRoot->Close(cookie);
	}

	if (count == 0) {
		// no boot volume found yet
		menu->AddItem(item = new(nothrow) MenuItem("<No boot volume found>"));
		item->SetType(MENU_ITEM_NO_CHOICE);
		item->SetEnabled(false);
	}

	menu->AddSeparatorItem();

	menu->AddItem(item = new(nothrow) MenuItem("Rescan volumes"));
	item->SetHelpText("Please insert a Haiku CD-ROM or attach a USB disk - "
		"depending on your system, you can then boot from there.");
	item->SetType(MENU_ITEM_NO_CHOICE);
	if (count == 0)
		item->Select(true);

	menu->AddItem(item = new(nothrow) MenuItem("Return to main menu"));
	item->SetType(MENU_ITEM_NO_CHOICE);

	if (gBootVolume.GetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE, false))
		menu->SetChoiceText("CD-ROM or hard drive");

	return menu;
}


static Menu*
add_safe_mode_menu()
{
	Menu* safeMenu = new(nothrow) Menu(SAFE_MODE_MENU, "Safe Mode Options");
	MenuItem* item;

	safeMenu->AddItem(item = new(nothrow) MenuItem("Safe mode"));
	item->SetData(B_SAFEMODE_SAFE_MODE);
	item->SetType(MENU_ITEM_MARKABLE);
	item->SetHelpText("Puts the system into safe mode. This can be enabled "
		"independently from the other options.");

	safeMenu->AddItem(item = new(nothrow) MenuItem("Disable user add-ons"));
	item->SetData(B_SAFEMODE_DISABLE_USER_ADD_ONS);
	item->SetType(MENU_ITEM_MARKABLE);
    item->SetHelpText("Prevents all user installed add-ons from being loaded. "
		"Only the add-ons in the system directory will be used.");

	safeMenu->AddItem(item = new(nothrow) MenuItem("Disable IDE DMA"));
	item->SetData(B_SAFEMODE_DISABLE_IDE_DMA);
	item->SetType(MENU_ITEM_MARKABLE);
    item->SetHelpText("Disables IDE DMA, increasing IDE compatibility "
		"at the expense of performance.");

#if B_HAIKU_PHYSICAL_BITS > 32
	// check whether we have memory beyond 4 GB
	bool hasMemoryBeyond4GB = false;
	for (uint32 i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
		addr_range& range = gKernelArgs.physical_memory_range[i];
		if (range.start >= (uint64)1 << 32) {
			hasMemoryBeyond4GB = true;
			break;
		}
	}

	bool needs64BitPaging = true;
		// TODO: Determine whether 64 bit paging (i.e. PAE for x86) is needed
		// for other reasons (NX support).

	// ... add the menu item, if so
	if (hasMemoryBeyond4GB || needs64BitPaging) {
		safeMenu->AddItem(
			item = new(nothrow) MenuItem("Ignore memory beyond 4 GiB"));
		item->SetData(B_SAFEMODE_4_GB_MEMORY_LIMIT);
		item->SetType(MENU_ITEM_MARKABLE);
		item->SetHelpText("Ignores all memory beyond the 4 GiB address limit, "
			"overriding the setting in the kernel settings file.");
	}
#endif

	platform_add_menus(safeMenu);

	safeMenu->AddSeparatorItem();
	sBlacklistRootMenu = new(std::nothrow) BlacklistRootMenu;
	safeMenu->AddItem(item = new(std::nothrow) MenuItem("Blacklist entries",
		sBlacklistRootMenu));
	item->SetHelpText("Allows to select system files that shall be ignored. "
		"Useful e.g. to disable drivers temporarily.");

	safeMenu->AddSeparatorItem();
	safeMenu->AddItem(item = new(nothrow) MenuItem("Return to main menu"));

	return safeMenu;
}


static Menu*
add_save_debug_syslog_menu()
{
	Menu* menu = new(nothrow) Menu(STANDARD_MENU, "Save syslog to volume ...");
	MenuItem* item;

	const char* const kHelpText = "Currently only FAT32 volumes are supported. "
		"Newly plugged in removable devices are only recognized after "
		"rebooting.";

	int32 itemsAdded = 0;

	void* cookie;
	if (gRoot->Open(&cookie, O_RDONLY) == B_OK) {
		Node* node;
		while (gRoot->GetNextNode(cookie, &node) == B_OK) {
			Directory* volume = static_cast<Directory*>(node);
			Partition* partition;
			if (gRoot->GetPartitionFor(volume, &partition) != B_OK)
				continue;

			// we support only FAT32 volumes ATM
			if (partition->content_type == NULL
				|| strcmp(partition->content_type, kPartitionTypeFAT32) != 0) {
				continue;
			}

			char name[B_FILE_NAME_LENGTH];
			if (volume->GetName(name, sizeof(name)) != B_OK)
				strlcpy(name, "unnamed", sizeof(name));

			// append offset, size, and type to the name
			size_t len = strlen(name);
			char offsetBuffer[32];
			char sizeBuffer[32];
			snprintf(name + len, sizeof(name) - len,
				" (%s, offset %s, size %s)", partition->content_type,
				size_to_string(partition->offset, offsetBuffer,
					sizeof(offsetBuffer)),
				size_to_string(partition->size, sizeBuffer,
					sizeof(sizeBuffer)));

			item = new(nothrow) MenuItem(name);
			item->SetData(volume);
			item->SetTarget(&debug_menu_save_previous_syslog);
			item->SetType(MENU_ITEM_NO_CHOICE);
			item->SetHelpText(kHelpText);
			menu->AddItem(item);
			itemsAdded++;
		}

		gRoot->Close(cookie);
	}

	if (itemsAdded == 0) {
		menu->AddItem(item
			= new(nothrow) MenuItem("No supported volumes found"));
		item->SetType(MENU_ITEM_NO_CHOICE);
		item->SetHelpText(kHelpText);
		item->SetEnabled(false);
	}

	menu->AddSeparatorItem();
	menu->AddItem(item = new(nothrow) MenuItem("Return to debug menu"));
	item->SetHelpText(kHelpText);

	return menu;
}


static Menu*
add_debug_menu()
{
	Menu* menu = new(std::nothrow) Menu(STANDARD_MENU, "Debug Options");
	MenuItem* item;

#if DEBUG_SPINLOCK_LATENCIES
	item = new(std::nothrow) MenuItem("Disable latency checks");
	if (item != NULL) {
		item->SetType(MENU_ITEM_MARKABLE);
		item->SetData(B_SAFEMODE_DISABLE_LATENCY_CHECK);
		item->SetHelpText("Disables latency check panics.");
		menu->AddItem(item);
	}
#endif

	menu->AddItem(item
		= new(nothrow) MenuItem("Enable serial debug output"));
	item->SetData("serial_debug_output");
	item->SetType(MENU_ITEM_MARKABLE);
	item->SetHelpText("Turns on forwarding the syslog output to the serial "
		"interface (default: 115200, 8N1).");

	menu->AddItem(item
		= new(nothrow) MenuItem("Enable on screen debug output"));
	item->SetData("debug_screen");
	item->SetType(MENU_ITEM_MARKABLE);
	item->SetHelpText("Displays debug output on screen while the system "
		"is booting, instead of the normal boot logo.");

	menu->AddItem(item
		= new(nothrow) MenuItem("Disable on screen paging"));
	item->SetData("disable_onscreen_paging");
	item->SetType(MENU_ITEM_MARKABLE);
	item->SetHelpText("Disables paging when on screen debug output is "
		"enabled.");

	menu->AddItem(item = new(nothrow) MenuItem("Enable debug syslog"));
	item->SetType(MENU_ITEM_MARKABLE);
	item->SetMarked(gKernelArgs.keep_debug_output_buffer);
	item->SetTarget(&debug_menu_toggle_debug_syslog);
    item->SetHelpText("Enables a special in-memory syslog buffer for this "
    	"session that the boot loader will be able to access after rebooting.");

	ring_buffer* syslogBuffer
		= (ring_buffer*)gKernelArgs.debug_output.Pointer();
	bool hasPreviousSyslog
		= syslogBuffer != NULL && ring_buffer_readable(syslogBuffer) > 0;
	if (hasPreviousSyslog) {
		menu->AddItem(item = new(nothrow) MenuItem(
			"Save syslog from previous session during boot"));
		item->SetType(MENU_ITEM_MARKABLE);
		item->SetMarked(gKernelArgs.previous_debug_size);
		item->SetTarget(&debug_menu_toggle_previous_debug_syslog);
		item->SetHelpText("Saves the syslog from the previous Haiku session to "
			"/var/log/previous_syslog when booting.");
	}

	bool currentLogItemVisible = platform_debug_get_log_buffer(NULL) != NULL;
	if (currentLogItemVisible) {
		menu->AddSeparatorItem();
		menu->AddItem(item
			= new(nothrow) MenuItem("Display current boot loader log"));
		item->SetTarget(&debug_menu_display_current_log);
		item->SetType(MENU_ITEM_NO_CHOICE);
		item->SetHelpText(
			"Displays the debug info the boot loader has logged.");
	}

	if (hasPreviousSyslog) {
		if (!currentLogItemVisible)
			menu->AddSeparatorItem();

		menu->AddItem(item
			= new(nothrow) MenuItem("Display syslog from previous session"));
		item->SetTarget(&debug_menu_display_previous_syslog);
		item->SetType(MENU_ITEM_NO_CHOICE);
		item->SetHelpText(
			"Displays the syslog from the previous Haiku session.");

		menu->AddItem(item = new(nothrow) MenuItem(
			"Save syslog from previous session", add_save_debug_syslog_menu()));
		item->SetHelpText("Saves the syslog from the previous Haiku session to "
			"disk. Currently only FAT32 volumes are supported.");
	}

	menu->AddSeparatorItem();
	menu->AddItem(item = new(nothrow) MenuItem(
		"Add advanced debug option"));
	item->SetType(MENU_ITEM_NO_CHOICE);
	item->SetTarget(&debug_menu_add_advanced_option);
	item->SetHelpText(
		"Allows advanced debugging options to be entered directly.");

	menu->AddSeparatorItem();
	menu->AddItem(item = new(nothrow) MenuItem("Return to main menu"));

	return menu;
}


static void
apply_safe_mode_options(Menu* menu)
{
	MenuItemIterator iterator = menu->ItemIterator();
	while (MenuItem* item = iterator.Next()) {
		if (item->Type() == MENU_ITEM_SEPARATOR || !item->IsMarked()
			|| item->Data() == NULL) {
			continue;
		}

		char buffer[256];
		if (snprintf(buffer, sizeof(buffer), "%s true\n",
				(const char*)item->Data()) >= (int)sizeof(buffer)
			|| !sSafeModeOptionsBuffer.Append(buffer)) {
			dprintf("apply_safe_mode_options(): failed to append option to "
				"buffer\n");
		}
	}
}


static void
apply_safe_mode_path_blacklist()
{
	if (sPathBlacklist->IsEmpty())
		return;

	bool success = sSafeModeOptionsBuffer.Append("EntryBlacklist {\n");

	for (PathBlacklist::Iterator it = sPathBlacklist->GetIterator();
		BlacklistedPath* path = it.Next();) {
		success &= sSafeModeOptionsBuffer.Append(path->Path());
		success &= sSafeModeOptionsBuffer.Append("\n", 1);
	}

	success &= sSafeModeOptionsBuffer.Append("}\n");

	if (!success) {
		dprintf("apply_safe_mode_options(): failed to append path "
			"blacklist to buffer\n");
	}
}


static bool
user_menu_reboot(Menu* menu, MenuItem* item)
{
	platform_exit();
	return true;
}


status_t
user_menu(BootVolume& _bootVolume, PathBlacklist& _pathBlacklist)
{

	Menu* menu = new(std::nothrow) Menu(MAIN_MENU);

	sMainMenu = menu;
	sBootVolume = &_bootVolume;
	sPathBlacklist = &_pathBlacklist;

	Menu* safeModeMenu = NULL;
	Menu* debugMenu = NULL;
	MenuItem* item;

	TRACE(("user_menu: enter\n"));

	// Add boot volume
	menu->AddItem(item = new(std::nothrow) MenuItem("Select boot volume",
		add_boot_volume_menu()));

	// Add safe mode
	menu->AddItem(item = new(std::nothrow) MenuItem("Select safe mode options",
		safeModeMenu = add_safe_mode_menu()));

	// add debug menu
	menu->AddItem(item = new(std::nothrow) MenuItem("Select debug options",
		debugMenu = add_debug_menu()));

	// Add platform dependent menus
	platform_add_menus(menu);

	menu->AddSeparatorItem();

	menu->AddItem(item = new(std::nothrow) MenuItem("Reboot"));
	item->SetTarget(user_menu_reboot);
	item->SetShortcut('r');

	menu->AddItem(item = new(std::nothrow) MenuItem("Continue booting"));
	if (!_bootVolume.IsValid()) {
		item->SetLabel("Cannot continue booting (Boot volume is not valid)");
		item->SetEnabled(false);
		menu->ItemAt(0)->Select(true);
	} else
		item->SetShortcut('b');

	menu->Run();

	apply_safe_mode_options(safeModeMenu);
	apply_safe_mode_options(debugMenu);
	apply_safe_mode_path_blacklist();
	add_safe_mode_settings(sSafeModeOptionsBuffer.String());
	delete menu;


	TRACE(("user_menu: leave\n"));

	sMainMenu = NULL;
	sBlacklistRootMenu = NULL;
	sBootVolume = NULL;
	sPathBlacklist = NULL;
	return B_OK;
}
