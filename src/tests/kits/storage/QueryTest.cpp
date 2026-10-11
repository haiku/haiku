/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Messenger.h>
#include <NodeMonitor.h>
#include <OS.h>
#include <ObjectList.h>
#include <Path.h>
#include <Query.h>
#include <StorageDefs.h>
#include <String.h>
#include <SupportDefs.h>
#include <Volume.h>
 
#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "BasicTest.h"
#include "TestApp.h"


class Query : public BQuery {
public:
#if TEST_R5
	status_t PushValue(int32 value)
	{
		PushInt32(value);
		return B_OK;
	}

	status_t PushValue(uint32 value)
	{
		PushUInt32(value);
		return B_OK;
	}

	status_t PushValue(int64 value)
	{
		PushInt64(value);
		return B_OK;
	}

	status_t PushValue(uint64 value)
	{
		PushUInt64(value);
		return B_OK;
	}

	status_t PushValue(float value)
	{
		PushFloat(value);
		return B_OK;
	}

	status_t PushValue(double value)
	{
		PushDouble(value);
		return B_OK;
	}

	status_t PushValue(const BString value, bool caseInsensitive = false)
	{
		PushString(value.String(), caseInsensitive);
		return B_OK;
	}

	status_t PushAttr(const char* attribute)
	{
		BQuery::PushAttr(attribute);
		return B_OK;
	}

	status_t PushOp(query_op op)
	{
		BQuery::PushOp(op);
		return B_OK;
	}
#else
	status_t PushValue(int32 value) { return PushInt32(value); }

	status_t PushValue(uint32 value) { return PushUInt32(value); }

	status_t PushValue(int64 value) { return PushInt64(value); }

	status_t PushValue(uint64 value) { return PushUInt64(value); }

	status_t PushValue(float value) { return PushFloat(value); }

	status_t PushValue(double value) { return PushDouble(value); }

	status_t PushValue(const BString value, bool caseInsensitive = false)
	{
		return PushString(value.String(), caseInsensitive);
	}
#endif
};

// PredicateNode

class PredicateNode {
public:
	virtual ~PredicateNode() {}

	virtual status_t push(Query& query) const = 0;
	virtual BString toString() const = 0;
};

// ValueNode

template<typename ValueType> class ValueNode : public PredicateNode {
public:
	ValueNode(ValueType v)
		: value(v)
	{
	}

	virtual ~ValueNode() {}

	virtual status_t push(Query& query) const { return query.PushValue(value); }

	virtual BString toString() const { return BString() << value; }

	ValueType value;
};

// float specialization
template<>
BString
ValueNode<float>::toString() const
{
	char buffer[32];
	sprintf(buffer, "0x%08lx", *(int32*)&value);
	return BString() << buffer;
}

// double specialization
template<>
BString
ValueNode<double>::toString() const
{
	char buffer[32];
	sprintf(buffer, "0x%016Lx", *(int64*)&value);
	return BString() << buffer;
}

// StringNode

class StringNode : public PredicateNode {
public:
	StringNode(BString v, bool caseInsensitive = false)
		: value(v),
		  caseInsensitive(caseInsensitive)
	{
	}

	virtual ~StringNode() {}

	virtual status_t push(Query& query) const { return query.PushValue(value, caseInsensitive); }

	virtual BString toString() const
	{
		BString escaped;
		if (caseInsensitive) {
			const char* str = value.String();
			int32 len = value.Length();
			for (int32 i = 0; i < len; i++) {
				char c = str[i];
				if (isalpha(c)) {
					int lower = tolower(c);
					int upper = toupper(c);
					if (lower < 0 || upper < 0)
						escaped << c;
					else
						escaped << "[" << (char)lower << (char)upper << "]";
				} else {
					escaped << c;
				}
			}
		} else {
			escaped = value;
		}
		escaped.CharacterEscape("\"\\'", '\\');
		return BString("\"") << escaped << "\"";
	}

	BString value;
	bool caseInsensitive;
};

// DateNode

class DateNode : public PredicateNode {
public:
	DateNode(BString v)
		: value(v)
	{
	}

	virtual ~DateNode() {}

	virtual status_t push(Query& query) const { return query.PushDate(value.String()); }

	virtual BString toString() const
	{
		BString escaped(value);
		escaped.CharacterEscape("%\"\\'", '\\');
		return BString("%") << escaped << "%";
	}

	BString value;
};

// AttributeNode

class AttributeNode : public PredicateNode {
public:
	AttributeNode(BString v)
		: value(v)
	{
	}

	virtual ~AttributeNode() {}

	virtual status_t push(Query& query) const { return query.PushAttr(value.String()); }

	virtual BString toString() const { return value; }

	BString value;
};

// short hands
typedef ValueNode<int32> Int32Node;
typedef ValueNode<uint32> UInt32Node;
typedef ValueNode<int64> Int64Node;
typedef ValueNode<uint64> UInt64Node;
typedef ValueNode<float> FloatNode;
typedef ValueNode<double> DoubleNode;

// ListNode

class ListNode : public PredicateNode {
public:
	ListNode(PredicateNode* child1 = NULL, PredicateNode* child2 = NULL,
		PredicateNode* child3 = NULL, PredicateNode* child4 = NULL, PredicateNode* child5 = NULL,
		PredicateNode* child6 = NULL)
		: children()
	{
		addChild(child1);
		addChild(child2);
		addChild(child3);
		addChild(child4);
		addChild(child5);
		addChild(child6);
	}

	virtual ~ListNode()
	{
		for (int32 i = 0; PredicateNode* child = childAt(i); i++)
			delete child;
	}

	virtual status_t push(Query& query) const
	{
		status_t error = B_OK;
		for (int32 i = 0; PredicateNode* child = childAt(i); i++) {
			error = child->push(query);
			if (error != B_OK)
				break;
		}
		return error;
	}

	virtual BString toString() const { return BString("INVALID"); }

	ListNode& addChild(PredicateNode* child)
	{
		if (child)
			children.AddItem(child);
		return *this;
	}

	PredicateNode* childAt(int32 index) const { return (PredicateNode*)children.ItemAt(index); }

	BObjectList<PredicateNode> children;
};

// OpNode

