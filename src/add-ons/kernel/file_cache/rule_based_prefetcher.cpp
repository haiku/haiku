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
#include <util/khash.h>
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
	COMMAND_NAME	= 0x1,
	OPEN_FILE		= 0x2,
	ARGUMENT_NAME	= 0x4,	
	ALL				= 0xff,
};

struct head {
	struct list_link	link;
	mount_id			device;
	vnode_id			parent;
	char				name[B_FILE_NAME_LENGTH];
	int32				confidence;
	bigtime_t			timestamp;
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

		match_type Match(int32 state, mount_id device, vnode_id parent, const char *name);
		void Prefetch();

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
	team_rules	*next;
	team_id		team;
	struct list	states;
};

class RuleMatcher {
	public:
		RuleMatcher(team_id team, Rule **_rule);
		~RuleMatcher();

	private:
		Rule	*fRule;
};

static hash_table *sRulesHash;
static hash_table *sTeamHash;
static recursive_lock sLock;


static int
rules_compare(void *_rules, const void *_key)
{
	struct rules *rules = (struct rules *)_rules;
	const char *key = (const char *)_key;

	return strcmp(rules->name, key);
}


static uint32
rules_hash(void *_rules, const void *_key, uint32 range)
{
	struct rules *rules = (struct rules *)_rules;
	const char *key = (const char *)_key;

	if (rules != NULL)
		return hash_hash_string(rules->name) % range;

	return hash_hash_string(key) % range;
}


static int
team_compare(void *_rules, const void *_key)
{
	team_rules *rules = (team_rules *)_rules;
	const team_id *team = (const team_id *)_key;

	if (rules->team == *team)
		return 0;

	return -1;
}


static uint32
team_hash(void *_rules, const void *_key, uint32 range)
{
	team_rules *rules = (team_rules *)_rules;
	const team_id *team = (const team_id *)_key;

	if (rules != NULL)
		return rules->team % range;

	return *team % range;
}


static void
team_gone(team_id team, void *_rules)
{
	team_rules *rules = (team_rules *)_rules;

	recursive_lock_lock(&sLock);
	hash_remove(sTeamHash, rules);
	recursive_lock_unlock(&sLock);

	delete rules;
}


static inline rules *
find_rules(const char *name)
{
	return (rules *)hash_lookup(sRulesHash, name);
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
	}

	// search for matching rule type
	
	Rule *rule = rules->first;
	while (rule && rule->Type() == type) {
		rule = rule->Next();
	}
	
	if (rule == NULL) {
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
							dprintf("%ld:%Ld:%s %s %ld\n", device, node, fileName, name, confidence);
							
							struct head *head = new ::head;
							head->device = device;
							head->parent = node;
							strlcpy(head->name, fileName, B_FILE_NAME_LENGTH);
							head->confidence = confidence;

							Rule *rule = get_rule(name, COMMAND_NAME);
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
	fConfidence(0)
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


//	#pragma mark -


static void
node_opened(void *vnode, int32 fdType, mount_id device, vnode_id parent,
	vnode_id node, const char *name, off_t size)
{
	if (device < gBootDevice) {
		// we ignore any access to rootfs, pipefs, and devfs
		// ToDo: if we can ever move the boot device on the fly, this will break
		return;
	}
}


static void
node_launched(size_t argCount, char * const *args)
{
}


static void
uninit()
{
	recursive_lock_lock(&sLock);

	// free all sessions from the hashes

	uint32 cookie = 0;
	team_rules *rules;
	while ((rules = (team_rules *)hash_remove_first(sTeamHash, &cookie)) != NULL) {
		delete rules;
	}
	Rule *rule;
	cookie = 0;
	while ((rule = (Rule *)hash_remove_first(sRulesHash, &cookie)) != NULL) {
		delete rule;
	}

	hash_uninit(sTeamHash);
	hash_uninit(sRulesHash);
	recursive_lock_destroy(&sLock);
}


static status_t
init()
{
	sTeamHash = hash_init(64, 0, &team_compare, &team_hash);
	if (sTeamHash == NULL)
		return B_NO_MEMORY;

	status_t status;

	sRulesHash = hash_init(64, 0, &rules_compare, &rules_hash);
	if (sRulesHash == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}

	if (recursive_lock_init(&sLock, "rule based prefetcher") < B_OK) {
		status = sLock.sem;
		goto err2;
	}

	load_rules();
	return B_OK;

err2:
	hash_uninit(sRulesHash);
err1:
	hash_uninit(sTeamHash);
	return status;
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
