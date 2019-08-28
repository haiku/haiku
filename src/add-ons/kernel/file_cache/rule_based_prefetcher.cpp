/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/** This module applies rules that were learned by another component by
 *	evaluating the cache log files.
 *
 *	Note: this module is using private kernel API and is definitely not
 *		meant to be an example on how to write modules.
 */


#include <KernelExport.h>
#include <Node.h>

#include <util/kernel_cpp.h>
#include <util/AutoLock.h>
#include <thread.h>
#include <team.h>
#include <file_cache.h>
#include <generic_syscall.h>
#include <syscalls.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>


#define TRACE_CACHE_MODULE
#ifdef TRACE_CACHE_MODULE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


extern dev_t gBootDevice;


enum match_type {
	NO_MATCH,
	CONTINUE_MATCHING,
	MATCH,
};

enum rule_type {
	LAUNCH_TYPE		= 0x1,
	OPEN_FILE_TYPE	= 0x2,
	ARGUMENTS_TYPE	= 0x4,
	ALL_TYPES		= 0xff,
};

struct head {
	head();

	struct list_link	link;
	mount_id			device;
	vnode_id			parent;
	vnode_id			node;
	char				name[B_FILE_NAME_LENGTH];
	int32				confidence;
	bigtime_t			timestamp;
	int32				used_count;
};

struct body {
	struct list_link	link;
};

class Rule {
	public:
		Rule(rule_type type);
		~Rule();

		status_t InitCheck();
		rule_type Type() const { return fType; }

		void AddHead(struct head *head);
		void AddBody(struct body *body);

		struct head *FindHead(mount_id device, vnode_id node);
		match_type Match(int32 state, mount_id device, vnode_id parent,
			const char *name);
		void Apply();

		void KnownFileOpened() { fKnownFileOpened++; }
		void UnknownFileOpened() { fUnknownFileOpened++; }

		void Dump();

		Rule *&Next() { return fNext; }

		static uint32 NextOffset() { return offsetof(Rule, fNext); }
		static status_t LoadRules();

	private:
		Rule		*fNext;
		rule_type	fType;
		struct list	fHeads;
		struct list	fBodies;
		int32		fHeadCount;
		int32		fBodyCount;
		int32		fConfidence;
		int32		fAppliedCount;
		int32		fKnownFileOpened;
		int32		fUnknownFileOpened;
};

struct rule_state {
	list_link	link;
	Rule		*rule;
	int32		state;
};

struct rules {
	rules		*next;
	char		name[B_FILE_NAME_LENGTH];
	Rule		*first;
};

struct team_rules {
	~team_rules();

	team_rules	*next;
	team_id		team;
	struct list	states;
	struct list	applied;
	bigtime_t	timestamp;
};

class RuleMatcher {
	public:
		RuleMatcher(team_id team, const char *name = NULL);
		~RuleMatcher();

		void GotFile(mount_id device, vnode_id node);
		void GotArguments(int32 argCount, char * const *args);

	private:
		void _CollectRules(const char *name);
		void _MatchFile(mount_id device, vnode_id parent, const char *name);
		void _MatchArguments(int32 argCount, char * const *args);

		team_rules	*fRules;
};


struct RuleHash {
		typedef char*		KeyType;
		typedef	rules		ValueType;

		size_t HashKey(KeyType key) const
		{
			return hash_hash_string(key);
		}

		size_t Hash(ValueType* value) const
		{
			return HashKey(value->name);
		}

		bool Compare(KeyType key, ValueType* rules) const
		{
			return strcmp(rules->name, key) == 0;
		}

		ValueType*& GetLink(ValueType* value) const
		{
			return value->fNext;
		}
};


struct TeamHash {
		typedef team_id		KeyType;
		typedef	team_rules	ValueType;

		size_t HashKey(KeyType key) const
		{
			return key;
		}

		size_t Hash(ValueType* value) const
		{
			return value->team;
		}

		bool Compare(KeyType key, ValueType* value) const
		{
			return value->team == key;
		}

		ValueType*& GetLink(ValueType* value) const
		{
			return value->fNext;
		}
};


typedef BOpenHashTable<RuleHash> RuleTable;
typedef BOpenHashTable<TeamHash> TeamTable;

static RuleTable *sRulesHash;
static TeamTable *sTeamHash;
static recursive_lock sLock;
int32 sMinConfidence = 5000;