class OpNode : public ListNode {
public:
	OpNode(query_op op, PredicateNode* left, PredicateNode* right = NULL)
		: ListNode(left, right),
		  op(op)
	{
	}

	virtual ~OpNode() {}

	virtual status_t push(Query& query) const
	{
		status_t error = ListNode::push(query);
		if (error == B_OK)
			error = query.PushOp(op);
		return error;
	}

	virtual BString toString() const
	{
		PredicateNode* left = childAt(0);
		PredicateNode* right = childAt(1);
		if (!left)
			return "INVALID ARGS";
		BString result;
		BString leftString = left->toString();
		BString rightString;
		if (right)
			rightString = right->toString();
		switch (op) {
			case B_INVALID_OP:
				result = "INVALID";
				break;
			case B_EQ:
				result << "(" << leftString << "==" << rightString << ")";
				break;
			case B_GT:
				result << "(" << leftString << ">" << rightString << ")";
				break;
			case B_GE:
				result << "(" << leftString << ">=" << rightString << ")";
				break;
			case B_LT:
				result << "(" << leftString << "<" << rightString << ")";
				break;
			case B_LE:
				result << "(" << leftString << "<=" << rightString << ")";
				break;
			case B_NE:
				result << "(" << leftString << "!=" << rightString << ")";
				break;
			case B_CONTAINS:
			{
				StringNode* strNode = dynamic_cast<StringNode*>(right);
				if (strNode)
					rightString = StringNode(BString("*") << strNode->value << "*").toString();
				result << "(" << leftString << "==" << rightString << ")";
				break;
			}
			case B_BEGINS_WITH:
			{
				StringNode* strNode = dynamic_cast<StringNode*>(right);
				if (strNode)
					rightString = StringNode(BString(strNode->value) << "*").toString();
				result << "(" << leftString << "==" << rightString << ")";
				break;
			}
			case B_ENDS_WITH:
			{
				StringNode* strNode = dynamic_cast<StringNode*>(right);
				if (strNode)
					rightString = StringNode(BString("*") << strNode->value).toString();
				result << "(" << leftString << "==" << rightString << ")";
				break;
			}
			case B_AND:
				result << "(" << leftString << "&&" << rightString << ")";
				break;
			case B_OR:
				result << "(" << leftString << "||" << rightString << ")";
				break;
			case B_NOT:
				result << "(" << "!" << leftString << ")";
				break;
			case _B_RESERVED_OP_:
				result = "RESERVED";
				break;
		}
		return result;
	}

	query_op op;
};

// QueryTestEntry
class QueryTestEntry {
public:
	QueryTestEntry(std::string path, node_flavor kind, const QueryTestEntry* linkTarget = NULL)
		: path(path),
		  cpath(NULL),
		  kind(kind),
		  linkToPath(),
		  clinkToPath(NULL),
		  directory(-1),
		  node(-1),
		  name()
	{
		cpath = this->path.c_str();
		if (linkTarget)
			linkToPath = linkTarget->path;
		clinkToPath = this->linkToPath.c_str();
	}

	std::string operator+(std::string leaf) const { return path + "/" + leaf; }

	std::string path;
	const char* cpath;
	node_flavor kind;
	std::string linkToPath;
	const char* clinkToPath;
	ino_t directory;
	ino_t node;
	std::string name;
};

static const char* testVolumeImage = "/tmp/query-test-image";
static const char* testMountPoint = "/non-existing-mount-point";

// the test entry hierarchy:
// mountPoint
// + dir1
//   + subdir11
//   + subdir12
//   + file11
//   + file12
//   + link11
// + dir2
//   + subdir21
//   + subdir22
//   + subdir23
//   + file21
//   + file22
//   + link21
// + dir3
//   + subdir31
//   + subdir32
//   + file31
//   + file32
//   + link31
// + file1
// + file2
// + file3
// + link1
// + link2
// + link3
static QueryTestEntry mountPoint(testMountPoint, B_DIRECTORY_NODE);
static QueryTestEntry dir1(mountPoint + "dir1", B_DIRECTORY_NODE);
static QueryTestEntry subdir11(dir1 + "subdir11", B_DIRECTORY_NODE);
static QueryTestEntry subdir12(dir1 + "subdir12", B_DIRECTORY_NODE);
static QueryTestEntry file11(dir1 + "file11", B_FILE_NODE);
static QueryTestEntry file12(dir1 + "file12", B_FILE_NODE);
static QueryTestEntry link11(dir1 + "link11", B_SYMLINK_NODE, &file11);
static QueryTestEntry dir2(mountPoint + "dir2", B_DIRECTORY_NODE);
static QueryTestEntry subdir21(dir2 + "subdir21", B_DIRECTORY_NODE);
static QueryTestEntry subdir22(dir2 + "subdir22", B_DIRECTORY_NODE);
static QueryTestEntry subdir23(dir2 + "subdir23", B_DIRECTORY_NODE);
static QueryTestEntry file21(dir2 + "file21", B_FILE_NODE);
static QueryTestEntry file22(dir2 + "file22", B_FILE_NODE);
static QueryTestEntry link21(dir2 + "link21", B_SYMLINK_NODE, &file12);
static QueryTestEntry dir3(mountPoint + "dir3", B_DIRECTORY_NODE);
static QueryTestEntry subdir31(dir3 + "subdir31", B_DIRECTORY_NODE);
static QueryTestEntry subdir32(dir3 + "subdir32", B_DIRECTORY_NODE);
static QueryTestEntry file31(dir3 + "file31", B_FILE_NODE);
static QueryTestEntry file32(dir3 + "file32", B_FILE_NODE);
static QueryTestEntry link31(dir3 + "link31", B_SYMLINK_NODE, &file22);
static QueryTestEntry file1(mountPoint + "file1", B_FILE_NODE);
static QueryTestEntry file2(mountPoint + "file2", B_FILE_NODE);
static QueryTestEntry file3(mountPoint + "file3", B_FILE_NODE);
static QueryTestEntry link1(mountPoint + "link1", B_SYMLINK_NODE, &file1);
static QueryTestEntry link2(mountPoint + "link2", B_SYMLINK_NODE, &file2);
static QueryTestEntry link3(mountPoint + "link3", B_SYMLINK_NODE, &file3);