static void
team_gone(team_id team, void *_rules)
{
	team_rules *rules = (team_rules *)_rules;

	recursive_lock_lock(&sLock);
	sTeamHash->Remove(rules);
	recursive_lock_unlock(&sLock);

	delete rules;
}


team_rules::~team_rules()
{
	// free rule states

	rule_state *state;
	while ((state = (rule_state *)list_remove_head_item(&states)) != NULL) {
		delete state;
	}
	while ((state = (rule_state *)list_remove_head_item(&applied)) != NULL) {
		delete state;
	}

	stop_watching_team(team, team_gone, this);
}


//	#pragma mark -


head::head()
{
	device = -1;
	parent = -1;
	node = -1;
	name[0] = '\0';
	confidence = -1;
	timestamp = 0;
	used_count = 0;
}


static inline rules *
find_rules(const char *name)
{
	return sRulesHash->Lookup(name);
}


/**	Finds the rule matching to the criteria (name and type).
 *	If there is no matching rule yet, it will create one.
 *	It will only return NULL in out of memory conditions.
 */

static Rule *
get_rule(const char *name, rule_type type)
{
	struct rules *rules = find_rules(name);
	if (rules == NULL) {
		// add new rules
		rules = new ::rules;
		if (rules == NULL)
			return NULL;

		strlcpy(rules->name, name, B_FILE_NAME_LENGTH);
		rules->first = NULL;
		sRulesHash->Insert(rules);
	}

	// search for matching rule type

	Rule *rule = rules->first;
	while (rule && rule->Type() != type) {
		rule = rule->Next();
	}

	if (rule == NULL) {
		TRACE(("create new rule for \"%s\", type %d\n", name, type));
		// there is no rule yet, create one
		rule = new Rule(type);
		if (rule == NULL)
			return NULL;

		rule->Next() = rules->first;
		rules->first = rule;
	}

	return rule;
}


static void
eat_spaces(char *&line)
{
	// eat starting white space
	while (isspace(line[0]))
		line++;
}


static bool
parse_ref(const char *string, mount_id &device, vnode_id &node, char **_end = NULL)
{
	// parse node ref
	char *end;
	device = strtol(string, &end, 0);
	if (end == NULL || device == 0 || end[0] != ':')
		return false;

	node = strtoull(end + 1, &end, 0);
	if (end == NULL || end[0] != ':')
		return false;

	if (_end)
		*_end = end + 1;
	return true;
}


static void
ignore_line(char *&line)
{
	while (line[0] && line[0] != '\n')
		line++;
}


static const char *
get_name(char *&line)
{
	if (line[0] != '"')
		return NULL;

	const char *name = ++line;

	while (line[0] && line[0] != '"') {
		if (line[0] == '\\')
			line++;
		line++;
	}

	line[0] = '\0';
	line++;

	return name;
}


static status_t
load_rules()
{
	int fd = open("/etc/cache_rules", O_RDONLY);
	if (fd < B_OK)
		return fd;

	struct stat stat;
	if (fstat(fd, &stat) != 0) {
		close(fd);
		return errno;
	}

	if (stat.st_size > 32767) {
		// for safety reasons
		// ToDo: make a bit larger later
		close(fd);
		return B_BAD_DATA;
	}

	char *buffer = (char *)malloc(stat.st_size + 1);
	if (buffer == NULL) {
		close(fd);
		return B_NO_MEMORY;
	}

	if (read(fd, buffer, stat.st_size) < stat.st_size) {
		free(buffer);
		close(fd);
		return B_ERROR;
	}

	buffer[stat.st_size] = '\0';

	char *line = buffer;
	while (line[0]) {
		switch (line[0]) {
			case 'c':
			{
				mount_id device;
				vnode_id node;

				// direct "command opens file" rule
				line += 2;
				if (parse_ref(line, device, node, &line)) {
					const char *fileName = get_name(line);
					if (fileName != NULL) {
						eat_spaces(line);
						const char *name = get_name(line);
						if (name != NULL) {
							eat_spaces(line);
							int32 confidence = strtoul(line, &line, 10);
							TRACE(("c %ld:%Ld:%s %s %ld\n", device, node, fileName, name, confidence));

							struct head *head = new ::head;
							head->device = device;
							head->parent = node;
							strlcpy(head->name, fileName, B_FILE_NAME_LENGTH);
							head->confidence = confidence;

							Rule *rule = get_rule(name, LAUNCH_TYPE);
							if (rule != NULL)
								rule->AddHead(head);
							break;
						}
					}
				}
				ignore_line(line);
				break;
			}
			default:
				// unknown rule - ignore line
				ignore_line(line);
				break;
		}
		line++;
	}

	free(buffer);
	close(fd);
	return B_OK;
}


//	#pragma mark -


Rule::Rule(rule_type type)
	:
	fType(type),
	fHeadCount(0),
	fBodyCount(0),
	fAppliedCount(0),
	fKnownFileOpened(0),
	fUnknownFileOpened(0)
{
	list_init(&fHeads);
	list_init(&fBodies);
}


Rule::~Rule()
{
	struct head *head;
	while ((head = (struct head *)list_remove_head_item(&fHeads)) != NULL) {
		delete head;
	}

	struct body *body;
	while ((body = (struct body *)list_remove_head_item(&fBodies)) != NULL) {
		delete body;
	}
}


void
Rule::AddHead(struct head *head)
{
	list_add_item(&fHeads, head);
	fHeadCount++;
}


void
Rule::AddBody(struct body *body)
{
	list_add_item(&fBodies, body);
	fBodyCount++;
}


void
Rule::Apply()
{
	TRACE(("Apply rule %p", this));

	struct head *head = NULL;
	while ((head = (struct head *)list_get_next_item(&fHeads, head)) != NULL) {
		if (head->confidence < sMinConfidence)
			continue;

		// prefetch node
		void *vnode;
		if (vfs_entry_ref_to_vnode(head->device, head->parent, head->name, &vnode) == B_OK) {
			vfs_vnode_to_node_ref(vnode, &head->device, &head->node);

			TRACE(("prefetch: %ld:%Ld:%s\n", head->device, head->parent, head->name));
			cache_prefetch(head->device, head->node, 0, ~0UL);

			// ToDo: put head into a hash so that some statistics can be backpropagated quickly
		} else {
			// node doesn't exist anymore
			head->confidence = -1;
		}
	}

	fAppliedCount++;
}


struct head *
Rule::FindHead(mount_id device, vnode_id node)
{
	// ToDo: use a hash for this!

	struct head *head = NULL;
	while ((head = (struct head *)list_get_next_item(&fHeads, head)) != NULL) {
		if (head->node == node && head->device == device)
			return head;
	}

	return NULL;
}


void
Rule::Dump()
{
	dprintf("  applied: %ld, known: %ld, unknown: %ld\n",
		fAppliedCount, fKnownFileOpened, fUnknownFileOpened);

	struct head *head = NULL;
	while ((head = (struct head *)list_get_next_item(&fHeads, head)) != NULL) {
		dprintf("  %ld:%Ld:\"%s\", ", head->device, head->parent, head->name);
		if (head->confidence < sMinConfidence)
			dprintf("-\n");
		else
			dprintf("%ld (%ld), %Ld us\n", head->used_count,
				head->used_count - fAppliedCount, head->timestamp);
	}
}


//	#pragma mark -


RuleMatcher::RuleMatcher(team_id team, const char *name)
	:
	fRules(NULL)
{
	recursive_lock_lock(&sLock);

	if (name == NULL)
		return;

	fRules = sTeamHash->Lookup(team);
	if (fRules != NULL)
		return;

	fRules = new team_rules;
	if (fRules == NULL)
		return;

	fRules->team = team;
	list_init(&fRules->states);
	list_init(&fRules->applied);

dprintf("new rules for \"%s\"\n", name);
	_CollectRules(name);

	sTeamHash->Insert(fRules);
	start_watching_team(team, team_gone, fRules);

	fRules->timestamp = system_time();
}


RuleMatcher::~RuleMatcher()
{
	recursive_lock_unlock(&sLock);
}


void
RuleMatcher::_CollectRules(const char *name)
{
	struct rules *rules = sRulesHash->Lookup(name);
	if (rules == NULL) {
		// there are no rules for this command
		return;
	}

	// allocate states for all rules found

	for (Rule *rule = rules->first; rule != NULL; rule = rule->Next()) {
		rule_state *state = new rule_state;
		if (state == NULL)
			continue;

		TRACE(("found rule %p for \"%s\"\n", rule, rules->name));
		state->rule = rule;
		state->state = 0;

		if (rule->Type() == LAUNCH_TYPE) {
			// we can already prefetch the simplest of all rules here
			// (it's fulfilled as soon as the command is launched)
			rule->Apply();
			list_add_item(&fRules->applied, state);
		} else
			list_add_item(&fRules->states, state);
	}
}