static QueryTestEntry* allTestEntries[] = {&dir1, &subdir11, &subdir12, &file11, &file12, &link11,
	&dir2, &subdir21, &subdir22, &subdir23, &file21, &file22, &link21, &dir3, &subdir31, &subdir32,
	&file31, &file32, &link31, &file1, &file2, &file3, &link1, &link2, &link3};
static const int32 allTestEntryCount = sizeof(allTestEntries) / sizeof(QueryTestEntry*);

// create_test_entries
void
create_test_entries(QueryTestEntry** testEntries, int32 count)
{
	// create the command line
	std::string cmdLine("true");
	for (int32 i = 0; i < count; i++) {
		const QueryTestEntry* entry = testEntries[i];
		switch (entry->kind) {
			case B_DIRECTORY_NODE:
				cmdLine += " ; mkdir " + entry->path;
				break;
			case B_FILE_NODE:
				cmdLine += " ; touch " + entry->path;
				break;
			case B_SYMLINK_NODE:
				cmdLine += " ; ln -s " + entry->linkToPath + " " + entry->path;
				break;
			case B_ANY_NODE:
			default:
				printf("WARNING: invalid node kind\n");
				break;
		}
	}
	BasicTest::execCommand(cmdLine);
}

// delete_test_entries
void
delete_test_entries(QueryTestEntry** testEntries, int32 count)
{
	// create the command line
	std::string cmdLine("true");
	for (int32 i = 0; i < count; i++) {
		const QueryTestEntry* entry = testEntries[i];
		switch (entry->kind) {
			case B_DIRECTORY_NODE:
			case B_FILE_NODE:
			case B_SYMLINK_NODE:
				cmdLine += " ; rm -rf " + entry->path;
				break;
			case B_ANY_NODE:
			default:
				printf("WARNING: invalid node kind\n");
				break;
		}
	}
	BasicTest::execCommand(cmdLine);
}

static void
TestPredicate(const PredicateNode& predicateNode, status_t pushResult = B_OK,
	status_t getResult = B_OK)
{
	BString predicateString = predicateNode.toString().String();
	// printf("predicate: `%s'\n", predicateString.String());
	//  GetPredicate(BString *)
	{
		Query query;
		//		CPPUNIT_ASSERT(predicateNode.push(query) == pushResult);
		status_t error = predicateNode.push(query);
		if (error != pushResult) {
			printf("predicate: `%s'\n", predicateString.String());
			printf("error: %lx vs %lx\n", error, pushResult);
		}
		CPPUNIT_ASSERT(error == pushResult);
		if (pushResult == B_OK) {
			BString predicate;
			//			CPPUNIT_ASSERT(query.GetPredicate(&predicate) == getResult);
			error = query.GetPredicate(&predicate);
			if (error != getResult) {
				printf("predicate: `%s'\n", predicateString.String());
				printf("error: %lx vs %lx\n", error, getResult);
			}
			CPPUNIT_ASSERT(error == getResult);
			if (getResult == B_OK) {
				CPPUNIT_ASSERT((int32)query.PredicateLength() == predicateString.Length() + 1);
				CPPUNIT_ASSERT(predicateString == predicate);
			}
		}
	}
	// GetPredicate(char *, size_t)
	{
		Query query;
		CPPUNIT_ASSERT(predicateNode.push(query) == pushResult);
		if (pushResult == B_OK) {
			char buffer[1024];
			CPPUNIT_ASSERT(query.GetPredicate(buffer, sizeof(buffer)) == getResult);
			if (getResult == B_OK)
				CPPUNIT_ASSERT(predicateString == buffer);
		}
	}
	// PredicateLength()
	{
		Query query;
		CPPUNIT_ASSERT(predicateNode.push(query) == pushResult);
		if (pushResult == B_OK) {
			size_t expectedLength = (getResult == B_OK ? predicateString.Length() + 1 : 0);
			CPPUNIT_ASSERT(query.PredicateLength() == expectedLength);
		}
	}
	// SetPredicate()
	{
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate(predicateString.String()));
		CPPUNIT_ASSERT((int32)query.PredicateLength() == predicateString.Length() + 1);
		BString predicate;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.GetPredicate(&predicate));
		CPPUNIT_ASSERT(predicateString == predicate);
	}
}

static void
TestOperator(query_op op)
{
	// well formed
	TestPredicate(OpNode(op, new AttributeNode("attribute"), new Int32Node(42)));
	TestPredicate(OpNode(op, new AttributeNode("attribute"), new StringNode("some string")));
	TestPredicate(OpNode(op, new AttributeNode("attribute"), new DateNode("22 May 2002")));
	// ill formed
	TestPredicate(OpNode(op, new AttributeNode("attribute"), NULL), B_OK, B_NO_INIT);
// R5: crashs when pushing B_CONTAINS/B_BEGINS/ENDS_WITH on an empty stack
#if TEST_R5
	if (op < B_CONTAINS || op > B_ENDS_WITH)
#endif
		TestPredicate(OpNode(op, NULL, NULL), B_OK, B_NO_INIT);
	TestPredicate(OpNode(op, new AttributeNode("attribute"), new DateNode("22 May 2002"))
					  .addChild(new Int32Node(42)),
		B_OK, B_NO_INIT);
}

	static void
	TestFetchPredicateInit(Query& query, TestSet& testSet, const char* mountPoint,
		const char* predicate, QueryTestEntry** entries, int32 entryCount)
{
	// init the query
	CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate(predicate));
	BVolume volume(dev_for_path(mountPoint));
	CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
	CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
	CPPUNIT_ASSERT_EQUAL(B_OK, query.Fetch());
	// init the test set
	testSet.clear();
	for (int32 i = 0; i < entryCount; i++)
		testSet.add(entries[i]->path);
}