void
RuleMatcher::GotFile(mount_id device, vnode_id node)
{
	if (fRules == NULL)
		return;

	// try to match the file with all open rules

	rule_state *state = NULL;
	while ((state = (rule_state *)list_get_next_item(&fRules->states, state)) != NULL) {
		if ((state->rule->Type() & OPEN_FILE_TYPE) == 0)
			continue;

		// ToDo
	}

	bigtime_t diff = system_time() - fRules->timestamp;

	// propagate the usage of this file back to the applied rules

	while ((state = (rule_state *)list_get_next_item(&fRules->applied, state)) != NULL) {
		struct head *head = state->rule->FindHead(device, node);
		if (head != NULL) {
			// grow confidence
			state->rule->KnownFileOpened();
			head->used_count++;
			if (head->used_count > 1)
				head->timestamp = (head->timestamp * (head->used_count - 1) + diff) / head->used_count;
		} else
			state->rule->UnknownFileOpened();
	}
}


void
RuleMatcher::GotArguments(int32 argCount, char * const *args)
{
	if (fRules == NULL)
		return;

	// try to match the arguments with all open rules

	rule_state *state = NULL;
	while ((state = (rule_state *)list_get_next_item(&fRules->states, state)) != NULL) {
		if ((state->rule->Type() & ARGUMENTS_TYPE) == 0)
			continue;

		// ToDo
	}
}


//	#pragma mark -


static void
node_opened(struct vnode *vnode, int32 fdType, dev_t device, vnode_id parent,
	vnode_id node, const char *name, off_t size)
{
	if (device < gBootDevice) {
		// we ignore any access to rootfs, pipefs, and devfs
		// ToDo: if we can ever move the boot device on the fly, this will break
		return;
	}

	// ToDo: this is only needed if there is no rule for this team yet - ideally,
	//	it would be handled by the log module, so that the vnode ID is sufficient
	//	to recognize a rule
	char buffer[B_FILE_NAME_LENGTH];
	if (name == NULL
		&& vfs_get_vnode_name(vnode, buffer, sizeof(buffer)) == B_OK)
		name = buffer;

	//dprintf("opened: %ld:%Ld:%Ld:%s (%s)\n", device, parent, node, name, thread_get_current_thread()->name);
	RuleMatcher matcher(team_get_current_team_id(), name);
	matcher.GotFile(device, node);
}


static void
node_launched(size_t argCount, char * const *args)
{
	//dprintf("launched: %s (%s)\n", args[0], thread_get_current_thread()->name);
	RuleMatcher matcher(team_get_current_team_id());
	matcher.GotArguments(argCount, args);
}


static void
uninit()
{
	recursive_lock_lock(&sLock);

	// free all sessions from the hashes

	team_rules *teamRules = sTeamHash->Clear(true);
	while (teamRules != NULL) {
		team_rules *next = teamRules->next;
		delete teamRules;
		teamRules = next;
	}

	struct rules *rules = sRulesHash->Clear(true);
	while (rules != NULL) {
		Rule *rule = rules->first;
		while (rule) {
			Rule *next = rule->Next();

			dprintf("Rule %p \"%s\"\n", rule, rules->name);
			rule->Dump();

			delete rule;
			rule = next;
		}

		struct rules *next = rules->next;
		delete rules;
		rules = next;
	}

	delete sTeamHash;
	delete sRulesHash;
	recursive_lock_destroy(&sLock);
}


static status_t
init()
{
	sTeamHash = new(std::nothrow) TeamTable();
	if (sTeamHash == NULL || sTeamHash->Init(64) != B_OK)
		return B_NO_MEMORY;

	status_t status;

	sRulesHash = new(std::nothrow) RuleTable();
	if (sRulesHash == NULL || sRulesHash->Init(64) != B_OK) {
		delete sTeamHash;
		return B_NO_MEMORY;
	}

	recursive_lock_init(&sLock, "rule based prefetcher");

	load_rules();
	return B_OK;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init();

		case B_MODULE_UNINIT:
			uninit();
			return B_OK;

		default:
			return B_ERROR;
	}
}


static struct cache_module_info sRuleBasedPrefetcherModule = {
	{
		CACHE_MODULES_NAME "/rule_based_prefetcher/v1",
		0,
		std_ops,
	},
	node_opened,
	NULL,
	node_launched,
};


module_info *modules[] = {
	(module_info *)&sRuleBasedPrefetcherModule,
	NULL
};