static void
TestFetchPredicate(const char* mountPoint, const char* predicate, QueryTestEntry** entries,
	int32 entryCount)
{
	// GetNextEntry()
	{
		Query query;
		TestSet testSet;
		TestFetchPredicateInit(query, testSet, mountPoint, predicate, entries, entryCount);
		BEntry entry;
		while (query.GetNextEntry(&entry) == B_OK) {
// Haiku supports rewinding queries, R5 does not.
#ifdef TEST_R5
			CPPUNIT_ASSERT_EQUAL(B_ERROR, query.Rewind());
#endif
			CPPUNIT_ASSERT_EQUAL(B_ERROR, query.CountEntries());
			BPath path;
			CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
			CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
			CPPUNIT_ASSERT(testSet.test(path.Path()) == true);
		}
		CPPUNIT_ASSERT(testSet.testDone() == true);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, query.GetNextEntry(&entry));
	}
	// GetNextRef()
	{
		Query query;
		TestSet testSet;
		TestFetchPredicateInit(query, testSet, mountPoint, predicate, entries, entryCount);
		entry_ref ref;
		while (query.GetNextRef(&ref) == B_OK) {
// Haiku supports rewinding queries, R5 does not.
#ifdef TEST_R5
			CPPUNIT_ASSERT_EQUAL(B_ERROR, query.Rewind());
#endif
			CPPUNIT_ASSERT_EQUAL(B_ERROR, query.CountEntries());
			BPath path(&ref);
			CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
			CPPUNIT_ASSERT(testSet.test(path.Path()) == true);
		}
		CPPUNIT_ASSERT(testSet.testDone() == true);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, query.GetNextRef(&ref));
	}
	// GetNextDirents()
	{
		Query query;
		TestSet testSet;
		TestFetchPredicateInit(query, testSet, mountPoint, predicate, entries, entryCount);
		size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent* ents = (dirent*)buffer;
		while (query.GetNextDirents(ents, bufSize, 1) == 1) {
// Haiku supports rewinding queries, R5 does not.
#ifdef TEST_R5
			CPPUNIT_ASSERT_EQUAL(B_ERROR, query.Rewind());
#endif
			CPPUNIT_ASSERT_EQUAL(B_ERROR, query.CountEntries());
			entry_ref ref(ents->d_pdev, ents->d_pino, ents->d_name);
			BPath path(&ref);
			CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
			CPPUNIT_ASSERT(testSet.test(path.Path()) == true);
		}
		CPPUNIT_ASSERT(testSet.testDone() == true);
		CPPUNIT_ASSERT(query.GetNextDirents(ents, bufSize, 1) == 0);
	}
	// interleaving use of the different methods
	{
		Query query;
		TestSet testSet;
		TestFetchPredicateInit(query, testSet, mountPoint, predicate, entries, entryCount);
		size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent* ents = (dirent*)buffer;
		entry_ref ref;
		BEntry entry;
		while (query.GetNextDirents(ents, bufSize, 1) == 1) {
// Haiku supports rewinding queries, R5 does not.
#ifdef TEST_R5
			CPPUNIT_ASSERT_EQUAL(B_ERROR, query.Rewind());
#endif
			CPPUNIT_ASSERT_EQUAL(B_ERROR, query.CountEntries());
			entry_ref entref(ents->d_pdev, ents->d_pino, ents->d_name);
			BPath entpath(&entref);
			CPPUNIT_ASSERT_EQUAL(B_OK, entpath.InitCheck());
			CPPUNIT_ASSERT(testSet.test(entpath.Path()) == true);
			if (query.GetNextRef(&ref) == B_OK) {
				BPath refpath(&ref);
				CPPUNIT_ASSERT_EQUAL(B_OK, refpath.InitCheck());
				CPPUNIT_ASSERT(testSet.test(refpath.Path()) == true);
			}
			if (query.GetNextEntry(&entry) == B_OK) {
				BPath path;
				CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
				CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
				CPPUNIT_ASSERT(testSet.test(path.Path()) == true);
			}
		}
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, query.GetNextEntry(&entry));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, query.GetNextRef(&ref));
		CPPUNIT_ASSERT(query.GetNextDirents(ents, bufSize, 1) == 0);
	}
}

class QueryTestEntry;
class BTestApp;

class QueryTest : public BasicTest {
	CPPUNIT_TEST_SUITE(QueryTest);
	CPPUNIT_TEST(Predicate_Empty);
	CPPUNIT_TEST(Predicate_Int32_ReturnsOk);
	CPPUNIT_TEST(Predicate_UInt32_ReturnsOk);
	CPPUNIT_TEST(Predicate_Int64_ReturnsOk);
#if !TEST_R5
	CPPUNIT_TEST(Predicate_UInt64_ReturnsOk);
#endif
	CPPUNIT_TEST(Predicate_Float_ReturnsOk);
	CPPUNIT_TEST(Predicate_Double_ReturnsOk);
	CPPUNIT_TEST(Predicate_String_ReturnsOk);
	CPPUNIT_TEST(Predicate_StringCaseInsensitive_ReturnsOk);
	CPPUNIT_TEST(Predicate_DateRelative_ReturnsOk);
	CPPUNIT_TEST(Predicate_DateAbsolute_ReturnsOk);
	CPPUNIT_TEST(Predicate_DateTomorrow_ReturnsOk);
	CPPUNIT_TEST(Predicate_DateTime_ReturnsOk);
	CPPUNIT_TEST(Predicate_DateInvalid_ReturnsError);
	CPPUNIT_TEST(Predicate_Attribute_ReturnsOk);
	CPPUNIT_TEST(Predicate_OperatorEqual_ReturnsOk);
	CPPUNIT_TEST(Predicate_OperatorGreater_ReturnsOk);
	CPPUNIT_TEST(Predicate_OperatorGreaterEqual_ReturnsOk);
	CPPUNIT_TEST(Predicate_OperatorLess_ReturnsOk);
	CPPUNIT_TEST(Predicate_OperatorLessEqual_ReturnsOk);
	CPPUNIT_TEST(Predicate_OperatorNotEqual_ReturnsOk);
	CPPUNIT_TEST(Predicate_OperatorContains_ReturnsOk);
	CPPUNIT_TEST(Predicate_OperatorBeginsWith_ReturnsOk);
	CPPUNIT_TEST(Predicate_OperatorEndsWith_ReturnsOk);
	CPPUNIT_TEST(Predicate_OperatorAnd_ReturnsOk);
	CPPUNIT_TEST(Predicate_OperatorOr_ReturnsOk);
	CPPUNIT_TEST(Predicate_OperatorNot_ReturnsOk);
	CPPUNIT_TEST(Predicate_WellFormedLegal);
	CPPUNIT_TEST(Predicate_WellFormedIllegal);
	CPPUNIT_TEST(Predicate_IllFormed);
	CPPUNIT_TEST(Predicate_Precedence);
	CPPUNIT_TEST(Predicate_Get_SetPredicate);
	CPPUNIT_TEST(Predicate_Length_SetPredicate);
	CPPUNIT_TEST(Predicate_Fetch_PushNotAllowed);
#if !TEST_R5
	CPPUNIT_TEST(Predicate_Args_ReturnsError);
#endif
	CPPUNIT_TEST(Parameter_PushDate);
	CPPUNIT_TEST(Parameter_PushQuery);
	CPPUNIT_TEST(Parameter_FetchLive);
	CPPUNIT_TEST(Parameter_SetVolume_ReturnsOk);
	CPPUNIT_TEST(Parameter_IsLive_DefaultFalse);
	CPPUNIT_TEST(Parameter_SetTarget_Invalid_ReturnsError);
	CPPUNIT_TEST(Parameter_SetTarget_Valid_ReturnsOk);
	CPPUNIT_TEST(Parameter_SetVolumeAndTargetAfterFetch_ReturnsError);
	CPPUNIT_TEST(Parameter_FetchNoVolume_ReturnsError);
	CPPUNIT_TEST(Fetch_NoInit_ReturnsError);
	CPPUNIT_TEST(Fetch_NoPredicate_ReturnsError);
	CPPUNIT_TEST(Fetch_NoVolume_ReturnsError);
	CPPUNIT_TEST(Fetch_InvalidPredicate_ReturnsError);
	CPPUNIT_TEST(Fetch_DoubleFetch_ReturnsError);
	CPPUNIT_TEST(Fetch_Files_ReturnsOk);
	CPPUNIT_TEST(Fetch_AttributeTypes_Attribute_ReturnsOk);
	CPPUNIT_TEST(Fetch_AttributeTypes_EndsWith_ReturnsOk);
	CPPUNIT_TEST(Fetch_Clear_ReturnsOk);
	CPPUNIT_TEST(Fetch_FetchAfterClear_ReturnsNoInit);
	CPPUNIT_TEST(Fetch_ReInit_ReturnsOk);
	CPPUNIT_TEST(Fetch_NextNoInit_ReturnsError);
	CPPUNIT_TEST(Fetch_NextNotFound_ReturnsError);
	CPPUNIT_TEST(Fetch_NextNull_ReturnsError);
	CPPUNIT_TEST(Live_Basic);
	CPPUNIT_TEST(Live_Move);
	CPPUNIT_TEST_SUITE_END();

	BTestApp* fApplication;
	bool fVolumeCreated;
	std::string fImageFile;

	void _RemoveLiveEntries(QueryTestEntry** entries, int32 entryCount,
		QueryTestEntry** queryEntries, int32 queryEntryCount)
	{
		delete_test_entries(entries, entryCount);
		CheckUpdateMessages(B_ENTRY_REMOVED, queryEntries, queryEntryCount);
		for (int32 i = 0; i < entryCount; i++) {
			QueryTestEntry* entry = entries[i];
			entry->directory = -1;
			entry->node = -1;
			entry->name = "";
		}
	}

	void _AddLiveEntries(QueryTestEntry** entries, int32 entryCount, QueryTestEntry** queryEntries,
		int32 queryEntryCount)
	{
		create_test_entries(entries, entryCount);
		for (int32 i = 0; i < entryCount; i++) {
			QueryTestEntry* entry = entries[i];
			BNode node(entry->cpath);
			CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
			node_ref nref;
			CPPUNIT_ASSERT_EQUAL(B_OK, node.GetNodeRef(&nref));
			entry->node = nref.node;
			entry_ref ref;
			CPPUNIT_ASSERT_EQUAL(B_OK, get_ref_for_path(entry->cpath, &ref));
			entry->directory = ref.directory;
			entry->name = ref.name;
		}
		CheckUpdateMessages(B_ENTRY_CREATED, queryEntries, queryEntryCount);
	}

public:
	void setUp()
	{
		BasicTest::setUp();
		fApplication = new BTestApp("application/x-vnd.obos.query-test");
		if (fApplication->Init() != B_OK) {
			fprintf(stderr, "Failed to initialize application.\n");
			delete fApplication;
			fApplication = NULL;
		}
	}

	void tearDown()
	{
		BasicTest::tearDown();
		if (fApplication) {
			fApplication->Terminate();
			delete fApplication;
			fApplication = NULL;
		}
	}

	void Predicate_Empty()
	{
		char buffer[1024];
		Query query;
		BString predicate;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, query.GetPredicate(&predicate));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, query.GetPredicate(buffer, sizeof(buffer)));
	}

	void Predicate_Int32_ReturnsOk() { TestPredicate(Int32Node(42)); }

	void Predicate_UInt32_ReturnsOk() { TestPredicate(UInt32Node(42)); }

	void Predicate_Int64_ReturnsOk() { TestPredicate(Int64Node(42)); }

	void Predicate_UInt64_ReturnsOk()
	{
#if !TEST_R5
		TestPredicate(UInt64Node(42));
#endif
	}

	void Predicate_Float_ReturnsOk() { TestPredicate(FloatNode(42)); }

	void Predicate_Double_ReturnsOk() { TestPredicate(DoubleNode(42)); }

	void Predicate_String_ReturnsOk()
	{
		TestPredicate(StringNode("some \" chars ' to \\ be ( escaped ) or % not!"));
	}

	void Predicate_StringCaseInsensitive_ReturnsOk()
	{
		TestPredicate(StringNode("some \" chars ' to \\ be ( escaped ) or % not!", true));
	}

	void Predicate_DateRelative_ReturnsOk() { TestPredicate(DateNode("+15 min")); }

	void Predicate_DateAbsolute_ReturnsOk() { TestPredicate(DateNode("22 May 2002")); }

	void Predicate_DateTomorrow_ReturnsOk() { TestPredicate(DateNode("tomorrow")); }

	void Predicate_DateTime_ReturnsOk() { TestPredicate(DateNode("17:57")); }

	void Predicate_DateInvalid_ReturnsError()
	{
		TestPredicate(DateNode("invalid date"), B_BAD_VALUE);
	}

	void Predicate_Attribute_ReturnsOk() { TestPredicate(AttributeNode("some attribute")); }

	void Predicate_OperatorEqual_ReturnsOk() { TestOperator(B_EQ); }

	void Predicate_OperatorGreater_ReturnsOk() { TestOperator(B_GT); }

	void Predicate_OperatorGreaterEqual_ReturnsOk() { TestOperator(B_GE); }

	void Predicate_OperatorLess_ReturnsOk() { TestOperator(B_LT); }

	void Predicate_OperatorLessEqual_ReturnsOk() { TestOperator(B_LE); }

	void Predicate_OperatorNotEqual_ReturnsOk() { TestOperator(B_NE); }

	void Predicate_OperatorContains_ReturnsOk() { TestOperator(B_CONTAINS); }

	void Predicate_OperatorBeginsWith_ReturnsOk() { TestOperator(B_BEGINS_WITH); }

	void Predicate_OperatorEndsWith_ReturnsOk() { TestOperator(B_ENDS_WITH); }

	void Predicate_OperatorAnd_ReturnsOk() { TestOperator(B_AND); }

	void Predicate_OperatorOr_ReturnsOk() { TestOperator(B_OR); }

	void Predicate_OperatorNot_ReturnsOk()
	{
		TestPredicate(OpNode(B_NOT, new AttributeNode("attribute")));
		TestPredicate(OpNode(B_NOT, new Int32Node(42)));
		TestPredicate(OpNode(B_NOT, new StringNode("some string")));
		TestPredicate(OpNode(B_NOT, new StringNode("some string", true)));
		TestPredicate(OpNode(B_NOT, new DateNode("22 May 2002")));
		TestPredicate(OpNode(B_NOT, NULL), B_OK, B_NO_INIT);
	}

	void Predicate_WellFormedLegal()
	{
		TestPredicate(OpNode(B_AND,
			new OpNode(B_CONTAINS, new AttributeNode("attribute"), new StringNode("hello")),
			new OpNode(B_OR,
				new OpNode(B_NOT,
					new OpNode(B_EQ, new AttributeNode("attribute2"), new UInt32Node(7)), NULL),
				new OpNode(B_GE, new AttributeNode("attribute3"), new DateNode("20 May 2002")))));
	}

	void Predicate_WellFormedIllegal()
	{
		TestPredicate(OpNode(B_EQ, new StringNode("hello"),
			new OpNode(B_LE, new OpNode(B_NOT, new Int32Node(17), NULL),
				new DateNode("20 May 2002"))));
	}

	void Predicate_IllFormed()
	{
		TestPredicate(ListNode(new Int32Node(42), new StringNode("hello!")), B_OK, B_NO_INIT);
		TestPredicate(OpNode(B_EQ, new StringNode("hello"), new OpNode(B_NOT, NULL)), B_OK,
			B_NO_INIT);
	}

	void Predicate_Precedence()
	{
		Query query;
		OpNode predicate1(B_CONTAINS, new AttributeNode("attribute"), new StringNode("hello"));
		StringNode predicate2("I'm the loser. :´-(");
		CPPUNIT_ASSERT_EQUAL(B_OK, predicate1.push(query));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate(predicate2.toString().String()));
		BString predicate;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.GetPredicate(&predicate));
		CPPUNIT_ASSERT(predicate == predicate1.toString());
	}

	void Predicate_Get_SetPredicate()
	{
		Query query;
		OpNode predicate1(B_CONTAINS, new AttributeNode("attribute"), new StringNode("hello"));
		StringNode predicate2("I'm the winner. :-)");
		CPPUNIT_ASSERT_EQUAL(B_OK, predicate1.push(query));
		BString predicate;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.GetPredicate(&predicate));
		CPPUNIT_ASSERT(predicate == predicate1.toString());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate(predicate2.toString().String()));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.GetPredicate(&predicate));
		CPPUNIT_ASSERT(predicate == predicate2.toString());
	}

	void Predicate_Length_SetPredicate()
	{
		Query query;
		OpNode predicate1(B_CONTAINS, new AttributeNode("attribute"), new StringNode("hello"));
		StringNode predicate2("I'm the winner. :-)");
		CPPUNIT_ASSERT_EQUAL(B_OK, predicate1.push(query));
		CPPUNIT_ASSERT((int32)query.PredicateLength() == predicate1.toString().Length() + 1);
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate(predicate2.toString().String()));
		BString predicate;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.GetPredicate(&predicate));
		CPPUNIT_ASSERT(predicate == predicate2.toString());
	}

	void Predicate_Fetch_PushNotAllowed()
	{
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("name=\"ThisShouldNotExist\""));
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.Fetch());
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.SetPredicate("name=\"ThisShouldNotExistEither\""));
#if TEST_R5
		CPPUNIT_ASSERT_EQUAL(B_OK, query.PushDate("20 May 2002"));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.PushValue((int32)42));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.PushValue((uint32)42));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.PushValue((int64)42));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.PushValue((uint64)42));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.PushValue((float)42));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.PushValue((double)42));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.PushValue("hello"));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.PushAttr("attribute"));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.PushOp(B_EQ));
#else
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.PushDate("20 May 2002"));
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.PushValue((int32)42));
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.PushValue((uint32)42));
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.PushValue((int64)42));
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.PushValue((uint64)42));
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.PushValue((float)42));
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.PushValue((double)42));
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.PushValue("hello"));
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.PushAttr("attribute"));
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.PushOp(B_EQ));
#endif
	}

	void Predicate_Args_ReturnsError()
	{
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, query.SetPredicate(NULL));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("hello"));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, query.GetPredicate(NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, query.GetPredicate(NULL, 10));
	}

	void Parameter_PushDate()
	{
		BQuery query;
		CPPUNIT_ASSERT_EQUAL(B_ERROR, query.TargetDevice());
	}

	void Parameter_PushQuery()
	{
#if !TEST_R5
		BQuery query;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, query.SetVolume(NULL));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, query.TargetDevice());
#endif
	}

	void Parameter_FetchLive()
	{
		BQuery query;
		BVolume volume(-2);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, query.TargetDevice());
	}

	void Parameter_SetVolume_ReturnsOk()
	{
		BQuery query;
		dev_t device = dev_for_path("/boot");
		BVolume volume(device);
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
		CPPUNIT_ASSERT(query.TargetDevice() == device);
	}

	void Parameter_IsLive_DefaultFalse()
	{
		BQuery query;
		CPPUNIT_ASSERT(query.IsLive() == false);
	}

	void Parameter_SetTarget_Invalid_ReturnsError()
	{
		BQuery query;
		BMessenger messenger;
		CPPUNIT_ASSERT(messenger.IsValid() == false);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, query.SetTarget(messenger));
		CPPUNIT_ASSERT(query.IsLive() == false);
	}

	void Parameter_SetTarget_Valid_ReturnsOk()
	{
		BQuery query;
		BMessenger messenger(&fApplication->Handler());
		CPPUNIT_ASSERT(messenger.IsValid() == true);
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetTarget(messenger));
		CPPUNIT_ASSERT(query.IsLive() == true);
	}

	void Parameter_SetVolumeAndTargetAfterFetch_ReturnsError()
	{
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("name=\"ThisShouldNotExist\""));
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.Fetch());
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.SetVolume(&volume));
		BMessenger messenger(&fApplication->Handler());
		CPPUNIT_ASSERT(messenger.IsValid() == true);
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.SetTarget(messenger));
	}

	void Parameter_FetchNoVolume_ReturnsError()
	{
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("name=\"ThisShouldNotExist\""));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, query.Fetch());
	}

	void Fetch_NoInit_ReturnsError()
	{
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, query.Fetch());
	}

	void Fetch_NoPredicate_ReturnsError()
	{
		Query query;
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, query.Fetch());
	}

	void Fetch_NoVolume_ReturnsError()
	{
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("name=\"ThisShouldNotExist\""));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, query.Fetch());
	}

	void Fetch_InvalidPredicate_ReturnsError()
	{
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("name=\"ThisShouldNotExist\"&&"));
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, query.Fetch());
	}

	void Fetch_DoubleFetch_ReturnsError()
	{
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("name=\"ThisShouldNotExist\""));
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.Fetch());
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, query.Fetch());
	}

	void Fetch_Files_ReturnsOk()
	{
		fImageFile = CreateVolume(testMountPoint, 2);
		create_test_entries(allTestEntries, allTestEntryCount);
		{
			QueryTestEntry* entries[]
				= {&file11, &file12, &file21, &file22, &file31, &file32, &file1, &file2, &file3};
			const int32 entryCount = sizeof(entries) / sizeof(QueryTestEntry*);
			TestFetchPredicate(testMountPoint, "name=\"file*\"", entries, entryCount);
		}
		DeleteVolume(fImageFile, testMountPoint);
	}

	void Fetch_AttributeTypes_Attribute_ReturnsOk()
	{
		fImageFile = CreateVolume(testMountPoint, 2);
		create_test_entries(allTestEntries, allTestEntryCount);
		QueryTestEntry* entries[] = {&file11, &file12, &link11, &file21, &file22, &link21, &file31,
			&file32, &link31, &file1, &file2, &file3, &link1, &link2, &link3};
		const int32 entryCount = sizeof(entries) / sizeof(QueryTestEntry*);
		TestFetchPredicate(testMountPoint, "name=\"*l*\"", entries, entryCount);
		DeleteVolume(fImageFile, testMountPoint);
	}

	void Fetch_AttributeTypes_EndsWith_ReturnsOk()
	{
		fImageFile = CreateVolume(testMountPoint, 2);
		create_test_entries(allTestEntries, allTestEntryCount);
		QueryTestEntry* entries[]
			= {&subdir12, &file12, &dir2, &subdir22, &file22, &subdir32, &file32, &file2, &link2};
		const int32 entryCount = sizeof(entries) / sizeof(QueryTestEntry*);
		TestFetchPredicate(testMountPoint, "name=\"*2\"", entries, entryCount);
		DeleteVolume(fImageFile, testMountPoint);
	}

	void Fetch_Clear_ReturnsOk()
	{
		
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.Clear());
	}

	void Fetch_FetchAfterClear_ReturnsNoInit()
	{
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("name=\"ThisShouldNotExist\""));
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.Fetch());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.Clear());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, query.Fetch());
	}


	void Fetch_ReInit_ReturnsOk()
	{
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("name=\"ThisShouldNotExist\""));
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.Fetch());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.Clear());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("name=\"ThisShouldNotExist\""));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.SetTo(dev_for_path("/boot")));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.Fetch());
	}

	void Fetch_NextNoInit_ReturnsError()
	{
		Query query;
		BEntry entry;
		entry_ref ref;
		const size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent* ents = (dirent*)buffer;
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, query.GetNextEntry(&entry));
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, query.GetNextRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, query.GetNextDirents(ents, bufSize, 1));
	}

	void Fetch_NextNotFound_ReturnsError()
	{
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("name=\"ThisShouldNotExist\""));
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.Fetch());
		BEntry entry;
		entry_ref ref;
		const size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent* ents = (dirent*)buffer;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, query.GetNextEntry(&entry));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, query.GetNextRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, query.GetNextDirents(ents, bufSize, 1));
	}

	void Fetch_NextNull_ReturnsError()
	{
#if !TEST_R5
		Query query;
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("name=\"ThisShouldNotExist\""));
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
		CPPUNIT_ASSERT_EQUAL(B_OK, query.Fetch());
		size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, query.GetNextEntry(NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, query.GetNextRef(NULL));
		CPPUNIT_ASSERT(equals(query.GetNextDirents(NULL, bufSize, 1), B_BAD_ADDRESS, B_BAD_VALUE));
#endif
	}

	void Live_Basic()
	{
		CPPUNIT_ASSERT(fApplication != NULL);
		fImageFile = CreateVolume(testMountPoint, 2);
		fVolumeCreated = true;
		create_test_entries(allTestEntries, allTestEntryCount);
		BMessenger target(&fApplication->Handler());

		// empty query, add some files, remove some files
		{
			Query query;
			CPPUNIT_ASSERT_EQUAL(B_OK, query.SetPredicate("name=\"*Argh\""));
			BVolume volume(dev_for_path(testMountPoint));
			CPPUNIT_ASSERT_EQUAL(B_OK, volume.InitCheck());
			CPPUNIT_ASSERT_EQUAL(B_OK, query.SetVolume(&volume));
			CPPUNIT_ASSERT_EQUAL(B_OK, query.SetTarget(target));
			CPPUNIT_ASSERT_EQUAL(B_OK, query.Fetch());
			BEntry entry;
			CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, query.GetNextEntry(&entry));
			// the test entries
			QueryTestEntry testDir1(dir1 + "testDirArgh", B_DIRECTORY_NODE);
			QueryTestEntry testDir2(dir1 + "testDir2", B_DIRECTORY_NODE);
			QueryTestEntry testFile1(subdir21 + "testFileArgh", B_FILE_NODE);
			QueryTestEntry testFile2(subdir21 + "testFile2", B_FILE_NODE);
			QueryTestEntry testLink1(subdir32 + "testLinkArgh", B_SYMLINK_NODE, &file11);
			QueryTestEntry testLink2(subdir32 + "testLink2", B_SYMLINK_NODE, &file11);
			QueryTestEntry* entries[]
				= {&testDir1, &testDir2, &testFile1, &testFile2, &testLink1, &testLink2};
			int32 entryCount = sizeof(entries) / sizeof(QueryTestEntry*);
			QueryTestEntry* queryEntries[] = {&testDir1, &testFile1, &testLink1};
			int32 queryEntryCount = sizeof(queryEntries) / sizeof(QueryTestEntry*);
			_AddLiveEntries(entries, entryCount, queryEntries, queryEntryCount);
			_RemoveLiveEntries(entries, entryCount, queryEntries, queryEntryCount);
		}
		DeleteVolume(fImageFile, testMountPoint);
	}

	void Live_Move()
	{
		CPPUNIT_ASSERT(fApplication != NULL);
		fImageFile = CreateVolume(testMountPoint, 2);
		fVolumeCreated = true;
		create_test_entries(allTestEntries, allTestEntryCount);
		BMessenger target(&fApplication->Handler());
		// non-empty query, add some files, remove some files
		{
			Query query;
			TestSet testSet;
			CPPUNIT_ASSERT_EQUAL(B_OK, query.SetTarget(target));
			QueryTestEntry* initialEntries[]
				= {&file11, &file12, &file21, &file22, &file31, &file32, &file1, &file2, &file3};
			int32 initialEntryCount = sizeof(initialEntries) / sizeof(QueryTestEntry*);
			TestFetchPredicateInit(query, testSet, testMountPoint, "name=\"*ile*\"", initialEntries,
				initialEntryCount);
			BEntry entry;
			while (query.GetNextEntry(&entry) == B_OK) {
				BPath path;
				CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
				CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
				CPPUNIT_ASSERT(testSet.test(path.Path()) == true);
			}
			CPPUNIT_ASSERT(testSet.testDone() == true);
			CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, query.GetNextEntry(&entry));
			// the test entries
			QueryTestEntry testDir1(dir1 + "testDir1", B_DIRECTORY_NODE);
			QueryTestEntry testDir2(dir1 + "testDir2", B_DIRECTORY_NODE);
			QueryTestEntry testFile1(subdir21 + "testFile1", B_FILE_NODE);
			QueryTestEntry testFile2(subdir21 + "testFile2", B_FILE_NODE);
			QueryTestEntry testLink1(subdir32 + "testLink1", B_SYMLINK_NODE, &file11);
			QueryTestEntry testLink2(subdir32 + "testLink2", B_SYMLINK_NODE, &file11);
			QueryTestEntry testFile3(subdir32 + "testFile3", B_FILE_NODE);
			QueryTestEntry* entries[] = {&testDir1, &testDir2, &testFile1, &testFile2, &testLink1,
				&testLink2, &testFile3};
			int32 entryCount = sizeof(entries) / sizeof(QueryTestEntry*);
			QueryTestEntry* queryEntries[] = {&testFile1, &testFile2, &testFile3};
			int32 queryEntryCount = sizeof(queryEntries) / sizeof(QueryTestEntry*);
			_AddLiveEntries(entries, entryCount, queryEntries, queryEntryCount);
			_RemoveLiveEntries(entries, entryCount, queryEntries, queryEntryCount);
		}
		DeleteVolume(fImageFile, testMountPoint);
	}

	void CheckUpdateMessages(uint32 opcode, QueryTestEntry** entries, int32 entryCount)
	{
		// wait for the messages
		snooze(100000);
		if (fApplication) {
			BMessageQueue& queue = fApplication->Handler().Queue();
			CPPUNIT_ASSERT(queue.Lock());
			try {
				int32 entryNum = 0;
				while (BMessage* _message = queue.NextMessage()) {
					BMessage message(*_message);
					delete _message;
					CPPUNIT_ASSERT(entryNum < entryCount);
					QueryTestEntry* entry = entries[entryNum];
					CPPUNIT_ASSERT_EQUAL(B_QUERY_UPDATE, message.what);
					uint32 msgOpcode;
					CPPUNIT_ASSERT_EQUAL(B_OK, message.FindInt32("opcode", (int32*)&msgOpcode));
					CPPUNIT_ASSERT(msgOpcode == opcode);
					dev_t device;
					CPPUNIT_ASSERT_EQUAL(B_OK, message.FindInt32("device", &device));
					CPPUNIT_ASSERT(device == dev_for_path(testMountPoint));
					ino_t directory;
					CPPUNIT_ASSERT_EQUAL(B_OK, message.FindInt64("directory", &directory));
					CPPUNIT_ASSERT(directory == entry->directory);
					ino_t node;
					CPPUNIT_ASSERT_EQUAL(B_OK, message.FindInt64("node", &node));
					CPPUNIT_ASSERT(node == entry->node);
					if (opcode == B_ENTRY_CREATED) {
						const char* name;
						CPPUNIT_ASSERT_EQUAL(B_OK, message.FindString("name", &name));
						CPPUNIT_ASSERT(entry->name == name);
					}
					entryNum++;
				}
				CPPUNIT_ASSERT(entryNum == entryCount);
			} catch (CppUnit::Exception exception) {
				queue.Unlock();
				throw exception;
			}
			queue.Unlock();
		}
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(QueryTest, getTestSuiteName());
